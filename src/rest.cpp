#include "rest.h"

rest::rest() {
  srand(time(NULL));
}

rest::~rest() {
}

rest& rest::getInstance() {
  static rest ui;
  return ui;
}

int rest::init() {
    int result;
    result  = httpd::getInstance().registerHttpCallbackRequest("/",
                                                               on_www_index_html,
                                                               NULL);
    result |= httpd::getInstance().registerHttpCallbackRequest("/index.html",
                                                               on_www_index_html,
                                                               NULL);
    return result;
}

int rest::term() {
    return 0;
}

int rest::on_www_index_html(httpd::req_method_t method,
                            const httpd::req_data_t& req,
                            httpd::response_data_t& response,
                            void* param) {
    return 0;
}
