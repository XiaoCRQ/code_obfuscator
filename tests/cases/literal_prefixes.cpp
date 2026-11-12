int main() {
  auto s = u8"hi";
  const wchar_t *w = L"wide";
  char16_t a = u'x';
  char32_t b = U'y';
  auto r = u8R"(raw)";
  const wchar_t *q = LR"d(lw)d";
  const char *p = R"tag(plain)tag";
  return (s[0] == u8'h' && w[0] == L'w' && a == u'x' && b == U'y' &&
          r[0] == u8'r' && q[0] == L'l' && p[0] == 'p')
             ? 0
             : 1;
}
