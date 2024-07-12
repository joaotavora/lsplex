#pragma once

#include <string>

#include "greeter/export.hpp"

namespace greeter {

  /**  Language codes to be used with the Greeter class */
  GREETER_EXPORT enum class LanguageCode { en, de, es, fr };

  /**
   * @brief A class for saying hello in multiple languages
   */
  GREETER_EXPORT class Greeter {
    std::string _name;

  public:
    /**
     * @brief Creates a new greeter
     * @param name the name to greet
     */
    explicit Greeter(std::string name);

    /**
     * @brief Creates a localized string containing the greeting
     * @param lang the language to greet in
     * @return a string containing the greeting
     */
    [[nodiscard]] std::string greet(LanguageCode lang = LanguageCode::en) const;
  };

}  // namespace greeter
