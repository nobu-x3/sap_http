#include "net/http.h"
#include <gtest/gtest.h>

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

TEST(ResponseTest, ConstructorWithBody) {
  http::Response resp(200, "Hello World");
  EXPECT_EQ(resp.status_code, 200);
  EXPECT_EQ(resp.body, "Hello World");
  EXPECT_EQ(resp.headers.get("Content-Length"), "11");
}