#include "tcp_connection.h"
#include <gtest/gtest.h>
#include <thread>

struct DummyData {
  int a;
  int b;
  char msg[128];
};

void testSync(TCPConnector *tcp, bool is_server) { ASSERT_TRUE(tcp->Sync()); }

void testExchange(TCPConnector *tcp, bool is_server) {
  if (is_server) {
    DummyData send_msg = {.a = 1, .b = 10};
    strcpy(send_msg.msg, "Hello TCP : server!!!");
    DummyData recv;
    EXPECT_EQ(tcp->ExchangeData((char *)&send_msg, sizeof(send_msg), (char *)&recv, sizeof(recv)),
              sizeof(recv));
    EXPECT_EQ(recv.a, 10);
    EXPECT_EQ(recv.b, 1);
    std::cout << recv.msg << std::endl;
    EXPECT_STREQ(recv.msg, "Hello TCP : client!!!");
  } else {
    DummyData send_msg = {.a = 10, .b = 1};
    strcpy(send_msg.msg, "Hello TCP : client!!!");
    DummyData recv;
    EXPECT_EQ(tcp->ExchangeData((char *)&send_msg, sizeof(send_msg), (char *)&recv, sizeof(recv)),
              sizeof(recv));
    EXPECT_EQ(recv.a, 1);
    EXPECT_EQ(recv.b, 10);
    std::cout << recv.msg << std::endl;
    EXPECT_STREQ(recv.msg, "Hello TCP : server!!!");
  }
}

TEST(TCPConnectorTest, Simple) {
  TCPConnector server("23333");
  auto client_thread = std::thread([]() {
    TCPConnector client;
    ASSERT_TRUE(client.Connect("127.0.0.1", "23333"));
    testSync(&client, false);
    testExchange(&client, false);
  });
  ASSERT_TRUE(server.Connect());
  testSync(&server, true);
  testExchange(&server, true);
  client_thread.join();
}
