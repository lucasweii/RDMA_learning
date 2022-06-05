#include <gtest/gtest.h>
#include <rdma.h>
#include <cstdio>
#include <iostream>

TEST(GetDevice, GetDeviceList) {
  struct ibv_device *dev = nullptr;
  struct ibv_device **dev_list = nullptr;
  int num;
  dev_list = ibv_get_device_list(&num);
  ASSERT_NE(dev_list, nullptr);

  for (int i = 0; i < num; i++) {
    dev = dev_list[i];
    auto ctx = ibv_open_device(dev);
    ASSERT_NE(ctx, nullptr);
    printf("=============== device information ============== \n");
    printf("dev name : %s\n", dev->dev_name);
    printf("dev name(2) : %s\n", ibv_get_device_name(dev));
    printf("dev path : %s\n", dev->dev_path);
    printf("ibdev path : %s\n", dev->ibdev_path);
    printf("transport type : %d\n", dev->transport_type);
    printf("node type %s\n", ibv_node_type_str(dev->node_type));
    printf("name : %s\n", dev->name);
    struct ibv_device_attr dev_attr;
    ASSERT_EQ(ibv_query_device(ctx, &dev_attr), 0);
    ASSERT_GE(dev_attr.phys_port_cnt, 1);
    printf("max QP %d\n", dev_attr.max_qp);
    printf("have %d port\n", dev_attr.phys_port_cnt);
    printf("=============== device port information ================= \n");
    for (int i = 0; i < dev_attr.phys_port_cnt; i++) {
      struct ibv_port_attr port_attr;
      ASSERT_EQ(ibv_query_port(ctx, i + 1, &port_attr), 0);
      printf("port state %s\n", ibv_port_state_str(port_attr.state));
      printf("port physical state %d\n", port_attr.phys_state);
      printf("max MTU %d\n", port_attr.max_mtu);
      printf("length of GID table %d\n", port_attr.gid_tbl_len);
      printf("lid of port %d\n", port_attr.lid);
      printf("active MTU %d\n", port_attr.active_mtu);
      printf("max message size %d\n", port_attr.max_msg_sz);
      printf("active width %d\n", port_attr.active_width);
      printf("active speed %d\n", port_attr.active_speed);
      printf("============== device port gid ============ \n");
      union ibv_gid gid;
      ASSERT_EQ(ibv_query_gid(ctx, i + 1, 0, &gid), 0);
      printf("subnet prefix %llx\n", gid.global.subnet_prefix);
      printf("interface id %llx\n", gid.global.interface_id);
      printf("\n\n");
    }
    ibv_close_device(ctx);
  }
  ibv_free_device_list(dev_list);
}