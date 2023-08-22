#include "Enclave_t.h"

#include <string>

COUNT count = 0;
PID id = 0; // each enclave needs to have its own id

// shared by all the enclaves
const char secret[] = {
  "MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQgnI0T6AoPs+ufh54e\n"
  "3tr6ywY7KkMBZhBs69NvMpvtXeehRANCAAS+G04ABpuwCvaS0v5fi9vuNOEitPon\n"
  "4nIDK/IJOsGXv85Jw5wayZI19lSB6ox05rLB+CxmEXrDyiOhX8Sz7c0L\n"
};

void add(COUNT c) {
  count += c;
}

sgx_status_t TEEadd(COUNT *j, COUNT *c) {
  sgx_status_t status = SGX_SUCCESS;

  add(*j);
  *c = count;

  //ocall_print(("counter=" + std::to_string(count)).c_str());

  return status;
}

auth_t authenticate(std::string text) {
  auth_t auth;
  std::string s = std::to_string(id) + secret + text;
  if (!SHA256 ((const unsigned char *)s.c_str(), s.size(), auth.hash.hash)) { }
  auth.hash.set=true;
  auth.id = id;

  return auth;
}

sgx_status_t TEEsign(payload_t *p, auth_t *res) {
  sgx_status_t status = SGX_SUCCESS;
  std::string text = "";
  for (int i = 0; i < p->size; i++) { text += ((char *)p->data)[i]; }
  *res = authenticate(text);
  return status;
}

bool sameHashes(unsigned char *h1, unsigned char *h2) {
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    if (h1[i] != h2[i]) { return false; }
  }
  return true;
}

bool verify(std::string text, auth_t a) {
  unsigned char hash[SHA256_DIGEST_LENGTH];
  std::string s = std::to_string(a.id) + secret + text;
  if (!SHA256 ((const unsigned char *)s.c_str(), s.size(), hash)) { }
  bool b = sameHashes(a.hash.hash,hash);

  return b;
}

sgx_status_t TEEverify(payload_t *p, auth_t *a, bool *res) {
  sgx_status_t status = SGX_SUCCESS;
  std::string text;
  for (int i = 0; i < p->size; i++) { text += (p->data)[i]; }
  *res = verify(text, *a);
  return status;
}
