/*
 * db.h
 * Copyright (c) 2023, Cory Montgomery
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

struct jx_value;
typedef struct jx_value jx_value;

struct kws_request
{
    const char *query;
    int page, page_size;
};

struct kws_result
{
    jx_value *result;
    int page, page_size;
};

bool db_kw_search(struct kws_result *result, struct kws_request *request);

void db_set_path(const char *fmt, ...);

bool db_open();

bool db_close();

bool db_get_error();

const char *db_get_error_msg();