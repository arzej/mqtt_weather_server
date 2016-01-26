#ifndef DBSTORAGE_H
#define	DBSTORAGE_H

#include <sqlite3.h>

class dbstorage {
private:
    sqlite3       *m_db;
	const char    *m_path;
    bool           is_open;
    int            open(const char *path=NULL);
    int            close();
    bool           table_exists();
    int            table_create();
    int            table_remove();
    int            triggers_create();
    int            table_average_create();
public:
    dbstorage();
    void setPath(const char *path);
	dbstorage(const char *path);
    static dbstorage& getInstance();
    int init(const char *path);
    int init();
    int term();
    int add(const char *location, const char *sensor, const char *value);
	~dbstorage();
};


#endif	/* DBSTORAGE_H */

