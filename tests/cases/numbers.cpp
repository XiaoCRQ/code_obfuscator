int main() {
  double a = 1.009;
  double b = .5f;
  double c = 5.;
  double d = 1e-9;
  double e = 0x1p-3;
  double f = 0x1.8p+3f;
  double g = 1.e+5;
  int h = 1'000'000;
  unsigned i = 0b1010'1010u;
  unsigned j = 0755;
  long long k = 1ll;
  return (a == 1.009 && b == 0.5 && c == 5.0 && d > 0 && e > 0 &&
          f == 12.0 && g == 100000.0 && h == 1000000 && i == 170u &&
          j == 493 && k == 1)
             ? 0
             : 1;
}
