#pragma once

#include "result.h"
#include "types.h"
#include <algorithm>
#include <chrono>
#include <future>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
namespace http {

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

inline EMethod string_to_method(std::string_view s) {
  if (s == "GET")
    return EMethod::GET;
  if (s == "POST")
    return EMethod::POST;
  if (s == "PUT")
    return EMethod::PUT;
  if (s == "DELETE")
    return EMethod::DELETE;
  if (s == "HEAD")
    return EMethod::HEAD;
  if (s == "PATCH")
    return EMethod::PATCH;
  if (s == "OPTIONS")
    return EMethod::OPTIONS;
  return EMethod::GET;
}

struct URL {
  std::string scheme;
  std::string host;
  std::string port;
  std::string path;
  std::string query;

  static stl::result<URL> parse(std::string_view raw_url);
  std::string full_path() const { return path + query; }
  static URL from_path(std::string_view path_and_query);
};

struct Headers {
  std::map<std::string, std::string> data;

  void set(std::string_view key, std::string_view value);
  std::string get(std::string_view key) const;
  bool has(std::string_view key) const;
};

struct Request {
  EMethod method = EMethod::GET;
  URL url;
  Headers headers;
  std::string body;
  std::chrono::milliseconds timeout{30000};

  // Optional: route params extracted by server routing (e.g., /users/:id)
  std::map<std::string, std::string> params;

  Request() = default;
  Request(http::EMethod m, http::URL u);

  void set_header(std::string_view key, std::string_view value);
  void set_body(std::string data);
};

struct Response {
  i32 status_code{0};
  std::string status_text;
  Headers headers;
  std::string body;
  Response() = default;
  Response(i32 code, std::string body_content = "");
  inline bool is_success() const {
    return status_code >= 200 && status_code < 300;
  }
};

class Client {
private:
  static stl::result<i32> connect_socket(const URL &u);
  static stl::result<> send_request(i32 sock, const Request &req);
  static stl::result<Response> read_response(i32 sock);

public:
  static std::future<stl::result<Response>> async_send(Request req);
  static stl::result<Response> send(const Request &req);
  static std::future<stl::result<Response>> get(std::string_view url_str);
  static std::future<stl::result<Response>> post(std::string_view url_str,
                                                 std::string body);
};

using RouteHandler = std::function<Response(const Request &)>;

struct Route {
  std::string path;
  EMethod method;
  RouteHandler handler;
  bool is_regex{false};
};

struct ServerConfig {
  i32 server_socket{-1};
  u16 port{8080};
  bool is_multithreaded{false};
};

class Server {
public:
  Server() = default;
  Server(ServerConfig cfg);
  ~Server();
  stl::result<> start();
  void run();
  void stop();

  template <typename Handler>
  void route(std::string_view path, EMethod method, Handler &&handler) {
    Route r;
    r.path = path;
    r.method = method;
    r.handler = std::forward<Handler>(handler);
    m_Routes.push_back(std::move(r));
  }

private:
  void handle_client(i32 client_socket);

private:
  ServerConfig m_Config;
  std::vector<Route> m_Routes;
  std::atomic<bool> m_IsRunning{false};
  std::vector<std::thread> m_WorkerThreads;
};

} // namespace http