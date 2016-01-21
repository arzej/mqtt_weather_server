#ifndef MQTTSERVER_H
#define	MQTTSERVER_H

#include <iostream>
#include <thread>
#include <algorithm>
#include <vector>

#include <mosquittopp.h>

#include "ctask.h"
#include "mosq.h"

class mqttserver : public ctask {
public:
    mqttserver();
    ~mqttserver();
    void init();
    void term();
private:
    bool run=true;
    class myMosq *mosq=NULL;
    virtual void doTask();

};


#endif	/* MQTTSERVER_H */

