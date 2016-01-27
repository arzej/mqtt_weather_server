#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <libsoup/soup.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include "httpd.h"
#include "httpserver.h"
#include "sem.h"

#define HTTP_PAGE_ERROR_404 "<html>Error 404 - The requested URL was not found</html>"

void soupServerCallback0(SoupServer *server,
                         SoupMessage *msg,
                         const char *path,
                         GHashTable *query,
                         SoupClientContext *context,
                         gpointer user_data) {
    httpserver* httpd = (httpserver*)user_data;
    if (httpd) {
        httpd->soupServerCallback1(server, msg, path, query, context, NULL);
    }
}

httpserver::httpserver() : m_soupServer(NULL) {

}

httpserver::~httpserver() {

}

int httpserver::init() {
    if (m_soupServer == NULL) {
        GError *error = NULL;
        m_soupServer= soup_server_new (SOUP_SERVER_SERVER_HEADER,
                                       "simple-httpd ", NULL);
		if (m_soupServer) {
            soup_server_listen_all (m_soupServer, (guint)8000,
                                    (SoupServerListenOptions)0, &error);
            soup_server_add_handler(m_soupServer,
                                    NULL,
                                    soupServerCallback0,
                                    this,
                                    NULL);
        }
    }
    return 0;
}

int httpserver::term() {
    return 0;
}

