#include <windows.h>

#include <array>
#include <cstdio>

int main() {
  HANDLE h_in = GetStdHandle(STD_INPUT_HANDLE);
  HANDLE h_out = GetStdHandle(STD_OUTPUT_HANDLE);
  std::array<char, 4096> buf{};

  while (true) {
    DWORD bread = 0;
    BOOL res = ReadFile(h_in, buf.data(), buf.size(), &bread, nullptr);
    if (!static_cast<bool>(res) || bread == 0) break;
    auto to_write = bread;
    while (to_write > 0) {
      DWORD written = 0;
      WriteFile(h_out, buf.data() + written, to_write - written, &written,
                nullptr);
      to_write -= written;
    }
  }

  return 0;
}
