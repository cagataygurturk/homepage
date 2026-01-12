#pragma once

#include <optional>
#include <string>

namespace homepage::utils {

class Config {
 public:
  static Config& instance();

  [[nodiscard]] int getPort() const { return port_; }
  [[nodiscard]] std::string getAddress() const { return address_; }

 private:
  Config();

  int port_;
  std::string address_;

  static std::optional<int> getEnvInt(const char* name,
                                      int defaultValue = 8080);
  static std::string getEnvString(
      const char* name, const std::string& defaultValue = "127.0.0.1");
};

}  // namespace homepage::utils