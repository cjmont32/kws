/*
 * util.c
 * Copyright (c) 2022, Cory Montgomery
 * All Rights Reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <sys/random.h>

#include "util.h"
#include "cgi.h"

static char rand_bytes[UTIL_RAND_BYTES_SIZE];

bool util_lookup_value_in_kv_string(const char *ptr, char *dst, size_t size, const char *key, const char *delim)
{
    size_t i;

    char *kv_string, *tok, *kv_sep, *val;

    if (dst == NULL || size <= 0 || key == NULL)
        return false;

    *dst = '\0';

    if (ptr == NULL)
        return false;

    kv_string = alloca(strlen(ptr) + 1);

    strcpy(kv_string, ptr);

    val = NULL;

    tok = strtok(kv_string, delim);

    while (tok != NULL) {
        while (isspace(*tok))
            tok++;

        kv_sep = strchr(tok, '=');

        if (kv_sep != NULL) {
            if (strncmp(tok, key, kv_sep - tok) == 0) {
                val = kv_sep + 1;
                break;
            }
        }

        tok = strtok(NULL, delim);
    }

    if (val != NULL) {
        for (i = 0; i < size - 1; i++) {
            if (val[i] == '\0')
                break;

            dst[i] = val[i];
        }

        dst[i] = '\0';

        return true;
    }

    return false;
}

void util_decode_uri_component(char *dst, size_t size, char *src)
{
    size_t i, j;

    char hex[3];

    unsigned char code;

    i = 0;
    j = 0;

    while (i < size - 1 && src[j] != '\0') {
        if (src[j] == '%') {
            if (strlen(src + j) < 3)
                break;

            hex[0] = src[++j];
            hex[1] = src[++j];
            hex[2] = '\0';

            j++;

            if (sscanf(hex, "%hhx", &code) == 1 && code > 0) {
                dst[i++] = (char)code;
            }
        }
        else {
            dst[i++] = src[j++];
        }
    }

    dst[i] = '\0';
}

void util_rand_bytes(char *buf, int bytes)
{
    int seed, value, i;

    unsigned char *ptr;

    ptr = (unsigned char *)&value;

    getrandom(&seed, sizeof(int), 0);

    srand(seed);

    for (i = 0; i < bytes; i++) {
        if (i % 4 == 0) {
            value = rand();
        }

        buf[i] = ptr[i % 4];
    }
}

bool util_gen_uid(char *buf)
{
    int i;

    *buf = '\0';

    util_rand_bytes(rand_bytes, UTIL_RAND_BYTES_SIZE);

    for (i = 0; i < UTIL_RAND_BYTES_SIZE; i++) {
        char hex[3];

        if (i > 0 && i % 2 == 0) {
            strcat(buf, "-");
        }

        snprintf(hex, 3, "%02hhx", rand_bytes[i]);
        strcat(buf, hex);
    }

    return true;
}