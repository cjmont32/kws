/*
 * db.c
 * Copyright (c) 2023, Cory Montgomery
 */

#include <stdio.h>
#include <sqlite3.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "db.h"

#include <jx_value.h>

#define DB_PATH_SIZE            1024
#define DB_ERROR_MSG_SIZE       1024

static sqlite3 *db;
static char db_path[DB_PATH_SIZE];
static char db_error_msg[DB_ERROR_MSG_SIZE];
static int db_rc = SQLITE_OK;
static bool _db_in_transaction = false;

enum db_status
{
    DB_STATUS_ERROR,
    DB_STATUS_OK,
    DB_STATUS_IN_TRANSACTION,
    DB_STATUS_NOT_IN_TRANSACTION
};

enum db_action
{
    DB_ACTION_GET_KW_CNT_EXACT_MATCH,
    DB_ACTION_GET_KW_CNT_LIKE_MATCH,
    DB_ACTION_GUARD
};

struct db_stmt {
    sqlite3_stmt *stmt;
    const char *sql;
}
db_stmt_cache[] =
{
    { NULL, "SELECT COUNT(*) AS CNT FROM keywords WHERE keyword = ?;" },
    { NULL, "SELECT COUNT(*) AS CNT FROM keywords WHERE keyword LIKE ?;" }
};

const char *get_answers_sql =
    "SELECT v.qid, v.question, v.rank, at.answer FROM                   "
    "(                                                                  "
    "   SELECT qt.qid, qt.question, COUNT(*) AS rank FROM               "
    "   (                                                               "
    "       SELECT qid, keyword FROM keywords WHERE keyword IN (%s)     "
    "   ) AS kt                                                         "
    "   INNER JOIN questions AS qt ON (kt.qid = qt.qid)                 "
    "   GROUP BY qt.qid, qt.question                                    "
    ") AS v                                                             "
    "INNER JOIN answers AS at ON (v.qid = at.qid)                       "
    "ORDER BY rank DESC                                                 "
    "LIMIT ? OFFSET ?;                                                  ";

void db_set_error_msg(const char *fmt, ...);
enum db_status db_exec_sql(const char *sql);
enum db_status db_begin();
enum db_status db_rollback();
enum db_status db_commit();
sqlite3_stmt *db_get_stmt(enum db_action action);
void db_clear_cache();
bool db_bind_null(sqlite3_stmt *stmt, int index);
bool db_bind_text(sqlite3_stmt *stmt, int index, const char *str);
bool db_bind_int(sqlite3_stmt *stmt, int index, int value);
bool db_bind_int_foreign_key(sqlite3_stmt *stmt, int index, int value);
int db_step(sqlite3_stmt *stmt);
bool db_reset(sqlite3_stmt *stmt);
bool db_finalize(sqlite3_stmt *stmt);

void str_to_lower(char *str)
{
    while (*str) {
        *str = tolower(*str);
        str++;
    }
}

void terminate(char *str)
{
    while (*str) {
        if (*str == '?') {
            *str = '\0';
            break;
        }

        str++;
    }
}

char *get_sql_with_n_params(const char *sql_in, int n)
{
    int count;

    char *sql_out, *p_str, *ptr;

    if (n < 1)
        return NULL;

    p_str = ptr = alloca(n * 2);

    do {
        *(ptr++) = '?';
        *(ptr++) = (n > 1) ? ',' : '\0';
    } while (--n > 0);

    count = snprintf(NULL, 0, sql_in, p_str);

    sql_out = malloc(count + 1);

    if (sql_out == NULL)
        return NULL;

    sprintf(sql_out, sql_in, p_str);

    return sql_out;
}

int db_get_keyword_count(enum db_action act, const char *param)
{
    int r;
    sqlite3_stmt *stmt;

    stmt = db_get_stmt(act);

    if (stmt == NULL)
        return -1;

    if (!db_bind_text(stmt, 1, param)) {
        db_reset(stmt);
        return -1;
    }

    db_rc = db_step(stmt);

    if (db_get_error()) {
        db_reset(stmt);
        return -1;
    }

    r = sqlite3_column_int(stmt, 0);

    db_reset(stmt);

    return r;
}

int db_get_keyword_exact_matches(const char* kw)
{
    return db_get_keyword_count(DB_ACTION_GET_KW_CNT_EXACT_MATCH, kw);
}

int db_get_keyword_like_matches(const char *kw)
{
    char *param;

    param = alloca(sizeof(kw) + 3);

    sprintf(param, "%%%s%%", kw);

    return db_get_keyword_count(DB_ACTION_GET_KW_CNT_LIKE_MATCH, param);
}

