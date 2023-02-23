/*
 * html.h
 * Copyright (c) 2023, Cory Montgomery
 */

#include <stdbool.h>

#define html_printf cgi_printf

void html_open_tag(const char *name);

void html_close_tag(const char *name);

void html_open_tag_begin(const char *name);

void html_open_tag_attr(const char *name, const char *value);

void html_open_tag_end();

void html_include_js(const char *path);

void html_include_stylesheet(const char *path);

void html_br();

void html_anchor(const char *target, const char *href, const char *content);
