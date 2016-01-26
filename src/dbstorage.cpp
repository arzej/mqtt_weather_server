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

    sql << "CREATE TRIGGER update_average_value_after_insert";
    sql << "AFTER INSERT";
    sql << "ON sensors";
    sql << "FOR EACH ROW";
    sql << "BEGIN";
    sql << "INSERT OR IGNORE INTO average (";
    sql << "       LOCATION,";
    sql << "       SENSOR,";
    sql << "       VALUE";
    sql << "       )";
    sql << "       VALUES (";
    sql << "       NEW.LOCATION,";
    sql << "       NEW.SENSOR,";
    sql << "       NEW.VALUE";
    sql << "       );";
    sql << "UPDATE average";
    sql << "SET VALUE = (";
    sql << "        SELECT AVG(VALUE)";
    sql << "          FROM sensors";
    sql << "         WHERE LOCATION == NEW.LOCATION AND";
    sql << "               TIME >= datetime('now', '-1 hour') AND";
    sql << "               SENSOR == NEW.SENSOR";
    sql << ")";
    sql << "WHERE LOCATION == NEW.LOCATION AND";
    sql << "SENSOR == NEW.SENSOR;";
    sql << "END;";

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
