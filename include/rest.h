#ifndef REST_H
#define	REST_H

#include "httpd.h"

class rest
{
public:
    static rest& getInstance();
    int init();
    int term();
protected:
    rest();
    ~rest();
    static int on_current(httpd::req_method_t method,
                          const httpd::req_data_t& req,
                          httpd::response_data_t& response,
                          void* param);
    int on_current1(httpd::req_method_t method, const httpd::req_data_t& req,
                    httpd::response_data_t& response);


    static const unsigned HTML_RESPONSE_MAX_SIZE = 102400 * 5;
    char* m_htmlResponseBuffer;
};

#endif
