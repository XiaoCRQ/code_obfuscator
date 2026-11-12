#include <cstdarg>

template <class... Ts> int count(Ts...) { return sizeof...(Ts); }
template <class... Ts> int add(Ts... xs) { return (xs + ...); }

int sum(int n, ...) {
  va_list ap;
  va_start(ap, n);
  int s = 0;
  for (int i = 0; i < n; i++)
    s += va_arg(ap, int);
  va_end(ap);
  return s;
}

int main() {
  int fold = add(1, 2, 3);
  return (sum(3, 1, 2, 3) == 6 && count(1, 2, 3) == 3 && fold == 6) ? 0 : 1;
}
