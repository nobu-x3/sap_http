#include "net/http.h"

namespace http {

URL URL::from_path(std::string_view path_and_query) {
  URL u;
  auto query_pos = path_and_query.find('?');
  if (query_pos != std::string_view::npos) {
    u.path = path_and_query.substr(0, query_pos);
    u.query = path_and_query.substr(query_pos);
  } else {
    u.path = path_and_query;
  }
  return u;
}

stl::result<URL> URL::parse(std::string_view raw_url) {
  URL u;
  size_t pos = 0;
  // Parse scheme
  auto scheme_end = raw_url.find("://");
  if (scheme_end == std::string_view::npos) {
    return stl::make_error<URL>("Invalid URL: missing scheme");
  }
  u.scheme = raw_url.substr(0, scheme_end);
  pos = scheme_end + 3;
  // Parse host and optional port
  auto path_start = raw_url.find('/', pos);
  auto query_start = raw_url.find('?', pos);
  auto host_end = std::min(path_start, query_start);
  if (host_end == std::string_view::npos) {
    host_end = raw_url.length();
  }
  auto host_port = raw_url.substr(pos, host_end - pos);
  auto port_pos = host_port.find(':');
  if (port_pos != std::string_view::npos) {
    u.host = host_port.substr(0, port_pos);
    u.port = host_port.substr(port_pos + 1);
  } else {
    u.host = host_port;
    u.port = (u.scheme == "https") ? "443" : "80";
  }
  // Parse path
  if (path_start != std::string_view::npos) {
    auto path_end = (query_start != std::string_view::npos) ? query_start
                                                            : raw_url.length();
    u.path = raw_url.substr(path_start, path_end - path_start);
  } else {
    u.path = "/";
  }
  // Parse query
  if (query_start != std::string_view::npos) {
    u.query = raw_url.substr(query_start);
  }
  return u;
}
} // namespace http