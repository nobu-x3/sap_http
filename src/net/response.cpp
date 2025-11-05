#include "net/http.h"

namespace http {
Response::Response(i32 code, std::string body_content)
    : status_code(code), body(std::move(body_content)) {
  headers.set("Content-Length", std::to_string(body.size()));
  headers.set("Content-Type", "text/plain");
}
} // namespace http