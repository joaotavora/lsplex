#include <doctest/doctest.h>
#include <fmt/core.h>
#include <lsplex/lsplex.h>
#include <lsplex/version.h>
#include <windows.h>

#include <fstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <sstream>

TEST_CASE("Lsplex version") {
  static_assert(std::string_view(LSPLEX_VERSION) == std::string_view("1.0.0"));
  CHECK(std::string(LSPLEX_VERSION) == std::string("1.0.0"));
}

struct redirector {
  redirector(const redirector &) = delete;
  redirector(redirector &&) = delete;
  redirector &operator=(const redirector &) = delete;
  redirector &operator=(redirector &&) = delete;
  explicit redirector(const std::string &inputFile)
      : orig_stdin{GetStdHandle(STD_INPUT_HANDLE)},
        orig_stdout{GetStdHandle(STD_OUTPUT_HANDLE)},
        file{CreateFile(inputFile.c_str(), GENERIC_READ, 0, nullptr,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)} {
    if (file == INVALID_HANDLE_VALUE)
      throw std::runtime_error("Failed to open input file");

    if (!SetStdHandle(STD_INPUT_HANDLE, file))
      throw std::runtime_error("Failed to redirect stdin");

    SECURITY_ATTRIBUTES saAttr = {sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    if (!CreatePipe(&read_pipe, &write_pipe, &saAttr, 0))
      throw std::runtime_error("Failed to create pipe");

    if (!SetStdHandle(STD_OUTPUT_HANDLE, write_pipe))
      throw std::runtime_error("Failed to redirect stdout");
  }

  ~redirector() {
    // Restore the original stdin and stdout handles
    SetStdHandle(STD_INPUT_HANDLE, orig_stdin);
    SetStdHandle(STD_OUTPUT_HANDLE, orig_stdout);

    // Close the handles
    CloseHandle(file);
    CloseHandle(write_pipe);
    CloseHandle(read_pipe);
  }

  std::string slurp() {
    // Close the write end of the pipe to signal EOF to the read end
    CloseHandle(write_pipe);
    write_pipe = INVALID_HANDLE_VALUE;

    // Read the contents of the read end of the pipe
    DWORD bytesRead = 0;
    std::vector<char> buffer(1024);
    std::string output;
    while (
        ReadFile(read_pipe, buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, nullptr)
        && bytesRead > 0) {
      output.append(buffer.data(), bytesRead);
    }

    return output;
  }

private:
  HANDLE orig_stdin{};
  HANDLE orig_stdout{};
  HANDLE file{};
  HANDLE read_pipe{};
  HANDLE write_pipe{};
};

TEST_CASE("Loop a single JSON message fully through LsPlex") {

  std::stringstream buffer;
  {
    std::ifstream file{"resources/well_behaved_messages.txt", std::ios::binary};
    buffer << file.rdbuf();
  }
  redirector r{"resources/well_behaved_messages.txt"};
  std::thread th{[]{
    lsplex::LsContact contact{"./wincat.exe", {}};
    lsplex::LsPlex plex{{contact}};
    plex.start();
  }};
  th.join();
  auto slurped = r.slurp();
  CHECK(slurped.size() == buffer.str().size());
  CHECK(slurped == buffer.str());
}
