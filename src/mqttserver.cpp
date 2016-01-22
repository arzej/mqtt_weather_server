#include <iostream>
#include <thread>
#include <algorithm>
#include <vector>
#include "mqttserver.h"

mqttserver::mqttserver() {
    mosq = new myMosq("collector", "sensor/#", mqtt_host, mqtt_port);
}

mqttserver::~mqttserver() {
    if(mosq!=NULL) {
        delete mosq;
    }
}

void mqttserver::init() {
    run=true;
    start();
}

void mqttserver::term() {
    run=false;
    mosq->disconnect();
    stop();
}

void mqttserver::doTask() {
    int rc;
    while(run) {
        if (mosq!=NULL) {
            rc = mosq->loop(1, 1);
            if(rc) {
                mosq->reconnect();
            }
        }
    }
}
