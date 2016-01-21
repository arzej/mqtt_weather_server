#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "httpd.h"
#include "httpserver.h"

httpd::httpd() : m_httpserver(NULL) {
}

httpd::~httpd() {
  assert(m_httpserver == NULL);
  if (m_httpserver) {
    delete m_httpserver;
  }
}

httpd& httpd::getInstance()
{
  static httpd hd;
  return hd;
}

int httpd::init() {
  int result = -1;
  m_httpserver = new httpserver;
  if (m_httpserver) {
    if (!m_httpserver->init()) {
      result = 0;
    } else {
      delete m_httpserver;
      m_httpserver = NULL;
    }
  }
  return result;
}

int httpd::term() {
  if (m_httpserver) {
    m_httpserver->term();
    delete m_httpserver;
    m_httpserver = NULL;
  }
  return 0;
}

int httpd::registerHttpCallbackRequest(const char* path,
                                       uri_handler_f handler,
                                       void* param /* = NULL */) {
  return m_httpserver->registerHttpCallbackRequest(path, handler, param);
}
