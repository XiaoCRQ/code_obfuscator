#include <cstddef>
int main() {
  auto a = R"(x
)"; auto b = R"(
#include <fake.h>
)";
  auto c = R"(y)"; auto d = R"(
# not a pragma
)";
  return (a[0] == 'x' && b[0] == '\n' && c[0] == 'y' && d[0] == '\n' &&
          sizeof(std::size_t) > 0)
             ? 0
             : 1;
}