jx_value *db_get_kw_list(struct kws_request *request)
{
    int count;
    jx_value *kw_set, *kw_list;
    char *query, *kw;

    kw_set = jxd_new();
    kw_list = jxa_new(10);

    query = alloca(strlen(request->query) + 1);

    strcpy(query, request->query);

    kw = strtok(query, " ");

    do {
        str_to_lower(kw);

        terminate(kw);

        if (jxd_has_key(kw_set, kw))
            continue;

        if (request->type == KW_SEARCH_TYPE_EXACT)
            count = db_get_keyword_exact_matches(kw);
        else
            count = db_get_keyword_like_matches(kw);

        if (count <= 0)
            continue;

        jxd_put_bool(kw_set, kw, true);
        jxa_push(kw_list, jxs_new(kw));
    } while ((kw = strtok(NULL, " ")) != NULL);

    jxv_free(kw_set);

    return kw_list;
}

bool db_kw_search(struct kws_response *response, struct kws_request *request)
{
    int matches, i, p, limit, offset, rc, qid, last_qid, rank;

    char *question, *answer;

    sqlite3_stmt *stmt;
    char *sql;

    jx_value *kw_list, *qt_list, *qt_object, *a_list;

    limit = request->page_size;
    offset = limit * (request->page - 1);

    if (request->query == NULL || strlen(request->query) == 0)
        return false;

    kw_list = db_get_kw_list(request);

    if (kw_list == NULL) {
        return false;
    }

    matches = jxa_get_length(kw_list);

    if (matches == 0) {
        response->matches = 0;
        jxv_free(kw_list);
        return true;
    }

    sql = get_sql_with_n_params(get_answers_sql, matches);

    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

    if (db_get_error()) {
        db_set_error_msg("prepare: [%s]", sqlite3_errstr(db_rc));

        jxv_free(kw_list);
        free(sql);

        return false;
    }

    for (i = 0, p = 1; i < matches; i++, p++) {
        char *keyword = jxs_get_str(jxa_get(kw_list, i));

        if (!db_bind_text(stmt, p, keyword)) {
            db_set_error_msg("query: [%s] bind: [%s] error: [%s]", sql, keyword, sqlite3_errstr(db_rc));

            db_finalize(stmt);
            jxv_free(kw_list);
            free(sql);

            return false;
        }
    }

    if (!db_bind_int(stmt, p++, limit)) {
        db_set_error_msg("query: [%s] bind: [%d] error: [%s]", sql, limit, sqlite3_errstr(db_rc));

        db_finalize(stmt);
        jxv_free(kw_list);
        free(sql);

        return false;
    }

    if (!db_bind_int(stmt, p++, offset)) {
        db_set_error_msg("query: [%s] bind: [%d] error: [%s]", sql, offset, sqlite3_errstr(db_rc));

        db_finalize(stmt);
        jxv_free(kw_list);
        free(sql);

        return false;
    }

    qt_list = jxa_new(10);
    qt_object = NULL;
    last_qid = -1;

    while ((rc = db_step(stmt)) != SQLITE_DONE) {
        if (rc != SQLITE_ROW) {
            return false;
        }

        qid = sqlite3_column_int(stmt, 0);
        question = (char *)sqlite3_column_text(stmt, 1);
        rank = sqlite3_column_int(stmt, 2);
        answer = (char *)sqlite3_column_text(stmt, 3);

        if (last_qid != qid) {
            qt_object = jxd_new();

            jxd_put_string(qt_object, "question", question);
            jxd_put_number(qt_object, "rank", rank);
            jxd_put(qt_object, "answers", jxa_new(10));

            jxa_push(qt_list, qt_object);

            last_qid = qid;
        }

        a_list = jxd_get(qt_object, "answers");

        jxa_push(a_list, jxs_new(answer));
    }

    db_finalize(stmt);

    response->result = qt_list;
    response->matches = matches;

    jxv_free(kw_list);

    free(sql);

    return true;
}

void db_set_error_msg(const char *fmt, ...)
{
    va_list ap;

    int r;

    va_start(ap, fmt);
    r = vsnprintf(db_error_msg, DB_ERROR_MSG_SIZE, fmt, ap);
    va_end(ap);

    if (r >= DB_ERROR_MSG_SIZE) {
        db_error_msg[DB_ERROR_MSG_SIZE - 1] = '\0';
    }
}

void db_set_path(const char *fmt, ...)
{
    va_list ap;

    int r;

    va_start(ap, fmt);
    r = vsnprintf(db_path, DB_PATH_SIZE, fmt, ap);
    va_end(ap);

    if (r >= DB_PATH_SIZE) {
        db_path[DB_PATH_SIZE - 1] = '\0';
    }
}

bool db_open()
{
    if ((db_rc = sqlite3_open_v2(db_path, &db, SQLITE_OPEN_READWRITE, NULL)) != SQLITE_OK) {
        db_set_error_msg("db_open: [%s]", sqlite3_errstr(db_rc));
        return false;
    }

    sqlite3_busy_timeout(db, 100);

    return true;
}

bool db_close()
{
    if (db == NULL)
        return false;

    db_clear_cache();

    sqlite3_close_v2(db);

    return true;
}

