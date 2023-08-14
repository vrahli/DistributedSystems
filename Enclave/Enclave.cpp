#include "Enclave_t.h"

#include <string>

#include "../App/config.h"

COUNT count = 0;

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
