#include <libsoup/soup.h>
#include "httpd.h"
#include "httpserver.h"

httpserver::httpserver() : m_soupServer(NULL)
{

}

httpserver::~httpserver()
{

}

int httpserver::init()
{
    return -1;
}

int httpserver::term()
{
    return -1;
}

int httpserver::registerHttpCallbackRequest(const char* path, httpd::uri_handler_f handler, void* param)
{
    return -1;
}
