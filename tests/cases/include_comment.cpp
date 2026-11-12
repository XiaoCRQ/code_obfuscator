#include <cstddef> /* trailing comment
#error this line is inside the comment
*/
int main() {
  return sizeof(std::size_t) >= 4 ? 0 : 1;
}
