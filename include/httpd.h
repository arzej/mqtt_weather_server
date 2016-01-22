#ifndef HTTPD_H
#define	HTTPD_H

#include <string>
#include <map>
#include <vector>

class httpserver;

class httpd
{
public:
  typedef enum {
      REQ_METHOD_GET,
      REQ_METHOD_POST,
  } req_method_t;

  typedef struct cookie_s {
      char body[32];
      char name[8];
  } cookie_t;

  typedef struct req_data_s {
        req_data_s() : fileBuffer(NULL), fileBufferLen(0), cookieSet(false) {}
        std::string reqPath;
        std::string fullURIString;
        std::map<std::string, std::string> formParams;
        std::vector<char> plainBuffer;
        const char* fileBuffer;
        unsigned int fileBufferLen;
        const char* indexBuffer;
        unsigned int indexBufferLen;
        bool cookieSet;
        cookie_t cookie;
  } req_data_t;

  typedef struct response_data_s {
        response_data_s() : contentType(NULL),
                            contentDisposition(NULL),
                            body(NULL), bodyLen(0),
                            setCookie(false), justLogIn(false), LogInPath("") {}
        const char* contentType;
        const char* contentDisposition;
        const char* body;
        unsigned int bodyLen;
        bool setCookie;
        cookie_t cookie;
        bool justLogIn;
        std::string LogInPath;
  } response_data_t;

  typedef int (*uri_handler_f)(req_method_t method,
                               const req_data_t& req,
                               response_data_t& response,
                               void* param);
  static httpd& getInstance();
  int init();
  int term();
  int registerHttpCallbackRequest(const char* path,
                                  uri_handler_f handler,
                                  void* param = NULL);
private:
  httpd();
  ~httpd();

  httpserver* m_httpserver;
};












#endif	/* HTTPD_H */

