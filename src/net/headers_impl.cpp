#include "net/http.h"

namespace http {

void Headers::set(std::string_view key, std::string_view value) {
  std::string lower_key(key);
  std::transform(lower_key.begin(), lower_key.end(), lower_key.begin(),
                 ::tolower);
  data[lower_key] = value;
}

std::string Headers::get(std::string_view key) const {
  std::string lower_key(key);
  std::transform(lower_key.begin(), lower_key.end(), lower_key.begin(),
                 ::tolower);
  auto it = data.find(lower_key);
  return (it != data.end()) ? it->second : "";
}

bool Headers::has(std::string_view key) const {
  std::string lower_key(key);
  std::transform(lower_key.begin(), lower_key.end(), lower_key.begin(),
                 ::tolower);
  return data.find(lower_key) != data.end();
}
} // namespace http