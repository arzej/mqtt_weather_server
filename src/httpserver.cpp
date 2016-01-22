#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <libsoup/soup.h>
#include "httpd.h"
#include "httpserver.h"

#define HTTP_PAGE_ERROR_404 "<html>Error 404 - The requested URL was not found</html>"

void soupServerCallback0(SoupServer *server,
                         SoupMessage *msg,
                         const char *path,
                         GHashTable *query,
                         SoupClientContext *client,
                         gpointer user_data) {
    httpserver* httpd = (httpserver*)user_data;
    if (httpd) {
        httpd->soupServerCallback1(server, msg, path, query, client, NULL);
    }
}

httpserver::httpserver() : m_soupServer(NULL) {

}

httpserver::~httpserver() {

}

int httpserver::init() {
    if (m_soupServer == NULL) {
        m_soupServer = soup_server_new(SOUP_SERVER_PORT, 8080, NULL);
        if (m_soupServer) {
            soup_server_add_handler(m_soupServer, NULL,
                                    soupServerCallback0, this, NULL);
            soup_server_run_async(m_soupServer);
        }
    }
    return 0;
}

int httpserver::term() {
    return 0;
}

int httpserver::registerHttpCallbackRequest(const char* path, httpd::uri_handler_f handler, void* param) {
    int result = -1;
    if (path) {
        std::string pathString(path);
        http_req_paths_t::iterator it = m_reqPaths.find(pathString);
        if (it == m_reqPaths.end() && handler) {
            http_handler_t h = {handler, param};
            std::pair<http_req_paths_t::iterator, bool> ret;
            ret = m_reqPaths.insert(http_req_paths_pair_t(path,h));
            if (ret.second) {
                result = 0;
            }
        } else if (it != m_reqPaths.end() && handler == NULL) {
            m_reqPaths.erase(it);
            result = 0;
        }
    }
    return result;
}

typedef std::pair<std::string, std::string> FormKeyValuePair;
static void ghashTable2Map(gpointer key, gpointer value, gpointer userData) {
    std::map<std::string, std::string>* mapformParams = (std::map<std::string, std::string>*)userData;
    if (mapformParams && key && value) {
        std::string keyString = (char*)key;
        std::string valueString = (char*)value;
        mapformParams->insert(FormKeyValuePair(keyString, valueString));
    }
}

void soup_message_set_redirect_ll (SoupMessage *msg, guint status_code, const char *redirect_uri) {
    SoupURI *location;
    char *location_str;

    location = soup_uri_new_with_base (soup_message_get_uri (msg), redirect_uri);
    g_return_if_fail (location != NULL);

    soup_message_set_status (msg, status_code);
    location_str = soup_uri_to_string (location, FALSE);
    soup_message_headers_replace (msg->response_headers, "Location",
                      location_str);
    g_free (location_str);
    soup_uri_free (location);
}

