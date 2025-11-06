#include "net/http.h"
#include <gtest/gtest.h>

TEST(ServerTest, CreateServer) {
  http::ServerConfig cfg{-1, 8080, false};
  http::Server server(std::move(cfg));
  SUCCEED();
}

TEST(ServerTest, AddRoute) {
  http::Server server;
  server.route("/test", http::EMethod::GET, [](const http::Request &req) {
    return http::Response(200, "Test response");
  });
  SUCCEED();
}

TEST(ServerTest, RouteHandlerWithJSON) {
  http::Server server;
  server.route("/api/data", http::EMethod::POST,
               [](const http::Request &req) {
                 // Simulate JSON processing
                 if (req.headers.get("Content-Type") == "application/json") {
                   return http::Response(201, R"({"status": "created"})");
                 }
                 return http::Response(400, "Bad Request");
               });

  SUCCEED();
}

TEST(ServerTest, MultipleRoutes) {
  http::Server server;
  server.route("/", http::EMethod::GET, [](const http::Request &) {
    return http::Response(200, "Home");
  });
  server.route("/api/users", http::EMethod::GET,
               [](const http::Request &) {
                 return http::Response(200, R"([{"id": 1, "name": "John"}])");
               });
  server.route("/api/users", http::EMethod::POST,
               [](const http::Request &req) {
                 return http::Response(201, "User created");
               });
  SUCCEED();
}

TEST(ServerTest, MultithreadedMode) {
  http::ServerConfig cfg{-1, 8081, true};
  http::Server server{std::move(cfg)};
  SUCCEED();
}