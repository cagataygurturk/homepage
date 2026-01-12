#include "utils/config.hpp"

#include <charconv>
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace homepage::utils {

Config& Config::instance() {
  static Config instance;
  return instance;
}

Config::Config() {
  port_ = getEnvInt("PORT", 8080).value_or(8080);
  address_ = getEnvString("ADDRESS", "::");
}

std::optional<int> Config::getEnvInt(const char* name, int defaultValue) {
  const char* env = std::getenv(name);
  if (env == nullptr) {
    return defaultValue;
  }

  int result;
  auto [ptr, ec] = std::from_chars(env, env + std::strlen(env), result);
  if (ec == std::errc{} && ptr == env + std::strlen(env)) {
    return result;
  }

  std::cerr << "Warning: Invalid value for " << name << ": " << env
            << ". Using default: " << defaultValue << std::endl;
  return defaultValue;
}

std::string Config::getEnvString(const char* name,
                                 const std::string& defaultValue) {
  const char* env = std::getenv(name);
  return env ? std::string(env) : defaultValue;
}

}  // namespace homepage::utils