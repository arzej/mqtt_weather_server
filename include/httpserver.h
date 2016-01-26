#ifndef HTTPSERVER_H
#define	HTTPSERVER_H

#include <libsoup/soup.h>
#include <map>
#include <string>
#include "sem.h"

class httpserver
{
public:
    httpserver();
    ~httpserver();

    int init();
    int term();
    int registerHttpCallbackRequest(const char* path,
                                  httpd::uri_handler_f handler,
                                  void* param);
    friend void soupServerCallback0(SoupServer *server,
                                  SoupMessage *msg,
                                  const char *path,
                                  GHashTable *query,
                                  SoupClientContext *client,
                                  gpointer user_data);
private:
    typedef struct http_handler_s {
        httpd::uri_handler_f handler;
        void* handlerParam;
    } http_handler_t;
    void soupServerCallback1(SoupServer *server, SoupMessage *msg,
                             const char *path, GHashTable *query,
                             SoupClientContext *client, gpointer user_data);
    SoupServer* m_soupServer;
    typedef std::map<std::string, http_handler_t> http_req_paths_t;
    typedef std::pair<std::string, http_handler_t> http_req_paths_pair_t;
    http_req_paths_t m_reqPaths;
    sem m_reqSem;
};

#endif	/* HTTPDCORE_H */

