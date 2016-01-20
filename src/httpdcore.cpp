#include <libsoup/soup.h>
#include "httpd.h"
#include "httpdcore.h"

httpdcore::httpdcore() : m_soupServer(NULL)
{

}

httpdcore::~httpdcore()
{

}

int httpdcore::init()
{

}

int httpdcore::term()
{

}

int httpdcore::registerHttpCallbackRequest(const char* path, httpd::uri_handler_f handler, void* param)
{

}
