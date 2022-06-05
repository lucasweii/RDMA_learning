#include "client.h"
#include <gtest/gtest.h>

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  Client client(1, 0);
  EXPECT_TRUE(client.Connect("192.168.1.11", "23333"));

  EXPECT_EQ(client.Recv(), std::string("hello world!!!"));

  EXPECT_TRUE(client.Sync());

  EXPECT_TRUE(client.Write("Hahhhh"));
  EXPECT_EQ(client.Read(), std::string("Hahhhh"));
  return 0;
}
