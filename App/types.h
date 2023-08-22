#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include <openssl/sha.h>
#include "config.h"

typedef struct _hash_t
{
  unsigned char hash[SHA256_DIGEST_LENGTH];
  bool set;
} hash_t;

typedef struct _auth_t
{
  PID id;
  hash_t hash;
} auth_t;

typedef struct _payload_t
{
  unsigned int size;
  unsigned char data[MAX_SIZE_PAYLOAD];
} payload_t;

#endif
