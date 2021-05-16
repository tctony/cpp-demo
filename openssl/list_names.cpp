// clang-format off
#define ZLOG_TAG "zlog"
#include "base/zlog/zlog_to_console.h"
// clang-format on

#include <openssl/evp.h>
#include <openssl/objects.h>

#ifdef USE_BORINGSSL
#error boringssl-master-with-bazel do not contains decrepit
#endif

void name_visitor(const OBJ_NAME *obj, void *arg) {
  auto name = obj->name;
  zinfo("%_: %_", (const char *)arg, name);
}

int main(int argc, char const *argv[]) {
  OpenSSL_add_all_algorithms();

  const int types[] = {
      OBJ_NAME_TYPE_MD_METH,
      OBJ_NAME_TYPE_CIPHER_METH,
#ifndef USE_BORINGSSL
      OBJ_NAME_TYPE_PKEY_METH,
      OBJ_NAME_TYPE_COMP_METH,
#endif
  };
  const char *names[] = {
      "digest",
      "cipher",
#ifndef USE_BORINGSSL
      "pkey",
      "comp",
#endif
  };
  for (int i = 0; i < sizeof(types) / sizeof(int); ++i) {
    void *arg = (void *)(names[i]);
    OBJ_NAME_do_all_sorted(types[i], name_visitor, arg);
  }

  return 0;
}
