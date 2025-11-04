#ifdef SAP_HTTP_USE_MODULES
import http;
#else
#include "net/http.h"
#endif
#include <gtest/gtest.h>

TEST(ClientTest, InvalidUrlGet) {
  auto future = http::Client::get("not-a-valid-url");
  auto result = future.get();

  EXPECT_FALSE(result.has_value());
  EXPECT_TRUE(result.has_error());
}

TEST(ClientTest, InvalidUrlPost) {
  auto future = http::Client::post("not-a-valid-url", "body");
  auto result = future.get();

  EXPECT_FALSE(result.has_value());
  EXPECT_TRUE(result.has_error());
}