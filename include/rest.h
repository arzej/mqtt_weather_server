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
  static int on_www_index_html(httpd::req_method_t method,
                               const httpd::req_data_t& req,
                               httpd::response_data_t& response,
                               void* param);




};

#endif
