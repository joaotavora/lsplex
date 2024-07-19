#pragma once

#include <string>
#include <vector>

#include "lsplex/export.hpp"

namespace lsplex {

LSPLEX_EXPORT class LsInvocation {
public:
  LsInvocation(std::vector<std::string> args) : _args(std::move(args)) {} // NOLINT
  private:
  std::vector<std::string> _args;
};

LSPLEX_EXPORT class LsPlex {
  std::vector<LsInvocation> _invocations;

public:
  explicit LsPlex(std::vector<LsInvocation> invocations);

  void start();

};

}  // namespace lsplex
