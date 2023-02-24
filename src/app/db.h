/*
 * db.h
 * Copyright (c) 2023, Cory Montgomery
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

struct jx_value_t;
typedef struct jx_value_t jx_value;

enum kw_search_type
{
    KW_SEARCH_TYPE_EXACT,
    KW_SEARCH_TYPE_LIKE
};

struct kws_request
{
    const char *query;
    enum kw_search_type type;
    int page, page_size;
};

struct kws_response
{
    jx_value *result;
    int page, page_size;
    int matches;
    bool error;
};

bool db_kw_search(struct kws_response *response, struct kws_request *request);

void db_set_path(const char *fmt, ...);

bool db_open();

bool db_close();

bool db_get_error();

const char *db_get_error_msg();