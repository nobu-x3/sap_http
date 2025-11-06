#include "net/http.h"

namespace http {

Request::Request(http::EMethod m, http::URL u) : method(m), url(std::move(u)) {
  // Set default headers for client requests if scheme/host are present
  if (!url.host.empty()) {
    headers.set("User-Agent", "cpp-http/1.0");
    headers.set("Accept", "*/*");
  }
}

void Request::set_header(std::string_view key, std::string_view value) {
  headers.set(key, value);
}

void Request::set_body(std::string data) {
  body = std::move(data);
  if (!headers.has("Content-Length")) {
    headers.set("Content-Length", std::to_string(body.size()));
  }
}

} // namespace http