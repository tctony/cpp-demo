// clang-format off
#define ZLOG_TAG "zlog"
#include "base/zlog/zlog_to_console.h"
// clang-format on

#include "openssl/hmac.h"

#include "absl/strings/escaping.h"

// https://tools.ietf.org/pdf/rfc2104.pdf
/*
K: secret key
H: hash function
B: block size
L: length of hash result
len(K): [1, B] or
        K = H(K) if len(K) > B, thus len(K) is L
        len(K) >= L is recommended
ipad = the byte 0x36 repeated B times
opad = the byte 0x5C repeated B times.

H(K XOR opad, H(K XOR ipad, text))
    (1) append zeros to the end of K to create a B byte string
        (e.g., if K is of length 20 bytes and B=64, then K will be
         appended with 44 zero bytes 0x00)
    (2) XOR (bitwise exclusive-OR) the B byte string computed in step
        (1) with ipad
    (3) append the stream of data ’text’ to the B byte string resulting
        from step (2)
    (4) apply H to the stream generated in step (3)
    (5) XOR (bitwise exclusive-OR) the B byte string computed in
        step (1) with opad
    (6) append the H result from step (4) to the B byte string
        resulting from step (5)
    (7) apply H to the stream generated in step (6) and output
the result
*/

// sample code implements hmac-sha-256

const std::string key = "this is the key";
const std::string data = "this is the data";
constexpr int digest_size = 32;
unsigned char digest[digest_size];
constexpr int block_size = 64;

std::string do_openssl_hmac() {
  auto evp_md = EVP_sha256();

  unsigned int out_len;
  HMAC(evp_md, key.data(), key.length(), (const unsigned char *)data.data(),
       data.length(), digest, &out_len);
  assert(out_len == digest_size);
  return absl::BytesToHexString(
      absl::string_view((const char *)digest, out_len));
}

std::string do_sha256(const std::string &input) {
  auto evp_md = EVP_sha256();
  unsigned int out_len;
  auto ret = EVP_Digest(input.data(), input.length(), digest, &out_len, evp_md,
                        nullptr);
  assert(ret == 1);
  assert(out_len == digest_size);
  return std::string((const char *)digest, out_len);
}

std::string do_my_hmac() {
  std::array<char, block_size> buffer{};
  std::copy(key.begin(), key.end(), buffer.begin());
  for (int i = 0; i < block_size; ++i) {
    buffer[i] ^= 0x36;
  }
  auto inner_result =
      do_sha256(std::string(buffer.begin(), buffer.end()) + data);
  assert(inner_result.length() == digest_size);
  for (int i = 0; i < block_size; ++i) {
    buffer[i] ^= 0x36;  // rollback to original value
    buffer[i] ^= 0x5c;
  }
  auto result =
      do_sha256(std::string(buffer.begin(), buffer.end()) + inner_result);
  assert(result.length() == digest_size);
  return absl::BytesToHexString(result);
}

int main(int argc, char const *argv[]) {
  auto openssl_result = do_openssl_hmac();
  zinfo("openssl result\n%_", openssl_result);
  auto my_result = do_my_hmac();
  zinfo("my result\n%_", my_result);
  if (openssl_result == my_result) {
    zinfo("ok");
  } else {
    zinfo("result not match");
  }
  return 0;
}
