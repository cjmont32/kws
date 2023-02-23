#include "cgi.h"
#include "util.h"

#include <jx_util.h>

#include <string.h>

jx_value *get_vars()
{
    extern char **environ;

    int i;

    char line[4096], *sep, *key, *value;

    jx_value *vars = jxd_new();

    if (vars == NULL)
        return NULL;

    for (i = 0; environ[i] != NULL; i++) {
        strcpy(line, environ[i]);

        sep = strchr(line, '=');
        key = line;
        value = sep + 1;

        *sep = '\0';

        jxd_put_string(vars, key, value);
    }

    return vars;
}

jx_value *get_input(jx_cntx *cntx)
{
    ssize_t r;

    if (cntx == NULL) {
        return NULL;
    }

    do {
        r = jx_read(cntx, 0, 2048);
    } while (r > 0);

    if (r < 0) {
        return NULL;
    }

    return jx_get_result(cntx);
}

void output_json(jx_value *v)
{
    char *str = jx_serialize_json(v, false);

    cgi_printf("%s", str);

    free(str);
}

void output_error(jx_cntx *cntx)
{
    if (cntx != NULL) {
        jx_free(cntx);
    }
}

void cgi_begin()
{
    cgi_set_content_type(HTTP_CONTENT_TYPE_APPLICATION_JSON);
}

void cgi_main()
{
    jx_cntx *cntx;
    jx_value *obj, *params;

    cntx = jx_new();

    if (cntx == NULL) {
        output_error(NULL);
        return;
    }

    obj = get_input(cntx);

    if (obj == NULL) {
        output_error(cntx);
        return;
    }

    params = jxd_get(obj, "params");

    output_json(params);

    jxv_free(obj);

    jx_free(cntx);
}

void cgi_end()
{

}