void httpserver::soupServerCallback1(SoupServer *server,
                                     SoupMessage *msg,
                                     const char *path,
                                     GHashTable *query,
                                     SoupClientContext *client,
                                     gpointer user_data)
{
    httpd::req_method_t method;
    httpd::req_data_t req;
    SoupBuffer *fileBuffer = NULL;
    SoupBuffer *indexBuffer = NULL;
    if (strcmp(msg->method, SOUP_METHOD_GET) == 0) {
        method = httpd::REQ_METHOD_GET;
    }
    else if (strcmp(msg->method, SOUP_METHOD_POST) == 0) {
        method = httpd::REQ_METHOD_POST;
    } else {
      soup_message_set_status (msg, SOUP_STATUS_NOT_FOUND);
      soup_message_set_response (msg, "text/html", SOUP_MEMORY_COPY, HTTPDCORE_PAGE_ERROR_404, strlen(HTTPDCORE_PAGE_ERROR_404));
      return;
    }
    req.reqPath = path;
    {
        SoupURI* soupUri = soup_message_get_uri(msg);
        if (soupUri) {
            char* uriString = soup_uri_to_string(soupUri, TRUE);
            if (uriString) {
                req.fullURIString = uriString;
                free(uriString);
            }
        }
    }
    if (msg->method == SOUP_METHOD_POST)
    {
      const char* contentType = soup_message_headers_get_content_type(msg->request_headers, NULL);
      if (contentType)
      {
        if (strcmp("application/x-www-form-urlencoded", contentType) == 0)
        {
          SoupBuffer* buffer = soup_message_body_flatten(msg->request_body);
          if (buffer)
          {
            GHashTable* formHTable = soup_form_decode(buffer->data);
            if (formHTable)
            {
              // convert GHashTable to std::map
              g_hash_table_foreach(formHTable, ghashTable2Map, &req.formParams);
              g_hash_table_destroy(formHTable);
              formHTable = NULL;
            }
            soup_buffer_free(buffer);
          }
        }
        else if (strcmp("multipart/form-data", contentType) == 0)
        {
          GHashTable* fHashTable = soup_form_decode_multipart(msg, "fileToUpload", NULL, NULL, &fileBuffer);
          if (fHashTable)
          {
            if (fileBuffer)
            {
              soup_buffer_get_data(fileBuffer, (const unsigned guint8**)&req.fileBuffer, (gsize*)&req.fileBufferLen);
            }
            g_hash_table_destroy(fHashTable);
          }
          else if (fileBuffer)
          {
            soup_buffer_free(fileBuffer);
            fileBuffer = NULL;
          }
          fHashTable = soup_form_decode_multipart(msg, "index", NULL, NULL, &indexBuffer);
          if (fHashTable)
          {
            if (indexBuffer)
            {
              soup_buffer_get_data(indexBuffer, (const unsigned guint8**)&req.indexBuffer, (gsize*)&req.indexBufferLen);
            }
            g_hash_table_destroy(fHashTable);
          }
          else if (indexBuffer)
          {
            soup_buffer_free(indexBuffer);
            indexBuffer = NULL;
          }
        }
        else if (strcmp("text/plain", contentType) == 0)
        {
          SoupBuffer* buffer = soup_message_body_flatten(msg->request_body);
          if (buffer)
          {
            unsigned int length = 0;
            while (length++ < buffer->length)
            {
              req.plainBuffer.push_back(buffer->data[length]);
            }
            soup_buffer_free(buffer);
          }
        }
      }
    }
    // get cookie
    {
      const char* cookieHdr = soup_message_headers_get_one(msg->request_headers, "Cookie");
      if (cookieHdr)
      {
        SoupCookie* cookie = soup_cookie_parse(cookieHdr, NULL);
        if (cookie)
        {
          const char* cookieName = soup_cookie_get_name(cookie);
          const char* cookieValue = soup_cookie_get_value(cookie);

          if (cookieName)
          {
            req.cookieSet = true;
            strncpy(req.cookie.name, cookieName, sizeof(req.cookie.name));
            req.cookie.name[sizeof(req.cookie.name)-1] = 0;
            req.cookie.body[0] = 0;
          }
          if (cookieValue)
          {
            strncpy(req.cookie.body, cookieValue, sizeof(req.cookie.body));
            req.cookie.body[sizeof(req.cookie.body)-1] = 0;
          }
        }
      }
    }
    {
        //OS::CLock lock(m_reqPathsSem);
        http_req_paths_t::iterator it = m_reqPaths.find(req.reqPath);
        bool reqHandled=false;

        if (it != m_reqPaths.end())
        {
          httpd::response_data_t response;
          if ((*it).second.handler(method, req, response, (*it).second.handlerParam) == 0)
          {
            soup_message_headers_append(msg->response_headers, "Cache-Control", "no-cache");
            if(msg->response_headers)
            {
              soup_message_headers_set_content_disposition(msg->response_headers, response.contentDisposition,NULL);
            }
            soup_message_set_response (msg, response.contentType, SOUP_MEMORY_COPY, response.body, response.bodyLen);
            soup_message_set_status (msg, SOUP_STATUS_OK);
            if (response.justLogIn)
            {
                if (response.LogInPath.length())
                {
                    soup_message_set_redirect_ll(msg,
                                                 302,
                                                 response.LogInPath.c_str());
                }
                else
                {
                    soup_message_set_redirect_ll(msg,
                                                 302,
                                                 "main.html");
                }
            }
            /* workaround START */
            /* : cookie was not set with old method for Chrome / IE when connecting by Host Name */
            /*   that's weird... */
            if (response.setCookie)
            {
                char buf[128];
                sprintf(buf,"%s=%s", response.cookie.name, response.cookie.body );
                soup_message_headers_append (msg->response_headers, "Set-Cookie", buf );
            }
            /* workaround END */
            reqHandled = true;
            /*
            if (response.setCookie)
            {
              SoupURI* soupUri = soup_message_get_uri(msg);
              const char* hostString = NULL;
              if (soupUri) { hostString = soup_uri_get_host(soupUri); }

              SoupCookie* cookie = soup_cookie_new(response.cookie.name, response.cookie.body, hostString?hostString:"*", "/", -1);
              if (cookie)
              {
                GSList *cookieList = NULL;

                soup_cookie_set_http_only(cookie, true);
                cookieList = g_slist_append(cookieList, cookie);
                printf("cookieList=%d\n",cookieList);
                if (cookieList) { soup_cookies_to_response(cookieList, msg); }
                soup_cookie_free(cookie);
                if (cookieList) { g_slist_free(cookieList); }
              }
            }*/
          }
        }
        if (!reqHandled) {
            soup_message_set_status (msg, SOUP_STATUS_NOT_FOUND);
            soup_message_set_response (msg, "text/html", SOUP_MEMORY_COPY, HTTP_PAGE_ERROR_404, strlen(HTTP_PAGE_ERROR_404));
        }
    }
    if (fileBuffer) {
        soup_buffer_free(fileBuffer);
        fileBuffer = NULL;
    }
}
