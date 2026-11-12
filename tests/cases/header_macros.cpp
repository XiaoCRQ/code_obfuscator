#include <cassert>
#include <cstdarg>
#include <cstddef>

int sum(int n, ...) {
  va_list ap;
  va_start(ap, n);
  int s = 0;
  for (int i = 0; i < n; i++)
    s += va_arg(ap, int);
  va_end(ap);
  return s;
}

struct S {
  int a;
  int b;
};

int main() {
  S s{1, 2};
  assert(
      s.a == 1);
  assert
      (s.b == 2);
  std::size_t off = offsetof(S, b);
  int total = sum(3, 1,
                  2, 3);
  assert(total == 6);
  return (off == sizeof(int) && total == 6) ? 0 : 1;
}
