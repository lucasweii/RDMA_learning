#include "rdma.h"
#include <glog/logging.h>
#include <cassert>
#include <cstring>
#include <exception>

RDMA::~RDMA() {
  int rc;

  if (qp_ != nullptr) {
    rc = ibv_destroy_qp(qp_);
    assert(rc == 0);
    qp_ = nullptr;
  }

  if (mr_ != nullptr) {
    rc = ibv_dereg_mr(mr_);
    assert(rc == 0);
    mr_ = nullptr;
  }

  if (cq_ != nullptr) {
    rc = ibv_destroy_cq(cq_);
    assert(rc == 0);
    cq_ = nullptr;
  }

  if (pd_ != nullptr) {
    rc = ibv_dealloc_pd(pd_);
    assert(rc == 0);
    pd_ = nullptr;
  }

  if (dev_ctx_ != nullptr) {
    rc = ibv_close_device(dev_ctx_);
    assert(rc == 0);
    dev_ctx_ = nullptr;
  }
}

bool RDMA::Init(std::string dev_name) {
  bool first_dev = false;
  int rc;

  if (dev_name.size() == 0) {
    first_dev = true;
  }
  int dev_num;
  auto dev_list = ibv_get_device_list(&dev_num);
  if (dev_num <= 0) {
    LOG(ERROR) << "none IB device";
    return false;
  }

  ibv_device *dev = nullptr;
  if (first_dev) {
    dev = dev_list[0];
    dev_name = dev->dev_name;
  } else {
    for (int i = 0; i < dev_num; i++) {
      auto name = ibv_get_device_name(dev_list[i]);
      if (strcmp(dev_name.c_str(), name) == 0) {
        dev = dev_list[i];
        break;
      }
    }
  }

  if (dev == nullptr) {
    LOG(ERROR) << "cannot find device : " << dev_name;
    ibv_free_device_list(dev_list);
    return false;
  }

  // open device
  dev_ctx_ = ibv_open_device(dev);
  assert(dev_ctx_ != nullptr);
  LOG(INFO) << "node state : " << ibv_node_type_str(dev->node_type);

  // query port num
  ibv_device_attr dev_attr;
  memset(&dev_attr, 0, sizeof(dev_attr));
  rc = ibv_query_device(dev_ctx_, &dev_attr);
  assert(rc == 0);
  if (ib_port_ > dev_attr.phys_port_cnt) {
    LOG(ERROR) << "invalid IB port for device : " << dev_name << " with " << dev_attr.phys_port_cnt
               << " port";
    ibv_free_device_list(dev_list);
    return false;
  }

  // query port lid and state
  ibv_port_attr port_attr;
  memset(&port_attr, 0, sizeof(port_attr));
  rc = ibv_query_port(dev_ctx_, ib_port_, &port_attr);
  assert(rc == 0);
  LOG(INFO) << "port state : " << ibv_port_state_str(port_attr.state);
  LOG(INFO) << "port lid : " << port_attr.lid;
  lid_ = port_attr.lid;

  // query gid
  rc = ibv_query_gid(dev_ctx_, ib_port_, gid_idx_, &gid_);
  assert(rc == 0);

  ibv_free_device_list(dev_list);

  pd_ = ibv_alloc_pd(dev_ctx_);
  assert(pd_ != nullptr);

  memset(buf_, 0, BUF_SIZE);
  mr_ = ibv_reg_mr(pd_, buf_, BUF_SIZE, BUF_ACCESS);
  assert(mr_ != nullptr);
  lkey_ = mr_->lkey;
  rkey_ = mr_->rkey;
  cq_ = ibv_create_cq(dev_ctx_, CQE_NUM, nullptr, nullptr, 0);
  assert(cq_ != nullptr);

  ibv_qp_init_attr qp_init_attr = {
      .send_cq = cq_,
      .recv_cq = cq_,
      .srq = nullptr,
      .cap =
          {
              .max_send_wr = 1,
              .max_recv_wr = 1,
              .max_send_sge = 1,
              .max_recv_sge = 1,
              .max_inline_data = 0,
          },
      .qp_type = IBV_QPT_RC,
      .sq_sig_all = 1,
  };
  qp_ = ibv_create_qp(pd_, &qp_init_attr);
  assert(qp_ != nullptr);

  // set local_info
  memcpy(local_info_.gid, gid_.raw, 16);
  local_info_.addr = (uint64_t)buf_;
  local_info_.lid = lid_;
  local_info_.qp_num = qp_->qp_num;
  local_info_.rkey = rkey_;
  char tmp[64];
  sprintf(tmp, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
          local_info_.gid[0], local_info_.gid[1], local_info_.gid[2], local_info_.gid[3],
          local_info_.gid[4], local_info_.gid[5], local_info_.gid[6], local_info_.gid[7],
          local_info_.gid[8], local_info_.gid[9], local_info_.gid[10], local_info_.gid[11],
          local_info_.gid[12], local_info_.gid[13], local_info_.gid[14], local_info_.gid[15]);
  LOG(INFO) << "LOCAL gid : " << tmp;
  LOG(INFO) << "LOCAL addr : " << local_info_.addr;
  LOG(INFO) << "LOCAL rkey : " << local_info_.rkey;
  LOG(INFO) << "LOCAL lid : " << local_info_.lid;
  LOG(INFO) << "LOCAL qp num : " << local_info_.qp_num;
  return true;
}

