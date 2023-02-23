/*
 * db.c
 * Copyright (c) 2023, Cory Montgomery
 */

#include <stdio.h>
#include <sqlite3.h>
#include <string.h>
#include <stdarg.h>

#include "db.h"

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
    DB_ACTION_GET_KEYWORDS_EXACT_MATCH,
    DB_ACTION_GET_KEYWORDS_LIKE_MATCH,
    DB_ACTION_GUARD
};

struct db_stmt {
    sqlite3_stmt *stmt;
    const char *sql;
}
db_stmt_cache[] =
{
    { NULL, "SELECT COUNT(*) AS CNT FROM keywords WHERE keyword = ?;" },
    { NULL, "SELECT COUNT(*) AS CNT FROM keywords WHERE like('%?%', keyword);" }
};

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
    db_rc = sqlite3_bind_text(stmt, index, str, -1, SQLITE_STATIC);

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