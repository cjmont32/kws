/*
 * html.c
 * Copyright (c) 2023, Cory Montgomery
 */

#include "cgi.h"
#include "html.h"

void html_open_tag(const char *name)
{
    html_printf("<%s>", name);
}

void html_close_tag(const char *name)
{
    html_printf("</%s>", name);
}

void html_open_tag_begin(const char *name)
{
    html_printf("<%s", name);
}

void html_open_tag_attr(const char *name, const char *value)
{
    html_printf(" %s=\"%s\"", name, value);
}

void html_open_tag_end()
{
    html_printf(">");
}

void html_include_js(const char *path)
{
    html_open_tag_begin("script");
    html_open_tag_attr("type", "text/javascript");
    html_open_tag_attr("src", path);
    html_open_tag_end();
    html_close_tag("script");
}

void html_include_stylesheet(const char *path)
{
    html_open_tag_begin("link");
    html_open_tag_attr("rel", "stylesheet");
    html_open_tag_attr("href", path);
    html_open_tag_end();
}

void html_br()
{
    html_printf("<br />");
}

void html_anchor(const char *target, const char *href, const char *content)
{
    if (target == NULL) {
        target = "_self";
    }

    html_printf("<a target=\"%s\" href=\"%s\">%s</a>", target, href, content);
}
