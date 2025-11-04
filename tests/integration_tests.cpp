#ifdef SAP_HTTP_USE_MODULES
import http;
#else
#include "net/http.h"
#endif
#include <gtest/gtest.h>

TEST(IntegrationTest, HttpBinGet) {
  auto future = http::Client::get("http://httpbingo.org/get");
  auto result = future.get();

  ASSERT_TRUE(result.has_value());
  auto &response = result.value();
  EXPECT_TRUE(response.is_success());
  EXPECT_EQ(response.status_code, 200);
  EXPECT_FALSE(response.body.empty());
}

TEST(IntegrationTest, HttpBinPost) {
  auto future =
      http::Client::post("http://httpbingo.org/post", R"({"test": "data"})");
  auto result = future.get();

  ASSERT_TRUE(result.has_value());
  auto &response = result.value();
  EXPECT_TRUE(response.is_success());
  EXPECT_EQ(response.status_code, 200);
}

TEST(IntegrationTest, HttpBinStatus404) {
  auto future = http::Client::get("http://httpbingo.org/status/404");
  auto result = future.get();

  ASSERT_TRUE(result.has_value());
  auto &response = result.value();
  EXPECT_FALSE(response.is_success());
  EXPECT_EQ(response.status_code, 404);
}

TEST(IntegrationTest, HttpBinHeaders) {
  auto url_result = http::URL::parse("http://httpbingo.org/headers");
  ASSERT_TRUE(url_result.has_value());

  http::Request req(http::EMethod::GET, std::move(url_result.value()));
  req.set_header("X-Custom-Header", "test-value");

  auto future = http::Client::async_send(std::move(req));
  auto result = future.get();

  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.value().is_success());
}

TEST(ResponseTest, IsSuccessFor2xx) {
  http::Response resp;
  resp.status_code = 200;
  EXPECT_TRUE(resp.is_success());

  resp.status_code = 201;
  EXPECT_TRUE(resp.is_success());

  resp.status_code = 204;
  EXPECT_TRUE(resp.is_success());

  resp.status_code = 299;
  EXPECT_TRUE(resp.is_success());
}

TEST(ResponseTest, IsSuccessForNon2xx) {
  http::Response resp;
  resp.status_code = 199;
  EXPECT_FALSE(resp.is_success());

  resp.status_code = 300;
  EXPECT_FALSE(resp.is_success());

  resp.status_code = 404;
  EXPECT_FALSE(resp.is_success());

  resp.status_code = 500;
  EXPECT_FALSE(resp.is_success());
}