void RDMA::SetRemoteInfo(const Connection &remote_info) {
  remote_info_ = remote_info;
  char tmp[128];
  sprintf(tmp, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
          remote_info_.gid[0], remote_info_.gid[1], remote_info_.gid[2], remote_info_.gid[3],
          remote_info_.gid[4], remote_info_.gid[5], remote_info_.gid[6], remote_info_.gid[7],
          remote_info_.gid[8], remote_info_.gid[9], remote_info_.gid[10], remote_info_.gid[11],
          remote_info_.gid[12], remote_info_.gid[13], remote_info_.gid[14], remote_info_.gid[15]);
  LOG(INFO) << "REMOTE gid : " << tmp;
  LOG(INFO) << "REMOTE addr : " << remote_info_.addr;
  LOG(INFO) << "REMOTE rkey : " << remote_info_.rkey;
  LOG(INFO) << "REMOTE lid : " << remote_info_.lid;
  LOG(INFO) << "REMOTE qp num : " << remote_info_.qp_num;
};

bool RDMA::ModifyQP(QPState state) {
  ibv_qp_attr attr;
  memset(&attr, 0, sizeof(attr));
  int flags;
  int rc;
  switch (state) {
    case INIT:
      attr.qp_state = IBV_QPS_INIT;
      attr.port_num = ib_port_;
      attr.pkey_index = 0;
      attr.qp_access_flags = BUF_ACCESS;
      flags = IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS;
      rc = ibv_modify_qp(qp_, &attr, flags);
      assert(rc == 0);
      break;
    case RTR:
      attr.qp_state = IBV_QPS_RTR;
      attr.path_mtu = IBV_MTU_256;
      attr.dest_qp_num = remote_info_.qp_num;
      attr.rq_psn = 0;
      attr.max_dest_rd_atomic = 1;
      attr.min_rnr_timer = 0x12;

      attr.ah_attr.dlid = remote_info_.lid;
      attr.ah_attr.sl = 0;
      attr.ah_attr.src_path_bits = 0;
      attr.ah_attr.port_num = ib_port_;
      attr.ah_attr.is_global = 1;
      memcpy(&attr.ah_attr.grh.dgid, remote_info_.gid, 16);
      attr.ah_attr.grh.flow_label = 0;
      attr.ah_attr.grh.hop_limit = 1;
      attr.ah_attr.grh.sgid_index = gid_idx_;
      attr.ah_attr.grh.traffic_class = 0;

      flags = IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN | IBV_QP_RQ_PSN |
              IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER;
      rc = ibv_modify_qp(qp_, &attr, flags);
      assert(rc == 0);
      break;
    case RTS:
      attr.qp_state = IBV_QPS_RTS;
      attr.timeout = 14;
      attr.retry_cnt = 7;
      attr.rnr_retry = 7;
      attr.sq_psn = 0;
      attr.max_rd_atomic = 1;
      flags = IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT | IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN |
              IBV_QP_MAX_QP_RD_ATOMIC;
      rc = ibv_modify_qp(qp_, &attr, flags);
      assert(rc == 0);
      break;
    default:
      assert(0);
      break;
  }
  return true;
}

