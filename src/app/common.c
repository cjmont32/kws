/*
 * common.c
 * Copyright (c) 2023, Cory Montgomery
 */

#include "cgi.h"
#include "util.h"
#include "html.h"

void cgi_begin()
{
    html_printf("<!DOCTYPE html>");

    html_open_tag("html");
    html_open_tag("head");

    html_open_tag_begin("meta");
    html_open_tag_attr("name", "viewport");
    html_open_tag_attr("content", "width=device-width,initial-scale=1.0");
    html_open_tag_end();

    html_open_tag_begin("meta");
    html_open_tag_attr("charset", "utf-8");
    html_open_tag_end();

    html_include_js("/static/js/kwsapp.js");
    html_include_stylesheet("/static/css/main.css");

    html_open_tag("title");
    html_printf("KWS App");
    html_close_tag("title");

    html_close_tag("head");

    html_open_tag("body");
}

void cgi_end()
{
    html_close_tag("body");
    html_close_tag("html");
}