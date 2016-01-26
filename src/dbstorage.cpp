#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <sstream>
#include <string.h>
#include <iostream>
#include <unistd.h>
#include "dbstorage.h"

//http://www.tutorialspoint.com/sqlite/sqlite_c_cpp.htm

static int db_callback(void *NotUsed, int argc, char **argv, char **azColName) {
   int i;
   for(i=0; i<argc; i++) {
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   printf("\n");
   return 0;
}

dbstorage::dbstorage() {
    m_path = NULL;
    m_db = NULL;
    is_open = NULL;
}

dbstorage::dbstorage(const char* path): m_path(path) {
    init();
}

dbstorage& dbstorage::getInstance() {
  static dbstorage db;
  return db;
}

int dbstorage::init(const char *path) {
    setPath(path);
    return init();
}

int dbstorage::init() {
    int rc=-1;
    if (m_path!=NULL) {
        if (open()==0) {
            if (!table_exists()) {
                if (table_create()==0) {
                    is_open=true;
                    rc = 0;
                }
            }
            table_average_create();
            triggers_create();
        }
    }
    return rc;
}

int dbstorage::term() {
    return close();
}

void dbstorage::setPath(const char *path) {
    m_path = path;
}

dbstorage::~dbstorage() {
    close();
}

int dbstorage::close() {
    if (is_open) {
        sqlite3_close(m_db);
        is_open = false;
    }
    return 0;
}

int dbstorage::open(const char *path) {
    int rc;
    /* Open database */
    rc = sqlite3_open(m_path, &m_db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(m_db));
        rc=-1;
    } else {
        fprintf(stderr, "Opened database successfully\n");
    }
    return rc;
}

int dbstorage::table_create() {
    const char *sql =    "CREATE TABLE IF NOT EXISTS sensors (" \
                         "    ID       INTEGER  PRIMARY KEY AUTOINCREMENT" \
                         "                      UNIQUE," \
                         "    TIME     DATETIME DEFAULT (datetime('now', 'localtime')),"\
                         "    LOCATION STRING," \
                         "    SENSOR   STRING," \
                         "    VALUE    DOUBLE" \
                         ");";
    int rc;
    const char* data = "Callback function called";
    char *zErrMsg = 0;
    /* Execute SQL statement */
    rc = sqlite3_exec(m_db, sql, db_callback, (void*)data, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::cout << "table_create error" << std::endl;
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        fprintf(stdout, "create table operation done successfully\n");
    }
    return rc;
}

bool dbstorage::table_exists() {
    const char *sql= "SELECT name FROM sqlite_master WHERE type='table' AND name='sensors';";
    int rc;
    bool ret = false;
    const char* data = "Callback function called";
    char *zErrMsg = 0;
    /* Execute SQL statement */
    rc = sqlite3_exec(m_db, sql, db_callback, (void*)data, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::cout << "table_exists error" << std::endl;
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        fprintf(stdout, "table exists Operation done successfully\n");
    }
    return ret;
}

int dbstorage::table_remove() {
    const char *sql = "drop table if exists sensors";
    int rc;
    const char* data = "Callback function called";
    char *zErrMsg = 0;
    // Execute SQL statement //
    rc = sqlite3_exec(m_db, sql, db_callback, (void*)data, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        fprintf(stdout, "drop table operation done successfully\n");
    }
    return rc;
}

int dbstorage::add(const char *location, const char *sensor, const char *value) {
    std::stringstream sql;
    int rc;
    const char* data = "Callback function called";
    char *zErrMsg = 0;
    // Create merged SQL statement //
    sql << "INSERT INTO sensors (LOCATION,SENSOR,VALUE) ";
    sql << "VALUES (";
    sql << "'" << location << "',";
    sql << "'" << sensor   << "',";
    sql << "'" << value    << "')";
    std::cout << sql.str() << std::endl;
    // Execute SQL statement //
    rc = sqlite3_exec(m_db, sql.str().c_str(), db_callback, (void*)data, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        fprintf(stdout, "add operation done successfully\n");
    }
    return rc;
}

