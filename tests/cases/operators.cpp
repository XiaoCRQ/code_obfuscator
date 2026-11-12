#include <compare>
#include <map>
#include <vector>

struct B {
  int m;
};
struct C {
  int f() { return 5; }
};

int main() {
  B b{};
  int B::*pm = &B::m;
  b.*pm = 4;
  int (C::*mf)() = &C::f;
  C c;
  C *pc = &c;
  auto cmp = (1 <=> 2);
  std::vector<std::vector<int>> v;
  v.push_back({1, 2});
  std::map<int, std::vector<int>> m;
  m[0] = v[0];
  return (b.*pm == 4 && (c.*mf)() == 5 && (pc->*mf)() == 5 && cmp < 0 &&
          m[0][1] == 2)
             ? 0
             : 1;
}
