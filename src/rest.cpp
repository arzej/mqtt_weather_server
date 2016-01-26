#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sstream>
#include <linux/ethtool.h>
#include <linux/sockios.h>
#include <iostream>
#include "dbstorage.h"
#include "cJSON.h"
#include "rest.h"

rest::rest() {
    srand(time(NULL));
    m_htmlResponseBuffer = new char[HTML_RESPONSE_MAX_SIZE];
}

rest::~rest() {
    if (m_htmlResponseBuffer) {
        delete m_htmlResponseBuffer;
        m_htmlResponseBuffer = NULL;
    }
}

rest& rest::getInstance() {
  static rest ui;
  return ui;
}

int rest::init() {
    int result=0;
    result |= httpd::getInstance().registerHttpCallbackRequest("/current",
                                                               on_current,
                                                               NULL);
    return result;
}

int rest::term() {
    return 0;
}

int rest::on_current(httpd::req_method_t method,
                     const httpd::req_data_t& req,
                     httpd::response_data_t& response,
                     void* param) {
    return rest::getInstance().on_current1(method, req, response);
}

int rest::on_current1(httpd::req_method_t method,
                      const httpd::req_data_t& req,
                      httpd::response_data_t& response) {
    int result = -1;
    if (method == httpd::REQ_METHOD_POST) {
        cJSON *answerJson = cJSON_CreateObject();
        if (answerJson != NULL) {
            cJSON *arr = cJSON_CreateArray();
            if (arr != NULL) {
                std::list<current_mesure_t> current_list;
                dbstorage::getInstance().getCurrent(current_list);
                for (std::list<current_mesure_t>::iterator it=current_list.begin();
                                               it != current_list.end(); ++it) {
                    cJSON *sensor = cJSON_CreateObject();
                    if (sensor != NULL) {
                        cJSON_AddItemToObject(sensor, "location",
                                      cJSON_CreateString(it->location.c_str()));
                        cJSON_AddItemToObject(sensor, "type",
                                          cJSON_CreateString(it->type.c_str()));
                        cJSON_AddItemToObject(sensor, "value",
                                                 cJSON_CreateNumber(it->value));
                        cJSON_AddItemToArray(arr, sensor);
                    }
                }
                cJSON_AddItemToObject(answerJson, "sensor", arr);
                char* json = cJSON_PrintUnformatted(answerJson);
                strncpy(m_htmlResponseBuffer, json, HTML_RESPONSE_MAX_SIZE-1);
                m_htmlResponseBuffer[HTML_RESPONSE_MAX_SIZE] = 0;
                free(json);
                json = NULL;
                cJSON_Delete(answerJson);
                response.contentType = "application/json";
                response.contentDisposition = "attachment";
                response.body = m_htmlResponseBuffer;
                response.bodyLen = strlen(m_htmlResponseBuffer);
                result = 0;
            } else {
                std::cerr << "json allocation error" << std::endl;
            }
        } else {
            std::cerr << "json allocation error" << std::endl;
        }
    } else {
        std::cerr << "only post implemented" << std::endl;
    }
    return result;
}