bool db_get_error()
{
    return !(db_rc == SQLITE_OK || db_rc == SQLITE_DONE || db_rc == SQLITE_ROW);
}

const char *db_get_error_msg()
{
    return db_error_msg;
}

enum db_status db_exec_sql(const char *sql)
{
    sqlite3_stmt *stmt;

    enum db_status status = DB_STATUS_ERROR;

    if (db == NULL)
        return false;

    db_rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

    if (db_get_error()) {
        db_set_error_msg("prepare: %s", sqlite3_errstr(db_rc));
        return status;
    }

    db_rc = sqlite3_step(stmt);

    if (db_get_error()) {
        db_set_error_msg("step: %s", sqlite3_errstr(db_rc));
        goto exit;
    }   

    status = DB_STATUS_OK;

exit:
    sqlite3_finalize(stmt);
    return status;
}

enum db_status db_begin()
{
    sqlite3_stmt *stmt;

    char *sql = "begin immediate;";

    enum db_status status = DB_STATUS_ERROR;

    int attempts = 0;

    if (_db_in_transaction) {
        return DB_STATUS_IN_TRANSACTION;
    }

    _db_in_transaction = true;

    sqlite3_busy_timeout(db, 100);

    db_rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

    if (db_get_error()) {
       db_set_error_msg("prepare: %s", sqlite3_errstr(db_rc));
       return DB_STATUS_ERROR;
    }

    while (attempts++ < 30) {
        if (attempts > 10) {
            sqlite3_busy_timeout(db, 500);
        }
        else if (attempts > 20) {
            sqlite3_busy_timeout(db, 1000);
        }

        db_rc = sqlite3_step(stmt);

        if (db_rc == SQLITE_BUSY) {
            sqlite3_reset(stmt);
            continue;
        }

        if (db_get_error()) {
            break;
        }

        status = DB_STATUS_OK;
    }

    if (status != DB_STATUS_OK) {
        db_set_error_msg("step: %s\n", sqlite3_errstr(db_rc));
    }

    sqlite3_finalize(stmt);

    return status;
}

enum db_status db_rollback()
{
    enum db_status status;

    if (!_db_in_transaction) {
        return DB_STATUS_NOT_IN_TRANSACTION;
    }

    status = db_exec_sql("rollback;");

        _db_in_transaction = false;

    return status;
}

enum db_status db_commit()
{
    enum db_status status;

    if (!_db_in_transaction) {
        return DB_STATUS_IN_TRANSACTION;
    }

    _db_in_transaction = false;

    status = db_exec_sql("commit;");

    return status;
}

sqlite3_stmt *db_get_stmt(enum db_action action)
{
    struct db_stmt item = db_stmt_cache[action];

    if (item.stmt == NULL) {
        db_rc = sqlite3_prepare_v2(db, item.sql, -1, &item.stmt, NULL);

        if (db_get_error()) {
            db_set_error_msg("prepare: %s", sqlite3_errstr(db_rc));
            return NULL;
        }
    }

    return item.stmt;
}

void db_clear_cache()
{
    int i;

    for (i = 0; i < DB_ACTION_GUARD; i++) {
        if (db_stmt_cache[i].stmt != NULL) {
            sqlite3_finalize(db_stmt_cache[i].stmt);
            db_stmt_cache[i].stmt = NULL;
        }
    }
}

bool db_bind_null(sqlite3_stmt *stmt, int index)
{
    db_rc = sqlite3_bind_null(stmt, index); 

    if (db_get_error()) {
        db_set_error_msg("bind: %s", sqlite3_errstr(db_rc));
        return false;
    }

    return true;
}

bool db_bind_text(sqlite3_stmt *stmt, int index, const char *str)
{
    db_rc = sqlite3_bind_text(stmt, index, str, -1, NULL);

    if (db_get_error()) {
        db_set_error_msg("bind: %s", sqlite3_errstr(db_rc));
        return false;
    }

    return true;
}

bool db_bind_int(sqlite3_stmt *stmt, int index, int value)
{
    db_rc = sqlite3_bind_int(stmt, index, value);

    if (db_get_error()) {
        db_set_error_msg("bind: %s", sqlite3_errstr(db_rc));
        return false;
    }

    return true;
}

bool db_bind_int_foreign_key(sqlite3_stmt *stmt, int index, int value)
{
    if (value <= 0) {
        return db_bind_null(stmt, index);
    }

    return db_bind_int(stmt, index, value);
}

int db_step(sqlite3_stmt *stmt)
{
    db_rc = sqlite3_step(stmt);

    if (db_get_error()) {
        db_set_error_msg("step: %s", sqlite3_errstr(db_rc));
    }   

    return db_rc;
}

bool db_reset(sqlite3_stmt *stmt)
{
    db_rc = sqlite3_reset(stmt);

    return !db_get_error();
}

bool db_finalize(sqlite3_stmt *stmt)
{
    db_rc = sqlite3_finalize(stmt);

    return !db_get_error();
}
