  #include <cstddef>
/* single-line comment */ #include <cstdint>
/*
 * block comment continued
 */ #include <cstdarg>
int main() {
  va_list ap;
  (void)ap;
  return (sizeof(std::size_t) >= 4 && sizeof(std::int32_t) == 4) ? 0 : 1;
}
