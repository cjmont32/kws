/*
 * cgi.c
 * Copyright (c) 2023, Cory Montgomery
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/random.h>

#include "cgi.h"
#include "util.h"
#include "html.h"
#include "db.h"

int _cgi_http_status = 200;
char _cgi_http_status_buf[12], *_cgi_http_status_str;
enum http_content_type _cgi_http_content_type = HTTP_CONTENT_TYPE_TEXT_HTML;
bool _cgi_exit, _cgi_malicious;

extern char **environ;

static FILE *content_stream;
static char *content_ptr;
static size_t content_size;

static const char *content_type_strings[HTTP_CONTENT_TYPE_GUARD] =
{
    "text/plain",
    "text/html",
    "application/json"
};

const char *cgi_get_ctype_string(enum http_content_type ctype)
{
    return content_type_strings[ctype];
}

bool cgi_is_debug()
{
    return true;
}

bool cgi_is_prod()
{
    return false;
}

bool cgi_open_stream()
{
    if (content_ptr != NULL) {
        cgi_close_stream();
    }

    content_stream = open_memstream(&content_ptr, &content_size);

    return content_stream != NULL;
}

void cgi_get_size(size_t *ptr)
{
    if (ptr != NULL)
        *ptr = content_size;
}

bool cgi_dump_stream(FILE *out)
{
    size_t i;

    if (content_stream == NULL)
        return false;

    fflush(content_stream);

    for (i = 0; i < content_size; i++) {
        fputc(content_ptr[i], out);
    }

    return true;
}

bool cgi_close_stream()
{
    if (content_stream == NULL)
        return false;

    fclose(content_stream);

    free(content_ptr);

    content_ptr = NULL;
    content_size = 0;

    return true;
}

bool cgi_printf(const char *fmt, ...)
{
    static char last;

    va_list ap;

    char *buf;

    int size;

    if (content_stream == NULL)
        return false;

    va_start(ap, fmt);
    size = vsnprintf(NULL, 0, fmt, ap) + 1;
    va_end(ap);

    buf = alloca(size);   

    va_start(ap, fmt);
    vsnprintf(buf, size, fmt, ap);
    va_end(ap);

    while (*buf != '\0') {
        // Convert all whitespace characters to space characters
        if (isspace(*buf)) {
            *buf = ' ';
        }

        if (last == ' ' && *buf == ' ') {
            buf++;
            continue;
        }

        last = *buf;

        fputc(*buf, content_stream);

        buf++;
    }

    fflush(content_stream);

    return true;

}

void cgi_set_status(int status)
{
    int bytes;

    _cgi_http_status = status;

    bytes = snprintf(_cgi_http_status_buf, 12, "%d", status);

    if (bytes >= 12) {
        _cgi_http_status_buf[11] = '\0';
    }

    _cgi_http_status_str = _cgi_http_status_buf;
}

void cgi_set_content_type(enum http_content_type ctype)
{
    _cgi_http_content_type = ctype;
}

void cgi_begin_http_headers()
{
    const char *ctype_string = cgi_get_ctype_string(_cgi_http_content_type);

    printf("Status: %d\n", _cgi_http_status);
    printf("Content-Type: %s\n", ctype_string);
}

void cgi_end_http_headers()
{
    printf("Content-Length: %lu\n", content_size);
    printf("Cache-Control: no-store\n");
    printf("\n");
}

const char *cgi_get_cookie_domain()
{
    char *base, *top;

    base = getenv("SERVER_NAME");

    if (base == NULL)
        return NULL;

    top = strrchr(base, '.');

    if (top == NULL) {
        return base;
    }

    while (top > base) {
        top--;

        if (*top == '.') {
            break;
        }
    }

    if (*top == '.') {
        top++;
    }

    return top;
}

bool cgi_set_cookie(const char *name, const char *value, const char *domain, uint64_t max_age, bool secure)
{
    if (name == NULL || value == NULL)
        return false;

    printf("Set-Cookie: %s=%s; Path=/; Max-Age=%lu", name, value, max_age);

    if (domain == NULL) {
        domain = cgi_get_cookie_domain();
    }

    if (domain != NULL) {
        printf("; Domain=%s", domain);
    }

    if (secure) {
        printf("; Secure");
    }

    printf("\n");

    return true;
}

bool cgi_get_cookie(char *dst, size_t size, const char *name)
{
    const char *http_cookie = getenv("HTTP_COOKIE");
    return util_lookup_value_in_kv_string(http_cookie, dst, size, name, ";");
}

bool cgi_get_string_from_query_string(char *dst, size_t size, const char *key)
{
    const char *query_str = getenv("QUERY_STRING");
    return util_lookup_value_in_kv_string(query_str, dst, size, key, "&");
}

int cgi_get_int_from_query_string(const char *key, bool *found)
{
    char buf[12];

    if (found != NULL)
        *found = false;

    if (cgi_get_string_from_query_string(buf, 12, key)) {
        if (found != NULL)
            *found = true;

        return (int)strtol(buf, NULL, 10);
    }

    return -1;
}

int cgi_get_int_from_env(const char *key, bool *found_ptr)
{
    int r;
    char *value;
    bool found;

    r = -1;
    found = false;

    value = getenv(key);

    if (value == NULL) {
        goto exit;
    }

    r = (int)strtol(value, NULL, 10);

    found = true;

exit:
    if (found_ptr != NULL) {
        *found_ptr = found;
    }

    return r;
}

void cgi_dump_env()
{
    int i;

    for (i = 0; environ[i] != NULL; i++) {
        cgi_printf("<b>%s<br/>", environ[i]);
    }
}