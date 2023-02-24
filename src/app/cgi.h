/*
 * cgi.h
 * Copyright (c) 2023, Cory Montgomery
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

enum http_content_type
{
    HTTP_CONTENT_TYPE_TEXT_PLAIN,
    HTTP_CONTENT_TYPE_TEXT_HTML,
    HTTP_CONTENT_TYPE_APPLICATION_JSON,
    HTTP_CONTENT_TYPE_GUARD
};

bool cgi_is_prod();

bool cgi_is_debug();

const char *cgi_server();

bool cgi_server_is_apache();

bool cgi_server_is_node();

bool cgi_open_stream();

bool cgi_get_content_size(size_t *size);

bool cgi_dump_stream(FILE *out);

bool cgi_close_stream();

bool cgi_printf(const char *fmt, ...);

void cgi_set_status(int status);

void cgi_set_content_type(enum http_content_type ctype);

void cgi_begin_http_headers();

void cgi_end_http_headers();

bool cgi_set_cookie(const char *name, const char *value, const char *domain, uint64_t max_age, bool secure);

bool cgi_get_cookie(char *dst, size_t size, const char *name);

bool cgi_get_string_from_query_string(char *dst, size_t size, const char *key);

int cgi_get_int_from_query_string(const char *key, bool *found);

int cgi_get_int_from_env(const char *key, bool *found);

void cgi_dump_env();

void cgi_send_redirect_response(const char *url);

void cgi_send_error_response(const char *err_msg);

void cgi_begin();

void cgi_main();

void cgi_end();