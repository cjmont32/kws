/*
 * main.c
 * Copyright (c) 2023, Cory Montgomery
 * All Rights Reserved.
 */

#include <string.h>
#include <stdbool.h>

#include "db.h"
#include "cgi.h"
#include "util.h"

extern int _cgi_http_status;
extern char *_cgi_http_status_str;
extern enum http_content_type _cgi_http_content_type;
extern bool _cgi_exit;

bool should_dumpenv()
{
    return cgi_is_debug() && cgi_get_int_from_query_string("dumpenv", NULL) == 1;
}

void cgi_send_error_response(const char *err_msg)
{
    _cgi_exit = true;

    cgi_set_status(500);
    
    cgi_set_content_type(HTTP_CONTENT_TYPE_TEXT_PLAIN);

    cgi_begin_http_headers();

    cgi_open_stream();

    cgi_printf(err_msg);

    cgi_end_http_headers();

    cgi_dump_stream(stdout);

    cgi_close_stream();
}

void cgi_send_redirect_response(const char *url)
{
    _cgi_exit = true;

    printf("Status: 302\n");
    printf("Location: %s\n", url);
    printf("\n"); 
}

int main(int argc, char **argv)
{ 
    const char *db_path;

    db_path = getenv("KWS_DB_PATH");

    if (db_path == NULL) {
        cgi_send_error_response("No path to database set in environment");
        return 0;
    }

    db_set_path("%s", db_path);

    if (!db_open()) {
        cgi_send_error_response("Database access error");
        return 0;
    }

    cgi_open_stream();

    cgi_set_status(200);

    cgi_begin();

    if (_cgi_exit) {
        goto exit;
    }

    if (should_dumpenv()) {
        cgi_dump_env();
    }

    cgi_main();

    if (_cgi_exit) {
        goto exit;
    }

    cgi_end();

    if (_cgi_exit) {
        goto exit;
    }

    cgi_begin_http_headers();

    cgi_end_http_headers();

    cgi_dump_stream(stdout);

    cgi_close_stream();

exit:
    db_close();

    return 0;
}