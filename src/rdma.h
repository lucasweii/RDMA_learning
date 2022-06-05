#include <glog/logging.h>
#include <infiniband/verbs.h>
#include <memory>
#include <string>

#define BUF_SIZE 1024
#define BUF_ACCESS (IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE)
#define CQE_NUM 1

enum Opcode {
  RDMA_SEND,
  RDMA_WRITE,
  RDMA_READ,
};

enum QPState {
  RESET = -1,
  INIT,
  RTR,
  RTS,
};

struct Connection {
  /* structure to exchange data which is needed to connect the QPs */
  uint64_t addr;   /* Buffer address */
  uint32_t rkey;   /* Remote key */
  uint32_t qp_num; /* QP number */
  uint16_t lid;    /* LID of the IB port */
  uint8_t gid[16]; /* gid */
  Connection &operator=(const Connection &conn) {
    this->addr = conn.addr;
    this->rkey = conn.rkey;
    this->qp_num = conn.qp_num;
    this->lid = conn.lid;
    memcpy(this->gid, conn.gid, 16);
    return *this;
  }
};

class RDMA {
  using WC = std::shared_ptr<ibv_wc>;

 public:
  RDMA() = default;
  RDMA(uint32_t ib_port, uint32_t gid_idx) : ib_port_(ib_port), gid_idx_(gid_idx){};
  virtual ~RDMA();

  RDMA(const RDMA &) = delete;
  RDMA &operator=(const RDMA &) = delete;

  bool Init(std::string dev_name = "");
  bool ModifyQP(QPState state);

  std::string Read();
  bool Write(std::string msg);
  bool Send(std::string msg);
  std::string Recv();

  void SetRemoteInfo(const Connection &remote_info);
  Connection LocalInfo() const { return local_info_; };
  uint32_t IBPort() const { return ib_port_; }
  ibv_gid GID() const { return gid_; }
  uint32_t LocalKey() const { return lkey_; }
  uint32_t RemoteKey() const { return rkey_; }
  uint32_t Lid() const { return lid_; }
  char *Buf() { return buf_; }
  void PostRecv();
  void PostSend(Opcode op);

 private:
  WC PollCQ();
  ibv_context *dev_ctx_ = nullptr;
  ibv_pd *pd_ = nullptr;
  ibv_mr *mr_ = nullptr;
  ibv_qp *qp_ = nullptr;
  ibv_cq *cq_ = nullptr;

  uint32_t ib_port_ = 1;
  uint32_t gid_idx_ = 0;
  uint32_t lkey_;
  uint32_t rkey_;
  uint16_t lid_;
  ibv_gid gid_;
  char buf_[BUF_SIZE];
  Connection local_info_;
  Connection remote_info_;

  uint64_t request_id_ = 0;
};