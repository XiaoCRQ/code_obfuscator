#include <cstddef>
int main() {
  int x = 1 + \
          2;
  /* multi
     line comment */
  const char *s = "abc\
#include <fake.h>";
  return (x == 3 && s[3] == '#' && sizeof(std::size_t) > 0) ? 0 : 1;
}