int dbstorage::table_average_create() {
    const char *sql =    "CREATE TABLE IF NOT EXISTS average (" \
                         "    LOCATION STRING (32) PRIMARY KEY" \
                         "                         NOT NULL," \
                         "    SENSOR   STRING (32) NOT NULL,"\
                         "    VALUE    DOUBLE      NOT NULL" \
                         ");";
    int rc;
    const char* data = "Callback function called";
    char *zErrMsg = 0;
    /* Execute SQL statement */
    rc = sqlite3_exec(m_db, sql, db_callback, (void*)data, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::cout << "table_create error" << std::endl;
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        fprintf(stdout, "create table operation done successfully\n");
    }
    return rc;
}

int dbstorage::triggers_create() {
    std::stringstream sql;
    int rc;
    const char* data = "Callback function called";
    char *zErrMsg = 0;
    sql << "CREATE TRIGGER IF NOT EXISTS update_average_value_after_insert " << std::endl;
    sql << "AFTER INSERT "                                         << std::endl;
    sql << "ON sensors "                                           << std::endl;
    sql << "FOR EACH ROW "                                         << std::endl;
    sql << "BEGIN "                                                << std::endl;
    sql << "INSERT OR IGNORE INTO average ("                       << std::endl;
    sql << "       LOCATION,"                                      << std::endl;
    sql << "       SENSOR,"                                        << std::endl;
    sql << "       VALUE"                                          << std::endl;
    sql << "       )"                                              << std::endl;
    sql << "       VALUES ("                                       << std::endl;
    sql << "       NEW.LOCATION,"                                  << std::endl;
    sql << "       NEW.SENSOR,"                                    << std::endl;
    sql << "       NEW.VALUE"                                      << std::endl;
    sql << "       );"                                             << std::endl;
    sql << "UPDATE average "                                       << std::endl;
    sql << "SET VALUE = ("                                         << std::endl;
    sql << "       SELECT AVG(VALUE)"                              << std::endl;
    sql << "       FROM sensors "                                  << std::endl;
    sql << "       WHERE LOCATION == NEW.LOCATION AND "            << std::endl;
    sql << "             TIME >= datetime('now', '-1 hour') AND "  << std::endl;
    sql << "             SENSOR == NEW.SENSOR"                     << std::endl;
    sql << ")"                                                     << std::endl;
    sql << "WHERE LOCATION == NEW.LOCATION AND "                   << std::endl;
    sql << "SENSOR == NEW.SENSOR;"                                 << std::endl;
    sql << "END;"                                                  << std::endl;
    std::cout << sql.str() << std::endl;
    // Execute SQL statement //
    rc = sqlite3_exec(m_db, sql.str().c_str(), db_callback, (void*)data, &zErrMsg);
    if (rc!=SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        fprintf(stdout, "add operation done successfully\n");
    }
    return rc;
}

int dbstorage::getCurrent(std::list<current_mesure_t> &current_list) {
    int rc=-1;
    sqlite3_stmt *stmt;
    const char *zErrMsg = 0;
    std::stringstream sql;
    int i=0;
    int row=0;

    sql << "SELECT LOCATION,SENSOR,VALUE FROM average";
    rc = sqlite3_prepare_v2(m_db, sql.str().c_str(), sql.str().length()+1, &stmt, &zErrMsg);
    while (1) {
        int s;
        s = sqlite3_step (stmt);
        if (s == SQLITE_ROW) {
            double value;
            const unsigned char *localization;
            const unsigned char *type;
            current_mesure_t cur;
            localization  = sqlite3_column_text (stmt, 0);
            type  = sqlite3_column_text (stmt, 1);
            value = sqlite3_column_double(stmt, 2);
            printf ("%d: %s %s: %f\n", row, localization, type, value);
            cur.location = std::string((const char *)localization);
            cur.type = std::string((const char *)type);
            cur.value = value;
            current_list.push_back (cur);
            row++;
        } else if (s == SQLITE_DONE) {
            rc = 0;
            break;
        } else {
            fprintf (stderr, "Failed.\n");
            rc = -2;
            break;
        }
        i++;
    }
    return rc;
}
