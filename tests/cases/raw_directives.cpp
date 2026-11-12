int main() {
  const char *s = R"(
#include <fake.h>
# not a pragma
)";
  return (s[0] == '\n' && s[1] == '#' && s[2] == 'i') ? 0 : 1;
}
