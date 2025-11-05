module;
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstring>
#include <future>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

export module http;
export import core;

export namespace http {

using namespace stl;

enum class EMethod { GET, POST, PUT, DELETE, HEAD, PATCH, OPTIONS };

inline std::string method_to_string(EMethod m) {
  switch (m) {
  case EMethod::GET:
    return "GET";
  case EMethod::POST:
    return "POST";
  case EMethod::PUT:
    return "PUT";
  case EMethod::DELETE:
    return "DELETE";
  case EMethod::HEAD:
    return "HEAD";
  case EMethod::PATCH:
    return "PATCH";
  case EMethod::OPTIONS:
    return "OPTIONS";
  }
  return "GET";
}

struct URL {
  std::string scheme;
  std::string host;
  std::string port;
  std::string path;
  std::string query;

  static result<URL> parse(std::string_view raw_url) {
    URL u;
    size_t pos = 0;
    // Parse scheme
    auto scheme_end = raw_url.find("://");
    if (scheme_end == std::string_view::npos) {
      return make_error<URL>("Invalid URL: missing scheme");
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
      auto path_end = (query_start != std::string_view::npos)
                          ? query_start
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

  std::string full_path() const { return path + query; }
};

struct Headers {
  std::map<std::string, std::string> data;

  void set(std::string_view key, std::string_view value) {
    std::string lower_key(key);
    std::transform(lower_key.begin(), lower_key.end(), lower_key.begin(),
                   ::tolower);
    data[lower_key] = value;
  }

  std::string get(std::string_view key) const {
    std::string lower_key(key);
    std::transform(lower_key.begin(), lower_key.end(), lower_key.begin(),
                   ::tolower);
    auto it = data.find(lower_key);
    return (it != data.end()) ? it->second : "";
  }

  bool has(std::string_view key) const {
    std::string lower_key(key);
    std::transform(lower_key.begin(), lower_key.end(), lower_key.begin(),
                   ::tolower);
    return data.find(lower_key) != data.end();
  }
};

struct Request {
  EMethod method = EMethod::GET;
  URL url;
  Headers headers;
  std::string body;
  std::chrono::milliseconds timeout{30000};

  Request(http::EMethod m, http::URL u) : method(m), url(std::move(u)) {
    headers.set("User-Agent", "cpp-http/1.0");
    headers.set("Accept", "*/*");
  }

  void set_header(std::string_view key, std::string_view value) {
    headers.set(key, value);
  }

  void set_body(std::string data) {
    body = std::move(data);
    if (!headers.has("Content-Length")) {
      headers.set("Content-Length", std::to_string(body.size()));
    }
  }
};

struct Response {
  i32 status_code{0};
  std::string status_text;
  Headers headers;
  std::string body;
  Response() = default;
	Response(i32 code, std::string body_content = "") 
		: status_code(code), body(std::move(body_content)) {
		headers.set("Content-Length", std::to_string(body.size()));
		headers.set("Content-Type", "text/plain");
	}
  bool is_success() const { return status_code >= 200 && status_code < 300; }
};

class Client {
private:
  static result<i32> connect_socket(const URL &u) {
    struct addrinfo hints{}, *res = nullptr;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(u.host.c_str(), u.port.c_str(), &hints, &res) != 0) {
      return make_error<i32>("Failed to resolve host: " + u.host);
    }
    i32 sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
      freeaddrinfo(res);
      return make_error<i32>("Failed to create socket");
    }
    if (connect(sock, res->ai_addr, res->ai_addrlen) < 0) {
      freeaddrinfo(res);
#ifdef _WIN32
      closesocket(sock);
#else
      close(sock);
#endif
      return make_error<i32>("Failed to connect to host");
    }
    freeaddrinfo(res);
    return sock;
  }

  static result<> send_request(i32 sock, const Request &req) {
    std::ostringstream ss;
    ss << method_to_string(req.method) << " " << req.url.full_path()
       << " HTTP/1.1\r\n";
    ss << "Host: " << req.url.host << "\r\n";
    for (const auto &[key, value] : req.headers.data) {
      ss << key << ": " << value << "\r\n";
    }
    ss << "\r\n";
    if (!req.body.empty()) {
      ss << req.body;
    }
    std::string request_str = ss.str();
    size_t sent = 0;
    while (sent < request_str.size()) {
      i32 n = ::send(sock, request_str.c_str() + sent,
                     request_str.size() - sent, 0);
      if (n <= 0) {
        return make_error<>("Failed to send request");
      }
      sent += n;
    }
    return result_success();
  }

