int main() {
  auto s = R"(line1
line2)";
  return (s[0] == 'l' && s[4] == '1' && s[5] == '\n' && s[6] == 'l') ? 0 : 1;
}
