#include "server.h"
#include <gtest/gtest.h>

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  Server sever("23333", 1, 0);
  EXPECT_TRUE(sever.Connect());
  EXPECT_TRUE(sever.Send("hello world!!!"));
  EXPECT_EQ(sever.Read(), std::string("hello world!!!"));
  
  EXPECT_TRUE(sever.Sync());

  EXPECT_STREQ(sever.Buf(), "Hahhhh");
  return 0;
}
