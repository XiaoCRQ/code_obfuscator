int main() {
  auto s = R"(hi)"; int x = 1;
  return (s[0] == 'h' && x == 1) ? 0 : 1;
}
