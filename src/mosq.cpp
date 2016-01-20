#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <sstream>
#include <string.h>
#include <iostream>
#include <unistd.h>

#include "mosq.h"
#include "dbstorage.h"

bool startsWith(const std::string& haystack, const std::string& needle) {
    return needle.length() <= haystack.length()
        && equal(needle.begin(), needle.end(), haystack.begin());
}

myMosq::myMosq(const char * _id,const char * _topic, const char * _host,
               int _port) : mosquittopp(_id)
{
	mosqpp::lib_init();      // Mandatory initialization for mosquitto library
	this->keepalive = 60;    // Basic configuration setup for myMosq class
	this->id = _id;
	this->port = _port;
	this->host = _host;
	this->topic = _topic;
	connect_async(host,      // non blocking connection to broker request
                  port,
  	              keepalive);
	loop_start();            // Start thread managing connection / publish / subscribe
};

myMosq::~myMosq() {
	loop_stop();             // Kill the thread
	mosqpp::lib_cleanup();   // Mosquitto library cleanup
}

bool myMosq::collect(const  char * _topic) {
	int ret = this->subscribe(NULL, _topic);
	return ( ret == MOSQ_ERR_SUCCESS );
}

bool myMosq::send_message(const  char * _message) {
	// Send message - depending on QoS, mosquitto lib managed re-submission this the thread
	//
	// * NULL : Message Id (int *) this allow to latter get status of each message
	// * topic : topic to be used
	// * lenght of the message
	// * message
	// * qos (0,1,2)
	// * retain (boolean) - indicates if message is retained on broker or not
	// Should return MOSQ_ERR_SUCCESS
	int ret = publish(NULL, this->topic, strlen(_message),_message, 1, false);
	return ( ret == MOSQ_ERR_SUCCESS );
}

void myMosq::on_disconnect(int rc) {
	std::cout << ">> myMosq - disconnection(" << rc << ")" << std::endl;
}

void myMosq::on_connect(int rc) {
	if ( rc == 0 ) {
		std::cout << ">> myMosq - connected with server" << std::endl;
        std::cout << ">> myMosq - subscribe:" << topic << std::endl;
        subscribe(NULL, topic);
	} else {
 		std::cout << ">> myMosq - Impossible to connect with server(" << rc << ")" << std::endl;
	}
}

void myMosq::on_publish(int mid) {
	std::cout << ">> myMosq - Message (" << mid << ") succeed to be published " << std::endl;
}

void myMosq::on_subscribe(int mid, int qos_count, const int *granted_qos) {
	printf("Subscription succeeded.mid %d qos %d\n", mid, qos_count);
}

void myMosq::on_unsubscribe(int mid) {
    printf("on_unsubscribe. mid: %d\n", mid);
}

void myMosq::on_log(int level, const char *str) {
    printf("on_log. level: %d msg:%s\n", level, str);
}

void myMosq::on_error() {
    printf("on_error.\n");
}


void myMosq::on_message(const struct mosquitto_message *message) {
	char buf[51];
	std::cout << "on_message" << std::endl;
    if (startsWith(message->topic, "sensor/")) {
        std::stringstream ss(message->topic);
        std::string substr, topic, location, sensor;
        getline( ss, substr, '/' );
        if (substr.length()) {
            topic = substr;
        }
        getline( ss, substr, '/' );
        if (substr.length()) {
            location = substr;
        }
        getline( ss, substr, '/' );
        if (substr.length()) {
            sensor = substr;
        }
        if (topic.length()&&location.length()&&sensor.length()) {
            std::cout << topic    << std::endl;
            std::cout << location << std::endl;
            std::cout << sensor   << std::endl;
            memset(buf, 0, 51*sizeof(char));
            memcpy(buf, message->payload, 50*sizeof(char));
            dbstorage::getInstance().add(location.c_str(),sensor.c_str(),buf);
        }
    }
}
