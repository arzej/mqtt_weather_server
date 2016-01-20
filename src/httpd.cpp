#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "httpd.h"
#include "httpdcore.h"

httpd::httpd() : m_httpdcore(NULL)
{

}

httpd::~httpd()
{
  assert(m_httpdcore == NULL);
  if (m_httpdcore)
  {
    delete m_httpdcore;
  }
}

httpd& httpd::getInstance()
{
  static httpd hd;
  return hd;
}

int httpd::init()
{
  int result = -1;

  m_httpdcore = new httpdcore;
  if (m_httpdcore)
  {
    if (!m_httpdcore->init())
    {
      result = 0;
    }
    else
    {
      delete m_httpdcore;
      m_httpdcore = NULL;
    }
  }

  return result;
}

int httpd::term()
{
  if (m_httpdcore)
  {
    m_httpdcore->term();
    delete m_httpdcore;
    m_httpdcore = NULL;
  }
  return 0;
}

int httpd::registerHttpCallbackRequest(const char* path,
                                       uri_handler_f handler,
                                       void* param /* = NULL */)
{
  return m_httpdcore->registerHttpCallbackRequest(path, handler, param);
}