  static result<Response> read_response(i32 sock) {
    Response resp;
    std::string buffer;
    char chunk[4096];
    bool headers_done = false;
    size_t content_length = 0;
    bool is_chunked = false;
    while (true) {
      i32 n = recv(sock, chunk, sizeof(chunk), 0);
      if (n <= 0)
        break;
      buffer.append(chunk, n);
      if (!headers_done) {
        auto header_end = buffer.find("\r\n\r\n");
        if (header_end != std::string::npos) {
          std::string header_section = buffer.substr(0, header_end);
          std::istringstream iss(header_section);
          std::string line;
          // Parse status line
          if (std::getline(iss, line)) {
            std::istringstream status_stream(line);
            std::string http_version;
            status_stream >> http_version >> resp.status_code;
            std::getline(status_stream, resp.status_text);
            if (!resp.status_text.empty() && resp.status_text[0] == ' ') {
              resp.status_text.erase(0, 1);
            }
          }
          // Parse headers
          while (std::getline(iss, line) && !line.empty() && line != "\r") {
            if (line.back() == '\r') {
              line.pop_back();
            }
            auto colon = line.find(':');
            if (colon != std::string::npos) {
              auto key = line.substr(0, colon);
              auto value = line.substr(colon + 1);
              if (!value.empty() && value[0] == ' ')
                value.erase(0, 1);
              resp.headers.set(key, value);
            }
          }
          buffer.erase(0, header_end + 4);
          headers_done = true;
          auto cl = resp.headers.get("content-length");
          if (!cl.empty()) {
            content_length = std::stoull(cl);
          }
          auto te = resp.headers.get("transfer-encoding");
          if (te.find("chunked") != std::string::npos) {
            is_chunked = true;
          }
        }
      }
      if (headers_done && !is_chunked && content_length > 0) {
        if (buffer.size() >= content_length) {
          resp.body = buffer.substr(0, content_length);
          break;
        }
      }
    }
    if (!headers_done) {
      return make_error<Response>("Failed to parse response headers");
    }
    if (!is_chunked && content_length == 0 && !buffer.empty()) {
      resp.body = buffer;
    } else if (!is_chunked && content_length > 0) {
      resp.body = buffer.substr(0, content_length);
    }
    return resp;
  }

public:
  static std::future<result<Response>> async_send(Request req) {
    return std::async(std::launch::async,
                      [req = std::move(req)]() -> result<Response> {
                        auto sock_result = connect_socket(req.url);
                        if (!sock_result) {
                          return make_error<Response>(sock_result.error());
                        }
                        i32 sock = sock_result.value();
                        auto send_result = send_request(sock, req);
                        if (!send_result) {
#ifdef _WIN32
                          closesocket(sock);
#else
				close(sock);
#endif
                          return make_error<Response>(send_result.error());
                        }
                        auto resp_result = read_response(sock);
#ifdef _WIN32
                        closesocket(sock);
#else
			close(sock);
#endif
                        return resp_result;
                      });
  }

  static result<Response> send(const Request &req) {
    auto future = async_send(Request(req));
    return future.get();
  }

  static std::future<result<Response>> get(std::string_view url_str) {
    auto url_result = URL::parse(url_str);
    if (!url_result) {
      std::promise<result<Response>> p;
      p.set_value(make_error<Response>(url_result.error()));
      return p.get_future();
    }
    return async_send(Request(EMethod::GET, std::move(url_result.value())));
  }

  static std::future<result<Response>> post(std::string_view url_str,
                                            std::string body) {
    auto url_result = URL::parse(url_str);
    if (!url_result) {
      std::promise<result<Response>> p;
      p.set_value(make_error<Response>(url_result.error()));
      return p.get_future();
    }
    Request req(EMethod::POST, std::move(url_result.value()));
    req.set_body(std::move(body));
    return async_send(std::move(req));
  }
};

} // namespace http