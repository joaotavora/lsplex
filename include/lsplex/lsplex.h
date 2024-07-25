#pragma once

#include <string>
#include <vector>

#include "lsplex/export.hpp"

namespace lsplex {

LSPLEX_EXPORT class LsContact {
public:
  LsContact(std::string exe, std::vector<std::string> args)
      : _exe{std::move(exe)}, _args{std::move(args)} {}  // NOLINT

  [[nodiscard]] std::vector<std::string> args() const { return _args; };
  [[nodiscard]] std::string exe() const { return _exe; };

private:
  std::string _exe;
  std::vector<std::string> _args;
};

LSPLEX_EXPORT class LsPlex {
  std::vector<LsContact> _contacts;

public:
  explicit LsPlex(std::vector<LsContact> contacts);

  void start();
};

}  // namespace lsplex
