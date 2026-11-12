#include <cstdio>
#ifdef NEVER_DEFINED
#error should not happen
#endif
#define LOCAL_MACRO 7
#pragma GCC diagnostic ignored "-Wunused-variable"
int main() {
  int x = LOCAL_MACRO;
  return x == 7 ? 0 : 1;
}
