#pragma once
#include <sqlite3.h>

int serializeQueryResult (sqlite3* db, sqlite3_stmt *res, void (*write)(const char*));