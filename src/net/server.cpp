#include "net/http.h"
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace http {

static stl::result<ServerRequest>
parse_request(const std::string &raw_request) {
  std::istringstream stream(raw_request);
  std::string line;
  if (!std::getline(stream, line)) {
    return stl::make_error<ServerRequest>("Empty request");
  }
  std::istringstream first_line(line);
  std::string method_str, path_str, version;
  first_line >> method_str >> path_str >> version;
  auto method = string_to_method(method_str);
  std::string path = path_str;
  std::string query;
  auto query_pos = path_str.find('?');
  if (query_pos != std::string::npos) {
    path = path_str.substr(0, query_pos);
    query = path_str.substr(query_pos + 1);
  }
  ServerRequest req(method, path);
  req.query = query;
  while (std::getline(stream, line) && line != "\r" && !line.empty()) {
    if (line.back() == '\r')
      line.pop_back();
    auto colon = line.find(':');
    if (colon != std::string::npos) {
      auto key = line.substr(0, colon);
      auto value = line.substr(colon + 1);
      if (!value.empty() && value[0] == ' ')
        value.erase(0, 1);
      req.headers.set(key, value);
    }
  }
  std::string body_content;
  std::string body_line;
  while (std::getline(stream, body_line)) {
    body_content += body_line + "\n";
  }
  if (!body_content.empty() && body_content.back() == '\n') {
    body_content.pop_back();
  }
  req.body = body_content;
  return req;
}

static std::string build_response(const Response &resp) {
  std::ostringstream ss;
  ss << "HTTP/1.1 " << resp.status_code << " ";
  switch (resp.status_code) {
  case 200:
    ss << "OK";
    break;
  case 201:
    ss << "Created";
    break;
  case 204:
    ss << "No Content";
    break;
  case 400:
    ss << "Bad Request";
    break;
  case 404:
    ss << "Not Found";
    break;
  case 500:
    ss << "Internal Server Error";
    break;
  default:
    ss << "Unknown";
    break;
  }
  ss << "\r\n";
  for (const auto &[key, value] : resp.headers.data) {
    ss << key << ": " << value << "\r\n";
  }
  ss << "\r\n";
  if (!resp.body.empty()) {
    ss << resp.body;
  }
  return ss.str();
}

Server::Server(ServerConfig cfg)
    : m_Config(std::move(cfg)), m_Routes(), m_IsRunning(false),
      m_WorkerThreads() {}

Server::~Server() { stop(); }

void Server::handle_client(i32 client_socket) {
  char buffer[8192];
  std::string request_data;
#ifdef _WIN32
  i32 n = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
#else
  ssize_t n = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
#endif
  if (n > 0) {
    buffer[n] = '\0';
    request_data = buffer;
    auto req_result = parse_request(request_data);
    Response resp(404, "Not Found");
    if (req_result) {
      auto &req = req_result.value();
      for (const auto &route : m_Routes) {
        if (route.method == req.method && route.path == req.path) {
          try {
            resp = route.handler(req);
          } catch (const std::exception &e) {
            resp = Response(500, std::string("Error: ") + e.what());
          }
          break;
        }
      }
    }
    std::string response_str = build_response(resp);
    send(client_socket, response_str.c_str(), response_str.size(), 0);
  }
#ifdef _WIN32
  closesocket(client_socket);
#else
  close(client_socket);
#endif
}

stl::result<> Server::start() {
#ifdef _WIN32
  WSADATA wsa_data;
  if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
    return stl::make_error<>("Failed to initialize Winsock");
  }
#endif
  m_Config.server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (m_Config.server_socket < 0) {
#ifdef _WIN32
    i32 err = WSAGetLastError();
    return stl::make_error<>("Failed to create socket: " + std::to_string(err));
#else
    return stl::make_error<>("Failed to create socket: " +
                             std::string(strerror(errno)));
#endif
  }
  i32 opt = 1;
  setsockopt(m_Config.server_socket, SOL_SOCKET, SO_REUSEADDR,
             (const char *)&opt, sizeof(opt));
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = htons(m_Config.port);
  if (bind(m_Config.server_socket, (sockaddr *)&addr, sizeof(addr)) < 0) {
#ifdef _WIN32
    i32 err = WSAGetLastError();
    closesocket(m_Config.server_socket);
    return stl::make_error<>("Failed to bind to port " +
                             std::to_string(m_Config.port) + ": " +
                             std::to_string(err));
#else
    close(m_Config.server_socket);
    return stl::make_error<>("Failed to bind to port " +
                             std::to_string(m_Config.port) + ": " +
                             std::string(strerror(errno)));
#endif
  }
  if (listen(m_Config.server_socket, 10) < 0) {
#ifdef _WIN32
    i32 err = WSAGetLastError();
    closesocket(m_Config.server_socket);
    return stl::make_error<>("Failed to listen: " + std::to_string(err));
#else
    close(m_Config.server_socket);
    return stl::make_error<>("Failed to listen: " +
                             std::string(strerror(errno)));
#endif
  }
  m_IsRunning = true;
  return stl::result_success();
}

void Server::run() {
  while (m_IsRunning.load()) {
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    i32 client_socket =
        accept(m_Config.server_socket, (sockaddr *)&client_addr, &client_len);
    if (client_socket < 0) {
#ifdef _WIN32
      int err = WSAGetLastError();
      if (!m_IsRunning.load())
        break;
      // transient errors: sleep and continue
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      continue;
#else
      if (errno == EINTR)
        continue;
      if (!m_IsRunning.load())
        break;
      if (errno == EBADF || errno == EINVAL || errno == ENOTSOCK)
        break;
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      continue;
#endif
    }
    if (m_Config.is_multithreaded) {
      std::thread([this, client_socket]() {
        handle_client(client_socket);
      }).detach();
    } else {
      handle_client(client_socket);
    }
  }
}

void Server::stop() {
  m_IsRunning.store(false);
  if (m_Config.server_socket >= 0) {
#ifdef _WIN32
    ::shutdown(m_Config.server_socket, SD_BOTH);
    closesocket(m_Config.server_socket);
    WSACleanup();
#else
    ::shutdown(m_Config.server_socket, SHUT_RDWR);
    close(m_Config.server_socket);
#endif
    m_Config.server_socket = -1;
  }
}
} // namespace http