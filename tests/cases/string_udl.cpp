#include <string>
#include <string_view>

int operator""_c(char c) { return c == 'x' ? 7 : 0; }
unsigned long operator""_r(const char *, unsigned long) { return 1; }

using namespace std::literals;

int main() {
  std::string s = "abc"s;
  std::string_view v = "xyz"sv;
  auto raw = R"(z)"_r;
  return (s.size() == 3 && v.size() == 3 && s[0] == 'a' && v[0] == 'x' &&
          'x'_c == 7 && raw == 1)
             ? 0
             : 1;
}
