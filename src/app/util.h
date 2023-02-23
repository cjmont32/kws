/*
 * util.h
 * Copyright (c) 2022, Cory Montgomery
 * All Rights Reserved.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define UTIL_UID_SIZE 40
#define UTIL_RAND_BYTES_SIZE 16

bool util_lookup_value_in_kv_string(const char *ptr, char *dst, size_t size, const char *key, const char *delim);

void util_decode_uri_component(char *dst, size_t size, char *src);

bool util_gen_uid(char *buf);
