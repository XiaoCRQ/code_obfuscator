#include <cstddef>
#if 0
#include <definitely_not_a_real_header_xyz.h>
#endif
int main() {
  return sizeof(std::size_t) >= 4 ? 0 : 1;
}
