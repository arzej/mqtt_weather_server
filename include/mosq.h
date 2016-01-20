#ifndef MOSQ_H
#define	MOSQ_H
#include <mosquittopp.h>

#define mqtt_host "localhost"
//#define mqtt_host "192.168.0.99"
#define mqtt_port 1883

class myMosq : public mosqpp::mosquittopp
{
private:
	const char    *host;
	const char    *id;
	const char    *topic;
	int            port;
	int            keepalive;

	void on_connect(int rc);
	void on_disconnect(int rc);
	void on_publish(int mid);
	void on_message(const struct mosquitto_message *message);
	void on_subscribe(int mid, int qos_count, const int *granted_qos);
    void on_unsubscribe(int mid);
	void on_log(int level, const char *str);
	void on_error();
public:
	myMosq(const char *id, const char * _topic, const char *host, int port);
	~myMosq();
	bool send_message(const char * _message);
    bool collect(const  char * _topic);
};

#endif	/* MOSQ_H */