int httpserver::registerHttpCallbackRequest(const char* path,
                                            httpd::uri_handler_f handler,
                                            void* param) {
    int result = -1;
    if (path) {
        slock l(m_reqSem);
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
    std::map<std::string, std::string>* mapformParams = (std::map<std::string,
                                                         std::string>*)userData;
    if (mapformParams && key && value) {
        std::string keyString = (char*)key;
        std::string valueString = (char*)value;
        mapformParams->insert(FormKeyValuePair(keyString, valueString));
    }
}

void soup_message_set_redirect_ll (SoupMessage *msg, guint status_code,
                                   const char *redirect_uri) {
    SoupURI *location;
    char *location_str;

    location = soup_uri_new_with_base (soup_message_get_uri (msg), redirect_uri);
    g_return_if_fail (location != NULL);

    soup_message_set_status (msg, status_code);
    location_str = soup_uri_to_string (location, FALSE);
    soup_message_headers_replace (msg->response_headers,
                                  "Location",
                                  location_str);
    g_free (location_str);
    soup_uri_free (location);
}

int httpserver::do_rest(SoupServer *server,
                        SoupMessage *msg,
                        const char *path) {
    int reqHandled=-1;
    SoupBuffer *fileBuffer = NULL;
    SoupBuffer *indexBuffer = NULL;
    httpd::req_data_t req;
    httpd::req_method_t method;
    slock l(m_reqSem);
    if (strcmp(msg->method, SOUP_METHOD_GET) == 0) {
        method = httpd::REQ_METHOD_GET;
    }
    else if (strcmp(msg->method, SOUP_METHOD_POST) == 0) {
        method = httpd::REQ_METHOD_POST;
    } else {
        soup_message_set_status (msg, SOUP_STATUS_NOT_FOUND);
        soup_message_set_response (msg, "text/html",
                                        SOUP_MEMORY_COPY,
                                   HTTP_PAGE_ERROR_404,
                                   strlen(HTTP_PAGE_ERROR_404));
        return -1;
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
    if (msg->method == SOUP_METHOD_POST) {
        const char* contentType = soup_message_headers_get_content_type(
                                                    msg->request_headers, NULL);
        if (contentType) {
            if (strcmp("application/x-www-form-urlencoded", contentType) == 0) {
                SoupBuffer* buffer = soup_message_body_flatten(msg->request_body);
                if (buffer) {
                    GHashTable* formHTable = soup_form_decode(buffer->data);
                    if (formHTable) {
                        // convert GHashTable to std::map
                        g_hash_table_foreach(formHTable,
                                             ghashTable2Map,
                                            &req.formParams);
                        g_hash_table_destroy(formHTable);
                        formHTable = NULL;
                    }
                    soup_buffer_free(buffer);
                }
            }
            else if (strcmp("multipart/form-data", contentType) == 0) {
                GHashTable* fHashTable = soup_form_decode_multipart(
                                                       msg, "fileToUpload",
                                                       NULL, NULL, &fileBuffer);
                if (fHashTable) {
                    if (fileBuffer) {
                        soup_buffer_get_data(fileBuffer,
                                             (const unsigned guint8**)&req.fileBuffer,
                                             (gsize*)&req.fileBufferLen);
                    }
                    g_hash_table_destroy(fHashTable);
                }
                else if (fileBuffer) {
                    soup_buffer_free(fileBuffer);
                    fileBuffer = NULL;
                }
                fHashTable = soup_form_decode_multipart(msg, "index",
                                                        NULL, NULL,
                                                       &indexBuffer);
                if (fHashTable) {
                    if (indexBuffer) {
                        soup_buffer_get_data(indexBuffer,
                                      (const unsigned guint8**)&req.indexBuffer,
                                      (gsize*)&req.indexBufferLen);
                    }
                    g_hash_table_destroy(fHashTable);
                }
                else if (indexBuffer) {
                    soup_buffer_free(indexBuffer);
                    indexBuffer = NULL;
                }
            }
            else if (strcmp("text/plain", contentType) == 0) {
                SoupBuffer* buffer = soup_message_body_flatten(msg->request_body);
                if (buffer) {
                    unsigned int length = 0;
                    while (length++ < buffer->length) {
                        req.plainBuffer.push_back(buffer->data[length]);
                    }
                    soup_buffer_free(buffer);
                }
            }
        }
    }
    http_req_paths_t::iterator it = m_reqPaths.find(req.reqPath);
    if (it != m_reqPaths.end()) {
      httpd::response_data_t response;
      if ((*it).second.handler(method, req,
          response, (*it).second.handlerParam) == 0) {
        soup_message_headers_append(msg->response_headers, "Cache-Control", "no-cache");
        if(msg->response_headers) {
            soup_message_headers_set_content_disposition(msg->response_headers,
                                                         response.contentDisposition,
                                                         NULL);
        }
        soup_message_set_response (msg,
                                   response.contentType,
                                   SOUP_MEMORY_COPY,
                                   response.body,
                                   response.bodyLen);
        soup_message_set_status (msg, SOUP_STATUS_OK);
        if (response.setCookie) {
            char buf[128];
            sprintf(buf,"%s=%s", response.cookie.name, response.cookie.body );
            soup_message_headers_append (msg->response_headers, "Set-Cookie", buf );
        }
        reqHandled = 0;
      }
    }
    return reqHandled;
}

bool httpserver::path_exists(const char* name)
{
    struct stat buffer;
    return (stat (name, &buffer) == 0);
}

void httpserver::soupServerCallback1(SoupServer *server,
                                     SoupMessage *msg,
                                     const char *path,
                                     GHashTable *query,
                                     SoupClientContext *context,
                                     gpointer user_data) {
    char *file_path=NULL;
    SoupMessageHeadersIter iter;
    const char *name=NULL, *value=NULL;

    g_print ("%s %s HTTP/1.%d\n", msg->method, path, soup_message_get_http_version (msg));
    soup_message_headers_iter_init (&iter, msg->request_headers);
    while (soup_message_headers_iter_next (&iter, &name, &value)) {
        g_print ("%s: %s\n", name, value);
    }
    if (msg->request_body->length) {
        g_print ("%s\n", msg->request_body->data);
    }
    if (do_rest(server, msg, path)!=0) {
        if (!path_exists("/usr/share/mqttws")) {
            file_path = g_strdup_printf ("./www/%s", path);
        } else {
            file_path = g_strdup_printf ("/usr/share/mqttws/%s", path);
        }
        if (msg->method == SOUP_METHOD_GET || msg->method == SOUP_METHOD_HEAD) {
            do_get (server, msg, file_path);
        }
        else if (msg->method == SOUP_METHOD_PUT) {
            do_put (server, msg, file_path);
        } else {
            soup_message_set_status (msg, SOUP_STATUS_NOT_FOUND);
            soup_message_set_response (msg, "text/html", SOUP_MEMORY_COPY,
                                       HTTP_PAGE_ERROR_404,
                                       strlen(HTTP_PAGE_ERROR_404));
        }
        g_free (file_path);
    }
    g_print ("  -> %d %s\n\n", msg->status_code, msg->reason_phrase);
}

static int compare_strings (gconstpointer a, gconstpointer b)
{
	const char **sa = (const char **)a;
	const char **sb = (const char **)b;
	return strcmp (*sa, *sb);
}

static GString *get_directory_listing (const char *path)
{
	GPtrArray *entries;
	GString *listing;
	char *escaped;
	DIR *dir;
	struct dirent *dent;
	int i;

	entries = g_ptr_array_new ();
	dir = opendir (path);
	if (dir) {
		while ((dent = readdir (dir))) {
			if (!strcmp (dent->d_name, ".") ||
			    (!strcmp (dent->d_name, "..") &&
			     !strcmp (path, "./")))
				continue;
			escaped = g_markup_escape_text (dent->d_name, -1);
			g_ptr_array_add (entries, escaped);
		}
		closedir (dir);
	}
	g_ptr_array_sort (entries, (GCompareFunc)compare_strings);
	listing = g_string_new ("<html>\r\n");
	escaped = g_markup_escape_text (strchr (path, '/'), -1);
	g_string_append_printf (listing, "<head><title>Index of %s</title></head>\r\n", escaped);
	g_string_append_printf (listing, "<body><h1>Index of %s</h1>\r\n<p>\r\n", escaped);
	g_free (escaped);
	for (i = 0; i < (int)entries->len; i++) {
		g_string_append_printf (listing, "<a href=\"%s\">%s</a><br>\r\n",
					(char *)entries->pdata[i],
					(char *)entries->pdata[i]);
		g_free (entries->pdata[i]);
	}
	g_string_append (listing, "</body>\r\n</html>\r\n");
	g_ptr_array_free (entries, TRUE);
	return listing;
}

void httpserver::do_put(SoupServer *server,
                        SoupMessage *msg,
                        const char *path) {
    struct stat st;
	FILE *f;
	gboolean created = TRUE;

	if (stat (path, &st) != -1) {
		const char *match = soup_message_headers_get_one (msg->request_headers, "If-None-Match");
		if (match && !strcmp (match, "*")) {
			soup_message_set_status (msg, SOUP_STATUS_CONFLICT);
			return;
		}
		if (!S_ISREG (st.st_mode)) {
			soup_message_set_status (msg, SOUP_STATUS_FORBIDDEN);
			return;
		}
		created = FALSE;
	}
	f = fopen (path, "w");
	if (!f) {
		soup_message_set_status (msg, SOUP_STATUS_INTERNAL_SERVER_ERROR);
		return;
	}
	fwrite (msg->request_body->data, 1, msg->request_body->length, f);
	fclose (f);
	soup_message_set_status (msg, created ? SOUP_STATUS_CREATED : SOUP_STATUS_OK);
}

void httpserver::do_get(SoupServer *server,
                        SoupMessage *msg,
                        const char *path) {
    char *slash;
	struct stat st;

	if (stat (path, &st) == -1) {
		if (errno == EPERM) {
			soup_message_set_status (msg, SOUP_STATUS_FORBIDDEN);
        } else if (errno == ENOENT) {
			soup_message_set_status (msg, SOUP_STATUS_NOT_FOUND);
		} else {
			soup_message_set_status (msg, SOUP_STATUS_INTERNAL_SERVER_ERROR);
        }
		return;
	}
	if (S_ISDIR (st.st_mode)) {
		GString *listing;
		char *index_path;
		slash = (char*)strrchr (path, '/');
		if (!slash || slash[1]) {
			char *redir_uri;
			redir_uri = g_strdup_printf ("%s/", soup_message_get_uri (msg)->path);
			soup_message_set_redirect (msg, SOUP_STATUS_MOVED_PERMANENTLY,
						                                             redir_uri);
			g_free (redir_uri);
			return;
		}
		index_path = g_strdup_printf ("%s/index.html", path);
		if (stat (index_path, &st) != -1) {
			do_get (server, msg, index_path);
			g_free (index_path);
			return;
		}
		g_free (index_path);
		listing = get_directory_listing (path);
		soup_message_set_response (msg, "text/html",
					   SOUP_MEMORY_TAKE,
					   listing->str, listing->len);
		soup_message_set_status (msg, SOUP_STATUS_OK);
		g_string_free (listing, FALSE);
		return;
	}
	if (msg->method == SOUP_METHOD_GET) {
		GMappedFile *mapping;
		SoupBuffer *buffer;
		mapping = g_mapped_file_new (path, FALSE, NULL);
		if (!mapping) {
			soup_message_set_status (msg, SOUP_STATUS_INTERNAL_SERVER_ERROR);
			return;
		}
		buffer = soup_buffer_new_with_owner (g_mapped_file_get_contents (mapping),
						     g_mapped_file_get_length (mapping),
						     mapping, (GDestroyNotify)g_mapped_file_unref);
		soup_message_body_append_buffer (msg->response_body, buffer);
		soup_buffer_free (buffer);
	} else /* msg->method == SOUP_METHOD_HEAD */ {
		char *length;

		/* We could just use the same code for both GET and
		 * HEAD (soup-message-server-io.c will fix things up).
		 * But we'll optimize and avoid the extra I/O.
		 */
		length = g_strdup_printf ("%lu", (gulong)st.st_size);
		soup_message_headers_append (msg->response_headers,
					                 "Content-Length", length);
		g_free (length);
	}
	soup_message_set_status (msg, SOUP_STATUS_OK);
}