void RDMA::PostRecv() {
  ibv_sge sge = {
      .addr = (uintptr_t)buf_,
      .length = BUF_SIZE,
      .lkey = lkey_,
  };

  ibv_recv_wr wr = {
      .wr_id = request_id_++,
      .next = nullptr,
      .sg_list = &sge,
      .num_sge = 1,
  };
  ibv_recv_wr *bad_wr;
  int rc = ibv_post_recv(qp_, &wr, &bad_wr);
  assert(rc == 0);
}

void RDMA::PostSend(Opcode op) {
  ibv_sge sge = {
      .addr = (uintptr_t)buf_,
      .length = BUF_SIZE,
      .lkey = lkey_,
  };
  ibv_wr_opcode opcode;
  switch (op) {
    case RDMA_WRITE:
      opcode = IBV_WR_RDMA_WRITE;
      break;
    case RDMA_READ:
      opcode = IBV_WR_RDMA_READ;
      break;
    case RDMA_SEND:
      opcode = IBV_WR_SEND;
      break;
    default:
      assert(0);
      break;
  }
  ibv_send_wr wr = {
      .wr_id = request_id_++,
      .next = nullptr,
      .sg_list = &sge,
      .num_sge = 1,
      .opcode = opcode,
      .send_flags = IBV_SEND_SIGNALED,
  };

  ibv_send_wr *bad_wr;

  if (opcode != IBV_WR_SEND) {
    wr.wr.rdma.remote_addr = remote_info_.addr;
    wr.wr.rdma.rkey = remote_info_.rkey;
  }
  int rc = ibv_post_send(qp_, &wr, &bad_wr);
  assert(rc == 0);
}

RDMA::WC RDMA::PollCQ() {
  ibv_wc *wc = new ibv_wc();
  int rc;
  do {
    rc = ibv_poll_cq(cq_, 1, wc);
  } while (rc == 0);
  LOG_ASSERT(rc == 1) << " return wc " << rc;
  return WC(wc);
}

std::string RDMA::Read() {
  PostSend(RDMA_READ);
  auto wc = PollCQ();
  if (wc->status == IBV_WC_SUCCESS) {
    LOG(INFO) << "finish READ request " << wc->wr_id;
    return std::string(Buf());
  }
  LOG(ERROR) << "fail READ " << wc->wr_id;
  LOG(ERROR) << "error :" << ibv_wc_status_str(wc->status);
  return "";
}

bool RDMA::Write(std::string msg) {
  strcpy(Buf(), msg.c_str());
  PostSend(RDMA_WRITE);
  auto wc = PollCQ();
  if (wc->status == IBV_WC_SUCCESS) {
    LOG(INFO) << "finish WRITE request " << wc->wr_id;
    return true;
  }
  LOG(ERROR) << "fail WRITE " << wc->wr_id;
  LOG(ERROR) << "error :" << ibv_wc_status_str(wc->status);
  return false;
}

bool RDMA::Send(std::string msg) {
  strcpy(Buf(), msg.c_str());
  PostSend(RDMA_SEND);
  auto wc = PollCQ();
  if (wc->status == IBV_WC_SUCCESS) {
    LOG(INFO) << "finish SEND request " << wc->wr_id;
    return true;
  }
  LOG(ERROR) << "fail SEND " << wc->wr_id;
  LOG(ERROR) << "error :" << ibv_wc_status_str(wc->status);
  return false;
}

std::string RDMA::Recv() {
  PostRecv();
  auto wc = PollCQ();
  if (wc->status == IBV_WC_SUCCESS) {
    LOG(INFO) << "finish RECV request " << wc->wr_id;
    return std::string(Buf());
  }
  LOG(ERROR) << "fail RECV  " << wc->wr_id;
  LOG(ERROR) << "error :" << ibv_wc_status_str(wc->status);

  return "";
}