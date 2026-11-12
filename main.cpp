#include <cctype>
#include <climits>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <unordered_set>
#include <vector>
long long max_line_chars = LLONG_MAX;
bool no_chars = false;
bool is_hex_digit(char c) {
  return std ::isxdigit(static_cast<unsigned char>(c));
}
bool is_id_char(char c) {
  unsigned char u = static_cast<unsigned char>(c);
  return std ::isalnum(u) || c == '_' || c == '$' || u >= 0x80;
}
bool is_id_start_char(char c) {
  unsigned char u = static_cast<unsigned char>(c);
  return std ::isalpha(u) || c == '_' || c == '$' || u >= 0x80;
}
// Length of the universal-character-name starting at pos, or 0.
size_t ucn_length(const std ::string &s, size_t pos) {
  if (pos + 1 >= s.size() || s[pos] != '\\')
    return 0;
  char kind = s[pos + 1];
  if (kind == 'u' || kind == 'U') {
    size_t n = kind == 'u' ? 4 : 8;
    if (pos + 2 + n > s.size())
      return 0;
    for (size_t k = 0; k < n; k++)
      if (!is_hex_digit(s[pos + 2 + k]))
        return 0;
    return 2 + n;
  }
  if (kind == 'N' && pos + 2 < s.size() && s[pos + 2] == '{') {
    size_t close = s.find('}', pos + 3);
    if (close == std ::string ::npos)
      return 0;
    return close - pos + 1;
  }
  return 0;
}
size_t scan_identifier(const std ::string &s, size_t pos) {
  size_t begin = pos;
  while (pos < s.size()) {
    size_t n = ucn_length(s, pos);
    if (n) {
      pos += n;
      continue;
    }
    if (!is_id_char(s[pos]))
      break;
    pos++;
  }
  return pos - begin;
}
bool is_pp_number_start(const std ::string &s, size_t pos) {
  if (pos >= s.size())
    return false;
  if (std ::isdigit(static_cast<unsigned char>(s[pos])))
    return true;
  return s[pos] == '.' && pos + 1 < s.size() &&
         std ::isdigit(static_cast<unsigned char>(s[pos + 1]));
}
// Preprocessing numbers keep floats, digit separators and suffixes together:
// 1.009, .5, 5., 1e-9, 0x1.8p+3f, 1'000'000, 0b1010'1010u, 42_km.
size_t scan_pp_number(const std ::string &s, size_t pos) {
  size_t begin = pos;
  while (pos < s.size()) {
    char c = s[pos];
    if ((c == 'e' || c == 'E' || c == 'p' || c == 'P') &&
        pos + 1 < s.size() && (s[pos + 1] == '+' || s[pos + 1] == '-')) {
      pos += 2;
      continue;
    }
    if (is_id_char(c) || c == '.' || c == '\'') {
      pos++;
      continue;
    }
    size_t n = ucn_length(s, pos);
    if (n) {
      pos += n;
      continue;
    }
    break;
  }
  return pos - begin;
}
// If a string/char literal starts at pos, returns the encoding-prefix length
// and sets is_raw. Returns npos when no literal starts there.
size_t literal_prefix(const std ::string &s, size_t pos, bool &is_raw) {
  is_raw = false;
  if (pos >= s.size())
    return std ::string ::npos;
  size_t p = pos;
  if (s.compare(pos, 3, "u8R") == 0) {
    p = pos + 3;
    is_raw = true;
  } else if (s.compare(pos, 2, "uR") == 0 || s.compare(pos, 2, "UR") == 0 ||
             s.compare(pos, 2, "LR") == 0) {
    p = pos + 2;
    is_raw = true;
  } else if (s.compare(pos, 1, "R") == 0) {
    p = pos + 1;
    is_raw = true;
  } else if (s.compare(pos, 2, "u8") == 0) {
    p = pos + 2;
  } else if (s.compare(pos, 1, "u") == 0 || s.compare(pos, 1, "U") == 0 ||
             s.compare(pos, 1, "L") == 0) {
    p = pos + 1;
  }
  if (p == pos) {
    if (s[pos] == '"' || s[pos] == '\'')
      return 0;
    return std ::string ::npos;
  }
  if (p >= s.size())
    return std ::string ::npos;
  if (s[p] == '"')
    return p - pos;
  if (s[p] == '\'' && !is_raw)
    return p - pos;
  return std ::string ::npos;
}
// Scan a quoted literal starting at quote_pos and glue any user-defined
// literal suffix ("abc"sv, 'x'_c, R"(x)"_r) onto it. Returns the length.
size_t scan_quoted_literal(const std ::string &s, size_t quote_pos) {
  char quote = s[quote_pos];
  size_t pos = quote_pos + 1;
  while (pos < s.size()) {
    if (s[pos] == '\\') {
      pos += 2;
      continue;
    }
    if (s[pos] == quote) {
      pos++;
      break;
    }
    pos++;
  }
  pos += scan_identifier(s, pos);
  return pos - quote_pos;
}
// Tokens that cannot be replaced by an object-like macro name: multi-line raw
// strings (the newline would end the #define) and "(", ")" and "," (a
// function-like macro name produced by an alias is only invoked when the next
// token is a real "(", and its arguments are only split/terminated by real ","
// and ")"). They are emitted literally instead.
bool is_aliasable_token(const std ::string &t) {
  return t.find('\n') == std ::string ::npos && t != "(" && t != ")" &&
         t != ",";
}
void collect_identifiers(const std ::string &text,
                         std ::unordered_set<std ::string> &out) {
  size_t i = 0;
  while (i < text.size()) {
    if (ucn_length(text, i) || is_id_start_char(text[i])) {
      size_t n = scan_identifier(text, i);
      if (n == 0) {
        i++;
        continue;
      }
      out.insert(text.substr(i, n));
      i += n;
      continue;
    }
    i++;
  }
}
// Quote a path so it survives system() unchanged: single quotes suppress
// expansion, and an embedded quote is closed/reopened as '\''.
std ::string shell_quote(const std ::string &s) {
  std ::string quoted = "'";
  for (char c : s) {
    if (c == '\'')
      quoted += "'\\''";
    else
      quoted += c;
  }
  quoted += "'";
  return quoted;
}
std ::string rm_command(const std ::string &path) {
  return "rm -f " + shell_quote(path);
}
std ::string gpp_command(const std ::string &in, const std ::string &out) {
  return "g++ -E -P -x c++ " + shell_quote(in) + " -o " + shell_quote(out);
}
std ::string raw_end_marker(const std ::string &delim) {
  return ")" + delim + "\"";
}
std ::string
generate_id(long long n, int L, const std ::vector<std ::string> &chars) {
  int K = chars.size();
  std ::vector<std ::string> res_parts(L, chars[0]);
  for (int i = L - 1; i >= 0; i--) {
    res_parts[i] = chars[n % K];
    n /= K;
  }
  std ::string res;
  for (const auto &s : res_parts)
    res += s;
  return res;
}
// Assign fixed-length aliases, skipping names that already occur in the input
// (a macro whose name is an input token would expand recursively and corrupt
// the program) and growing the aliases when the candidate space runs out.
std ::map<std ::string, std ::string>
create_mapping(const std ::unordered_set<std ::string> &unique_items,
               const std ::vector<std ::string> &chars,
               const std ::unordered_set<std ::string> &forbidden) {
  std ::map<std ::string, std ::string> mapping;
  size_t K = chars.size();
  size_t N = unique_items.size();
  if (N == 0)
    return mapping;
  int L = 1;
  long long cap = static_cast<long long>(K);
  while (cap < static_cast<long long>(N)) {
    cap *= static_cast<long long>(K);
    L++;
  }
  for (;;) {
    mapping.clear();
    long long idx = 0;
    bool complete = true;
    for (const auto &item : unique_items) {
      std ::string id;
      do {
        if (idx >= cap) {
          complete = false;
          break;
        }
        id = generate_id(idx++, L, chars);
      } while (forbidden.find(id) != forbidden.end());
      if (!complete)
        break;
      mapping[item] = id;
    }
    if (complete)
      break;
    cap *= static_cast<long long>(K);
    L++;
  }
  return mapping;
}
std ::vector<std ::string>
tokenize_stream(std ::ifstream &input,
                std ::vector<std ::string> &preprocessors) {
  std ::vector<std ::string> tokens;
  std ::string line;
  const std ::vector<std ::string> operators = {
      "...", "<=>", ">>=", "<<=", "->*", "->", "::", "++", "--", "==", "!=",
      "<=",  ">=",  "+=",  "-=",  "*=",  "/=", "%=", "|=", "&=", "^=", "&&",
      "||",  "<<",  ">>",  "<:",  ":>",  "<%",  "%>",  "+",  "-",  "*",  "/",
      "%",   "=",   "<",   ">",   "!",   "&",   "|",   "^",   "~",  "?",  ":",
      ",",   ".*",
  };
  while (std ::getline(input, line)) {
    if (line.empty())
      continue;
    size_t first_char = line.find_first_not_of(" \t");
    if (first_char != std ::string ::npos && line[first_char] == '#') {
      std ::string combined = line;
      while (!combined.empty() && combined.back() == '\\') {
        combined.pop_back();
        std ::string next_line;
        if (!std ::getline(input, next_line))
          break;
        combined += " " + next_line;
      }
      preprocessors.push_back(combined);
      continue;
    }
    size_t i = 0;
    while (i < line.size()) {
      if (std ::isspace(static_cast<unsigned char>(line[i]))) {
        i++;
        continue;
      }
      bool is_raw = false;
      size_t prefix = literal_prefix(line, i, is_raw);
      if (prefix != std ::string ::npos) {
        size_t quote_pos = i + prefix;
        if (is_raw) {
          size_t paren = line.find('(', quote_pos + 1);
          if (paren == std ::string ::npos) {
            // Not a valid raw-string opener; keep the prefix as an identifier.
            size_t id_len = scan_identifier(line, i);
            if (id_len == 0)
              id_len = prefix;
            tokens.push_back(line.substr(i, id_len));
            i += id_len;
            continue;
          }
          std ::string delim =
              line.substr(quote_pos + 1, paren - quote_pos - 1);
          std ::string end_marker = raw_end_marker(delim);
          std ::string raw_text = line.substr(i, paren + 1 - i);
          std ::string rest = line.substr(paren + 1);
          size_t end_pos = rest.find(end_marker);
          bool closed = end_pos != std ::string ::npos;
          if (!closed) {
            raw_text += rest;
            std ::string next_line;
            while (std ::getline(input, next_line)) {
              raw_text += "\n";
              rest = next_line;
              end_pos = rest.find(end_marker);
              if (end_pos != std ::string ::npos) {
                closed = true;
                break;
              }
              raw_text += rest;
            }
          }
          if (!closed) {
            tokens.push_back(raw_text);
            return tokens;
          }
          raw_text += rest.substr(0, end_pos + end_marker.size());
          line = rest;
          i = end_pos + end_marker.size();
          size_t suffix = scan_identifier(line, i);
          if (suffix) {
            raw_text += line.substr(i, suffix);
            i += suffix;
          }
          tokens.push_back(raw_text);
          continue;
        }
        size_t lit_len = scan_quoted_literal(line, quote_pos);
        tokens.push_back(line.substr(i, prefix + lit_len));
        i += prefix + lit_len;
        continue;
      }
      if (is_pp_number_start(line, i)) {
        size_t n = scan_pp_number(line, i);
        tokens.push_back(line.substr(i, n));
        i += n;
        continue;
      }
      if (ucn_length(line, i) || is_id_start_char(line[i])) {
        size_t n = scan_identifier(line, i);
        if (n == 0)
          n = 1;
        tokens.push_back(line.substr(i, n));
        i += n;
        continue;
      }
      if (line.compare(i, 2, "//") == 0)
        break;
      bool matched = false;
      for (const auto &op : operators) {
        if (line.compare(i, op.size(), op) == 0) {
          // C++ lexing rule: "<:" followed by ':' or '>' is "<" plus the
          // rest, so std::vector<::ns::T> stays a template argument list.
          if (op == "<:" && i + 2 < line.size() &&
              (line[i + 2] == ':' || line[i + 2] == '>'))
            continue;
          tokens.push_back(op);
          i += op.size();
          matched = true;
          break;
        }
      }
      if (matched)
        continue;
      tokens.push_back(std ::string(1, line[i++]));
    }
  }
  return tokens;
}
void obfuscate(std ::ifstream &input, std ::ofstream &output,
               const std ::vector<std ::string> &chars, std ::string head,
               int line_count) {
  std ::vector<std ::string> preprocessors;
  std ::vector<std ::string> tokens = tokenize_stream(input, preprocessors);
  if (!head.empty())
    output << head << "\n";
  for (const auto &p : preprocessors)
    output << p << "\n";
  int cnt = 0;
  long long line_chars = 0;
  if (no_chars) {
    for (const auto &t : tokens) {
      output << t << " ";
      line_chars += t.size();
      if (t.find('\n') != std ::string ::npos) {
        line_chars = cnt = 0;
        continue;
      }
      if (line_chars >= max_line_chars || ++cnt % line_count == 0) {
        line_chars = cnt = 0;
        output << "\n";
      }
    }
    return;
  }
  std ::unordered_set<std ::string> unique_tokens;
  for (const auto &t : tokens)
    if (is_aliasable_token(t))
      unique_tokens.insert(t);
  std ::unordered_set<std ::string> forbidden = unique_tokens;
  for (const auto &p : preprocessors)
    collect_identifiers(p, forbidden);
  collect_identifiers(head, forbidden);
  auto mapping = create_mapping(unique_tokens, chars, forbidden);
  for (const auto &m : mapping)
    output << "#define " << m.second << " " << m.first << "\n";
  for (const auto &t : tokens) {
    std ::string text = t;
    auto it = mapping.find(t);
    if (it != mapping.end())
      text = it->second;
    output << text << " ";
    line_chars += text.size();
    if (text.find('\n') != std ::string ::npos) {
      line_chars = cnt = 0;
      continue;
    }
    if (line_chars >= max_line_chars || ++cnt % line_count == 0) {
      line_chars = cnt = 0;
      output << "\n";
    }
  }
}
std ::string normalize_line(const std ::string &s) {
  if (s.empty())
    return "";
  std ::string result;
  bool in_string = false;
  char quote_char = 0;
  bool last_was_space = false;
  size_t start = s.find_first_not_of(" \t\r\n");
  if (start == std ::string ::npos)
    return "";
  size_t end = s.find_last_not_of(" \t\r\n");
  for (size_t i = start; i <= end; ++i) {
    char c = s[i];
    if (in_string) {
      result += c;
      if (c == '\\' && i < end) {
        result += s[++i];
      } else if (c == quote_char) {
        in_string = false;
      }
      last_was_space = false;
      continue;
    }
    if (is_pp_number_start(s, i)) {
      size_t n = scan_pp_number(s, i);
      result.append(s, i, n);
      i += n - 1;
      last_was_space = false;
      continue;
    }
    if (c == '"' || c == '\'') {
      in_string = true;
      quote_char = c;
      result += c;
      last_was_space = false;
      continue;
    }
    if (std ::isspace(static_cast<unsigned char>(c))) {
      if (!last_was_space) {
        result += ' ';
        last_was_space = true;
      }
    } else {
      result += c;
      last_was_space = false;
    }
  }
  return result;
}
struct LinePart {
  std ::string text;
  bool verbatim;
};
// Update the parenthesis nesting depth of code (outside literals). Used to
// keep lines taking part in a multi-line function-like macro invocation
// verbatim, so the preprocessor still sees real "(", "," and ")" tokens.
void count_parens(const std ::string &line, size_t start, int &depth) {
  size_t i = start;
  while (i < line.size()) {
    bool is_raw = false;
    size_t prefix = literal_prefix(line, i, is_raw);
    if (prefix != std ::string ::npos) {
      size_t quote_pos = i + prefix;
      if (is_raw) {
        size_t paren = line.find('(', quote_pos + 1);
        if (paren != std ::string ::npos) {
          std ::string delim =
              line.substr(quote_pos + 1, paren - quote_pos - 1);
          std ::string end_marker = raw_end_marker(delim);
          size_t close = line.find(end_marker, paren + 1);
          if (close == std ::string ::npos)
            return;
          i = close + end_marker.size();
          i += scan_identifier(line, i);
          continue;
        }
        i += prefix;
        continue;
      }
      size_t lit_len = scan_quoted_literal(line, quote_pos);
      i = quote_pos + lit_len;
      continue;
    }
    if (is_pp_number_start(line, i)) {
      i += scan_pp_number(line, i);
      continue;
    }
    if (line.compare(i, 2, "//") == 0)
      return;
    if (line[i] == '(')
      depth++;
    else if (line[i] == ')')
      depth--;
    i++;
  }
}
// If the text contains a raw string that does not close on this line, set
// delim and return true. Closed raw strings are scanned past.
bool find_unclosed_raw(const std ::string &line, std ::string &delim) {
  size_t i = 0;
  while (i < line.size()) {
    bool is_raw = false;
    size_t prefix = literal_prefix(line, i, is_raw);
    if (prefix != std ::string ::npos) {
      size_t quote_pos = i + prefix;
      if (is_raw) {
        size_t paren = line.find('(', quote_pos + 1);
        if (paren != std ::string ::npos) {
          std ::string d =
              line.substr(quote_pos + 1, paren - quote_pos - 1);
          std ::string end_marker = raw_end_marker(d);
          size_t close = line.find(end_marker, paren + 1);
          if (close == std ::string ::npos) {
            delim = d;
            return true;
          }
          i = close + end_marker.size();
          i += scan_identifier(line, i);
          continue;
        }
        i += prefix;
        continue;
      }
      size_t lit_len = scan_quoted_literal(line, quote_pos);
      i = quote_pos + lit_len;
      continue;
    }
    if (is_pp_number_start(line, i)) {
      i += scan_pp_number(line, i);
      continue;
    }
    if (line.compare(i, 2, "//") == 0)
      return false;
    i++;
  }
  return false;
}
void obfuscate_line(std ::ifstream &input, std ::ofstream &output,
                    const std ::vector<std ::string> &chars, std ::string head,
                    int line_count) {
  std ::vector<std ::string> preprocessors;
  std ::vector<LinePart> parts;
  std ::string line;
  std ::string current_raw_block;
  bool in_raw_string = false;
  std ::string raw_delim;
  int paren_depth = 0;
  while (std ::getline(input, line)) {
    if (in_raw_string) {
      std ::string end_marker = raw_end_marker(raw_delim);
      size_t close = line.find(end_marker);
      if (close == std ::string ::npos) {
        current_raw_block += "\n" + line;
        continue;
      }
      size_t after = close + end_marker.size();
      after += scan_identifier(line, after);
      current_raw_block += "\n" + line.substr(0, after);
      parts.push_back({current_raw_block, true});
      current_raw_block.clear();
      in_raw_string = false;
      std ::string tail = line.substr(after);
      count_parens(tail, 0, paren_depth);
      if (paren_depth < 0)
        paren_depth = 0;
      std ::string tail_delim;
      if (find_unclosed_raw(tail, tail_delim)) {
        // The closing line starts another raw string that runs on.
        in_raw_string = true;
        raw_delim = tail_delim;
        current_raw_block = tail;
      } else if (!tail.empty()) {
        parts.push_back({tail, true});
      }
      continue;
    }
    size_t first = line.find_first_not_of(" \t");
    if (first != std ::string ::npos && line[first] == '#') {
      std ::string combined = line;
      while (!combined.empty() && combined.back() == '\\') {
        combined.pop_back();
        std ::string next_line;
        if (!std ::getline(input, next_line))
          break;
        combined += " " + next_line;
      }
      preprocessors.push_back(combined);
      continue;
    }
    int start_depth = paren_depth;
    count_parens(line, 0, paren_depth);
    if (paren_depth < 0)
      paren_depth = 0;
    bool unbalanced = start_depth != 0 || paren_depth != 0;
    // A line whose first token is "(" may be the continuation of a
    // function-like macro invocation started at the end of the previous line;
    // the "(" has to stay a real token for the preprocessor to see it.
    bool starts_with_lparen = first != std ::string ::npos && line[first] == '(';
    bool has_raw = false;
    size_t i = 0;
    while (i < line.size()) {
      bool is_raw_start = false;
      size_t prefix = literal_prefix(line, i, is_raw_start);
      if (prefix != std ::string ::npos) {
        size_t quote_pos = i + prefix;
        if (is_raw_start) {
          size_t paren = line.find('(', quote_pos + 1);
          if (paren != std ::string ::npos) {
            std ::string delim =
                line.substr(quote_pos + 1, paren - quote_pos - 1);
            std ::string end_marker = raw_end_marker(delim);
            size_t close = line.find(end_marker, paren + 1);
            has_raw = true;
            if (close == std ::string ::npos) {
              in_raw_string = true;
              raw_delim = delim;
              current_raw_block = line;
              break;
            }
            // Closed on this line: keep scanning the rest of the line.
            i = close + end_marker.size();
            i += scan_identifier(line, i);
            continue;
          }
          i += prefix;
          continue;
        }
        size_t lit_len = scan_quoted_literal(line, quote_pos);
        i = quote_pos + lit_len;
        continue;
      }
      if (is_pp_number_start(line, i)) {
        i += scan_pp_number(line, i);
        continue;
      }
      if (line.compare(i, 2, "//") == 0) {
        line = line.substr(0, i);
        break;
      }
      i++;
    }
    if (has_raw || unbalanced || starts_with_lparen) {
      if (!in_raw_string)
        parts.push_back({line, true});
      continue;
    }
    std ::string clean_line = normalize_line(line);
    if (!clean_line.empty())
      parts.push_back({clean_line, false});
  }
  if (in_raw_string)
    parts.push_back({current_raw_block, true});
  if (!head.empty())
    output << head << "\n";
  for (const auto &p : preprocessors)
    output << p << "\n";
  int cnt = 0;
  long long line_chars = 0;
  if (no_chars) {
    for (const auto &part : parts) {
      output << part.text << " ";
      line_chars += part.text.size();
      if (part.verbatim || part.text.find('\n') != std ::string ::npos) {
        output << "\n";
        line_chars = cnt = 0;
        continue;
      }
      if (line_chars >= max_line_chars || ++cnt % line_count == 0) {
        line_chars = cnt = 0;
        output << "\n";
      }
    }
    return;
  }
  std ::unordered_set<std ::string> unique_items;
  for (const auto &part : parts)
    if (!part.verbatim)
      unique_items.insert(part.text);
  std ::unordered_set<std ::string> forbidden;
  for (const auto &part : parts)
    collect_identifiers(part.text, forbidden);
  for (const auto &p : preprocessors)
    collect_identifiers(p, forbidden);
  collect_identifiers(head, forbidden);
  auto mapping = create_mapping(unique_items, chars, forbidden);
  for (const auto &m : mapping)
    output << "#define " << m.second << " " << m.first << "\n";
  for (const auto &part : parts) {
    if (part.verbatim) {
      output << part.text << "\n";
      line_chars = cnt = 0;
      continue;
    }
    std ::string text = mapping[part.text];
    output << text << " ";
    line_chars += text.size();
    if (line_chars >= max_line_chars || ++cnt % line_count == 0) {
      line_chars = cnt = 0;
      output << "\n";
    }
  }
}
struct ScanState {
  bool block_comment = false;
  bool raw = false;
  std ::string raw_delim;
};
// Track comments and raw strings across physical lines so a #include that only
// appears inside literal text is not mistaken for a directive.
void scan_state(const std ::string &line, ScanState &st) {
  size_t i = 0;
  while (i < line.size()) {
    if (st.block_comment) {
      size_t close = line.find("*/", i);
      if (close == std ::string ::npos)
        return;
      st.block_comment = false;
      i = close + 2;
      continue;
    }
    if (st.raw) {
      std ::string end_marker = ")" + st.raw_delim + "\"";
      size_t close = line.find(end_marker, i);
      if (close == std ::string ::npos)
        return;
      st.raw = false;
      i = close + end_marker.size();
      continue;
    }
    if (line.compare(i, 2, "//") == 0)
      return;
    if (line.compare(i, 2, "/*") == 0) {
      st.block_comment = true;
      i += 2;
      continue;
    }
    if (is_pp_number_start(line, i)) {
      i += scan_pp_number(line, i);
      continue;
    }
    bool is_raw = false;
    size_t prefix = literal_prefix(line, i, is_raw);
    if (prefix != std ::string ::npos) {
      size_t quote_pos = i + prefix;
      if (is_raw) {
        size_t paren = line.find('(', quote_pos + 1);
        if (paren != std ::string ::npos) {
          st.raw_delim = line.substr(quote_pos + 1, paren - quote_pos - 1);
          std ::string end_marker = raw_end_marker(st.raw_delim);
          size_t close = line.find(end_marker, paren + 1);
          if (close == std ::string ::npos) {
            st.raw = true;
            return;
          }
          i = close + end_marker.size();
          continue;
        }
        i += prefix;
        continue;
      }
      size_t lit_len = scan_quoted_literal(line, quote_pos);
      i = quote_pos + lit_len;
      continue;
    }
    i++;
  }
}
size_t skip_space_comments(const std ::string &line, size_t i) {
  while (i < line.size()) {
    if (line[i] == ' ' || line[i] == '\t') {
      i++;
      continue;
    }
    if (line.compare(i, 2, "/*") == 0) {
      size_t close = line.find("*/", i + 2);
      if (close == std ::string ::npos)
        return std ::string ::npos;
      i = close + 2;
      continue;
    }
    if (line.compare(i, 2, "//") == 0)
      return std ::string ::npos;
    break;
  }
  return i;
}
// Returns the position of the '#' of an include directive on this line, or
// npos. in_block_comment handles a comment opened on an earlier line; the
// text before the '#' is kept in the temp file so the comment still closes.
size_t find_include_directive(const std ::string &line,
                              bool in_block_comment) {
  size_t i = 0;
  if (in_block_comment) {
    size_t close = line.find("*/");
    if (close == std ::string ::npos)
      return std ::string ::npos;
    i = close + 2;
  }
  i = skip_space_comments(line, i);
  if (i == std ::string ::npos || i >= line.size() || line[i] != '#')
    return std ::string ::npos;
  size_t hash_pos = i;
  i = skip_space_comments(line, i + 1);
  if (i == std ::string ::npos || i >= line.size())
    return std ::string ::npos;
  size_t begin = i;
  while (i < line.size() && is_id_char(line[i]))
    i++;
  std ::string name = line.substr(begin, i - begin);
  if (name == "include" || name == "include_next")
    return hash_pos;
  return std ::string ::npos;
}
// Position just past the operand of an include directive (the <...>, "...",
// or macro name), so comments inside the operand are not mistaken for a
// trailing block comment.
size_t include_operand_end(const std ::string &line, size_t hash_pos) {
  size_t i = hash_pos + 1;
  while (i < line.size() && (line[i] == ' ' || line[i] == '\t'))
    i++;
  while (i < line.size() && is_id_char(line[i]))
    i++;
  while (i < line.size() && (line[i] == ' ' || line[i] == '\t'))
    i++;
  if (i >= line.size())
    return line.size();
  if (line[i] == '<') {
    size_t end = line.find('>', i + 1);
    return end == std ::string ::npos ? line.size() : end + 1;
  }
  if (line[i] == '"') {
    size_t end = line.find('"', i + 1);
    return end == std ::string ::npos ? line.size() : end + 1;
  }
  while (i < line.size() && is_id_char(line[i]))
    i++;
  return i;
}
// If line[from..] ends inside a block comment, return the position of the
// unmatched "/*"; otherwise npos. A "//" starts a line comment.
size_t unclosed_block_comment_start(const std ::string &line, size_t from) {
  size_t line_comment = line.find("//", from);
  size_t limit =
      line_comment == std ::string ::npos ? line.size() : line_comment;
  size_t open = std ::string ::npos;
  size_t p = from;
  while (p < limit) {
    size_t o = line.find("/*", p);
    if (o == std ::string ::npos || o >= limit)
      break;
    size_t c = line.find("*/", o + 2);
    if (c == std ::string ::npos || c >= limit) {
      open = o;
      break;
    }
    p = c + 2;
    open = std ::string ::npos;
  }
  return open;
}
std ::ifstream expand_file(std ::string file_path, std ::string out_path) {
  std ::ifstream code_file(file_path);
  if (!code_file.is_open()) {
    std ::cout << "Code file open failed!" << std ::endl;
    std ::cout << file_path << std ::endl;
    exit(21);
  }
  std ::vector<std ::string> lines;
  std ::string code_line;
  while (std ::getline(code_file, code_line))
    lines.push_back(code_line);
  if (code_file.bad() || (!code_file.eof() && code_file.fail())) {
    std ::cout << "Code file read failed!" << std ::endl;
    std ::cout << file_path << std ::endl;
    exit(21);
  }
  code_file.close();
  if (!lines.empty() && lines[0].size() >= 3 &&
      static_cast<unsigned char>(lines[0][0]) == 0xEF &&
      static_cast<unsigned char>(lines[0][1]) == 0xBB &&
      static_cast<unsigned char>(lines[0][2]) == 0xBF)
    lines[0].erase(0, 3);
  std ::string marker_base = "__OBFUSCATOR_INCLUDE_";
  for (;;) {
    bool clash = false;
    for (const auto &line : lines)
      if (line.find(marker_base) != std ::string ::npos) {
        clash = true;
        break;
      }
    if (!clash)
      break;
    marker_base += "X";
  }
  std ::ofstream tmp_file(out_path + ".tmp");
  if (!tmp_file.is_open()) {
    std ::cout << "Tmp file open failed!" << std ::endl;
    std ::cout << out_path + ".tmp" << std ::endl;
    exit(22);
  }
  std ::vector<std ::string> includes;
  ScanState state;
  bool continued = false;
  for (const auto &line : lines) {
    size_t hash_pos =
        (!state.raw && !continued)
            ? find_include_directive(line, state.block_comment)
            : std ::string ::npos;
    if (hash_pos != std ::string ::npos) {
      std ::string marker =
          marker_base + std ::to_string(includes.size()) + "__";
      std ::string directive = line.substr(hash_pos);
      std ::string tail;
      // A block comment opened after the operand must stay in the temp file,
      // otherwise the lines it comments out become active code there. It is
      // not restored, since -E strips the comment from the output.
      size_t open =
          unclosed_block_comment_start(line, include_operand_end(line, hash_pos));
      if (open != std ::string ::npos) {
        directive = line.substr(hash_pos, open - hash_pos);
        tail = line.substr(open);
      }
      // Keep the comment/whitespace prefix in the preprocessed input; only the
      // directive itself is restored afterwards.
      tmp_file << line.substr(0, hash_pos) << "\n\"" << marker << "\"";
      if (!tail.empty())
        tmp_file << " " << tail;
      tmp_file << "\n";
      includes.push_back(directive);
    } else {
      tmp_file << line << "\n";
    }
    scan_state(line, state);
    continued = !line.empty() && line.back() == '\\';
  }
  tmp_file.close();
  if (tmp_file.fail()) {
    std ::cout << "Tmp file write failed!" << std ::endl;
    std ::cout << out_path + ".tmp" << std ::endl;
    exit(22);
  }
  std ::string raw_path = out_path + ".raw";
  std ::string exp_path = out_path + ".exp";
  std ::string rm_tmp = rm_command(out_path + ".tmp");
  std ::string rm_raw = rm_command(raw_path);
  std ::string rm_exp = rm_command(exp_path);
  int res = system(gpp_command(out_path + ".tmp", raw_path).data());
  if (!res) {
    std ::ifstream raw_file(raw_path);
    if (!raw_file.is_open()) {
      res = 1;
    } else {
      std ::ofstream exp_file(exp_path);
      if (!exp_file.is_open()) {
        std ::cout << "Exp file open failed!" << std ::endl;
        std ::cout << exp_path << std ::endl;
        system(rm_tmp.data());
        system(rm_raw.data());
        exit(23);
      }
      std::string line;
      while (std::getline(raw_file, line)) {
        // Only a line consisting solely of the marker token is restored, so a
        // user macro that happens to expand to the marker text cannot inject
        // an include directive into the middle of the output.
        std::string restored;
        size_t first = line.find_first_not_of(" \t\r");
        if (first != std::string::npos) {
          size_t last = line.find_last_not_of(" \t\r");
          std::string token = line.substr(first, last - first + 1);
          for (size_t k = 0; k < includes.size(); k++) {
            std::string marker =
                "\"" + marker_base + std::to_string(k) + "__\"";
            if (token == marker) {
              restored = includes[k];
              break;
            }
          }
        }
        exp_file << (restored.empty() ? line : restored) << "\n";
      }
      if (raw_file.bad() || (!raw_file.eof() && raw_file.fail()))
        res = 1;
      exp_file.close();
      if (exp_file.fail())
        res = 1;
    }
  }
  res |= system(rm_tmp.data());
  res |= system(rm_raw.data());
  if (res) {
    system(rm_exp.data());
    std ::cout << "Preprocessing failed!" << std ::endl;
    exit(24);
  }
  return std::ifstream(exp_path);
}
bool parse_count(const std ::string &str, long long &out) {
  if (str.empty())
    return false;
  long long res = 0;
  for (char c : str) {
    if (c < '0' || c > '9')
      return false;
    int digit = c - '0';
    if (res > (LLONG_MAX - digit) / 10)
      return false;
    res = res * 10 + digit;
  }
  out = res;
  return true;
}
void chars_push(std ::vector<std ::string> &chars, std ::string using_chars) {
  for (size_t i = 0; i < using_chars.size(); i++) {
    while (i < using_chars.size() && using_chars[i] == ' ')
      i++;
    std ::string str;
    while (i < using_chars.size() && using_chars[i] != ' ')
      str.push_back(using_chars[i++]);
    if (str.size())
      chars.push_back(str);
  }
}
bool valid_symbols(const std ::vector<std ::string> &chars) {
  if (chars.size() < 2)
    return false;
  std ::unordered_set<std ::string> seen;
  // Equal-length symbols make concatenation injective: splitting an alias into
  // fixed-size chunks recovers the symbol sequence, so two different indices
  // can never produce the same alias.
  size_t symbol_len = chars[0].size();
  for (const auto &s : chars) {
    if (s.empty() || s.size() != symbol_len || !is_id_start_char(s[0]))
      return false;
    for (char c : s)
      if (!is_id_char(c))
        return false;
    if (!seen.insert(s).second)
      return false;
  }
  return true;
}
int main(int argc, char **argv) {
  std ::string file_path, out_path;
  std ::string head, using_chars = "OOO OO1 O1O O11", using_chars_x[2];
  std ::vector<std ::string> chars, chars_x[2];
  int mode = 0;
  int line_count = 7;
  for (int i = 1; i < argc; i++) {
    std ::string arg = argv[i];
    if (!arg.empty() && arg[0] == '-') {
      if (arg.size() > 2)
        return 11;
      switch (arg[1]) {
      case 's':
        if (++i >= argc)
          return 12;
        using_chars = argv[i];
        break;
      case 'h':
        if (++i >= argc)
          return 13;
        {
          std ::ifstream head_file(argv[i]);
          std ::string line;
          if (!head_file.is_open()) {
            std ::cout << "head file open failed!" << std ::endl;
            std ::cout << argv[i] << std ::endl;
            return 14;
          }
          while (std ::getline(head_file, line))
            head += line + '\n';
          if (head_file.bad() || (!head_file.eof() && head_file.fail())) {
            std ::cout << "head file read failed!" << std ::endl;
            std ::cout << argv[i] << std ::endl;
            return 14;
          }
        }
        break;
      case 'l':
        mode = 1;
        break;
      case 'm':
        if (++i >= argc)
          return 15;
        {
          long long value = 0;
          if (!parse_count(argv[i], value) || value < 1 || value > INT_MAX)
            return 15;
          line_count = static_cast<int>(value);
        }
        break;
      case 'x':
        if (i + 2 >= argc)
          return 16;
        using_chars_x[0] = argv[++i];
        using_chars_x[1] = argv[++i];
        mode = 2;
        break;
      case 't':
        mode = 3;
        break;
      case 'n':
        no_chars = true;
        break;
      case 'M':
        if (++i >= argc)
          return 17;
        {
          long long value = 0;
          if (!parse_count(argv[i], value) || value < 1)
            return 17;
          max_line_chars = value;
        }
        break;
      default:
        std ::cout << "Unknown command!" << std ::endl;
        return 19;
      }
    } else {
      if (file_path.size() == 0)
        file_path = arg;
      else if (out_path.size() == 0)
        out_path = arg;
      else {
        std ::cout << "Unknown command!" << std ::endl;
        return 18;
      }
    }
  }
  if (file_path.empty() || out_path.empty()) {
    std ::cout << "Input and output paths are required!" << std::endl;
    return 11;
  }
  if (mode < 2) {
    chars_push(chars, using_chars);
    if (!no_chars && !valid_symbols(chars)) {
      std ::cout << "Invalid symbol set!" << std ::endl;
      return 12;
    }
  } else if (mode == 2) {
    chars_push(chars_x[0], using_chars_x[0]);
    chars_push(chars_x[1], using_chars_x[1]);
    if (!no_chars && (!valid_symbols(chars_x[0]) || !valid_symbols(chars_x[1]))) {
      std ::cout << "Invalid symbol set!" << std ::endl;
      return 16;
    }
  }
  std ::ifstream exp_file = expand_file(file_path, out_path);
  if (!exp_file.is_open()) {
    std ::cout << "Exp file open failed!" << std::endl;
    std ::cout << out_path + ".exp" << std::endl;
    return 23;
  }
  std ::ofstream out_file(out_path);
  if (!out_file.is_open()) {
    std ::cout << "Output file open failed!" << std::endl;
    std ::cout << out_path << std::endl;
    system(rm_command(out_path + ".exp").data());
    return 25;
  }
  std ::string rm_exp = rm_command(out_path + ".exp");
  auto cleanup = [&]() {
    out_file.close();
    system(rm_exp.data());
  };
  if (mode < 2) {
    if (mode <= 0)
      obfuscate(exp_file, out_file, chars, head, line_count);
    else
      obfuscate_line(exp_file, out_file, chars, head, line_count);
    exp_file.close();
    if (exp_file.bad()) {
      cleanup();
      return 24;
    }
  } else if (mode == 2) {
    std ::string tmp_path = out_path + ".tmp";
    std ::ofstream tmp_file(tmp_path);
    if (!tmp_file.is_open()) {
      std ::cout << "tmp file open failed!" << std::endl;
      cleanup();
      return 31;
    }
    obfuscate(exp_file, tmp_file, chars_x[0], head, line_count);
    exp_file.close();
    tmp_file.close();
    if (exp_file.bad() || tmp_file.fail()) {
      std ::cout << "tmp file write failed!" << std::endl;
      system(rm_command(tmp_path).data());
      cleanup();
      return 31;
    }
    std ::ifstream in_tmp_file(tmp_path);
    if (!in_tmp_file.is_open()) {
      std ::cout << "tmp file open failed!" << std::endl;
      system(rm_command(tmp_path).data());
      cleanup();
      return 31;
    }
    obfuscate_line(in_tmp_file, out_file, chars_x[1], head, line_count);
    in_tmp_file.close();
    if (in_tmp_file.bad()) {
      system(rm_command(tmp_path).data());
      cleanup();
      return 32;
    }
    if (system(rm_command(tmp_path).data())) {
      cleanup();
      return 32;
    }
  } else if (mode == 3) {
    std ::string line;
    while (getline(exp_file, line))
      out_file << line << '\n';
    exp_file.close();
    if (exp_file.bad()) {
      cleanup();
      return 24;
    }
  }
  out_file.close();
  if (out_file.fail()) {
    system(rm_exp.data());
    return 25;
  }
  if (system(rm_exp.data()))
    return 33;
}
