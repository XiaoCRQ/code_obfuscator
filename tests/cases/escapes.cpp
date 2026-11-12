int main() {
  const char *s = "a\\";
  const char *t = "q\\\"z";
  char c = '\\';
  char d = '\'';
  char e = 'q';
  return (s[0] == 'a' && s[1] == '\\' && s[2] == 0 && t[0] == 'q' &&
          t[1] == '\\' && t[2] == '"' && t[3] == 'z' && c == '\\' &&
          d == '\'' && e == 'q')
             ? 0
             : 1;
}
