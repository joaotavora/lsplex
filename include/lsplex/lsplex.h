#pragma once

#include <string>

#include "lsplex/export.hpp"

namespace lsplex {

/**  Language codes to be used with the LsPlex class */
LSPLEX_EXPORT enum class LanguageCode { en, de, es, fr };

/**
 * @brief A class for saying hello in multiple languages
 */
LSPLEX_EXPORT class LsPlex {
  std::string _name;

public:
  /**
   * @brief Creates a new lsplex
   * @param name the name to greet
   */
  explicit LsPlex(std::string name);

  /**
   * @brief Creates a localized string containing the greeting
   * @param lang the language to greet in
   * @return a string containing the greeting
   */
  [[nodiscard]] std::string greet(LanguageCode lang = LanguageCode::en) const;
};

}  // namespace lsplex
