#include <iostream>
#include <string>
#include <vector>

#include "base/util/time_util.hpp"
#include "openssl/ec.h"
#include "openssl/ecdsa.h"
#include "openssl/evp.h"
#include "openssl/obj_mac.h"

class Worker {
 public:
  virtual ~Worker() = default;
  virtual std::string Name() const = 0;
  virtual void Prepare() = 0;
  virtual void Sign(const std::string data, std::string& sig) = 0;
  virtual void Verify(const std::string data, const std::string sig) = 0;
};

class Worker_Foo : public Worker {
 public:
  std::string Name() const override { return "Foo"; }
  void Prepare() override {}
  void Sign(const std::string data, std::string& sig) override {}
  void Verify(const std::string data, const std::string sig) override {}
};

class Worker_ECDSA : public Worker {
 public:
  std::string Name() const override { return "ECDSA_" + CurveName(); }
  void Prepare() override {
    pkey_ = EC_KEY_new();
    assert(pkey_ != nullptr);
    curve_ = EC_GROUP_new_by_curve_name(CurveId());
    assert(curve_ != nullptr);
    int ret = EC_KEY_set_group(pkey_, curve_);
    assert(ret == 1);
    ret = EC_KEY_generate_key(pkey_);
    assert(ret == 1);
  }
  void Sign(const std::string data, std::string& sig) override {
    sig_ =
        ECDSA_do_sign((const unsigned char*)data.c_str(), data.size(), pkey_);
    assert(sig_ != nullptr);
  }
  void Verify(const std::string data, const std::string sig) override {
    int ret = ECDSA_do_verify((const unsigned char*)data.c_str(), data.size(),
                              sig_, pkey_);
    assert(ret == 1);
  }

  ~Worker_ECDSA() {
    if (nullptr != sig_) {
      ECDSA_SIG_free(sig_);
    }
    if (nullptr != curve_) {
      EC_GROUP_free(curve_);
    }
    if (nullptr != pkey_) {
      EC_KEY_free(pkey_);
    }
  }

 protected:
  virtual std::string CurveName() const = 0;
  virtual int CurveId() const = 0;

 private:
  EC_KEY* pkey_;
  EC_GROUP* curve_;
  ECDSA_SIG* sig_;
};

// clang-format off
#define Worker_ECDSA_Define(_name, _sn, _nid)                      \
  class Worker_ECDSA_##_name : public Worker_ECDSA {               \
    protected:                                                     \
      std::string CurveName() const override{ return _sn; }        \
      int CurveId() const override { return _nid; }                \
  };
// clang-format on

Worker_ECDSA_Define(prime192v1, SN_X9_62_prime192v1, NID_X9_62_prime192v1);
Worker_ECDSA_Define(prime256v1, SN_X9_62_prime256v1, NID_X9_62_prime256v1);

class Worker_EVP : Worker {
 public:
  std::string Name() const override { return "EVP"; }
  void Prepare() override {
    ctx_ = EVP_MD_CTX_new();
    assert(ctx_ != nullptr);

    // TODO
    // EVP_PKEY_CTX_
  }
  void Sign(const std::string data, std::string& sig) override {
    EVP_DigestSignInit(ctx_, nullptr, nullptr, nullptr, pkey_);
  }
  void Verify(const std::string data, const std::string sig) override {
    // TODO
  }

 private:
  EVP_MD_CTX* ctx_;
  EVP_PKEY* pkey_;
  EVP_PKEY_CTX* pctx_;
};

int main(int argc, char const* argv[]) {
  std::vector<Worker*> workers{
      // new Worker_Foo(),
      new Worker_ECDSA_prime192v1(),
      new Worker_ECDSA_prime256v1(),
  };

  std::string data = "01234567890123456789012345678901234567890123456789";
  for (auto w : workers) {
    w->Prepare();
    std::string sig;
    {
      auto cost =
          base::util::timeCostInNanoseconds([&] { w->Sign(data, sig); });
      std::cout << w->Name() << " sign cost: " << cost << "ns\n";
    }
    {
      auto cost =
          base::util::timeCostInNanoseconds([&] { w->Verify(data, sig); });
      std::cout << w->Name() << " verify cost: " << cost << "ns\n";
    }
  }

  for (auto w : workers) {
    delete w;
  }
  workers.clear();

  return 0;
}
