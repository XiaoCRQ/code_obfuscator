#include <vector>
namespace ns {
struct S {
  int v;
};
} // namespace ns
int main() {
  int a<:2:> = <%1, 2%>;
  std::vector<::ns::S> v;
  v.push_back(ns::S{3});
  return (a[1] == 2 && v[0].v == 3) ? 0 : 1;
}
