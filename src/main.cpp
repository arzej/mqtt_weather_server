#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <sstream>
#include <string.h>
#include <iostream>
#include <unistd.h>
#include <exception>
#include <glib.h>
#include "dbstorage.h"
#include "mqttserver.h"
#include "lock.h"
#include "httpd.h"
#include "rest.h"

static int run = 1;

void handle_signal(int s) {
    std::cout << "signal stop" << std::endl;
    run = 0;
}

int main(int argc, char *argv[]) {
    int i, j;
    GMainLoop* ml = NULL;
    mqttserver mqtt;
    lock l;
    if (!l.isLocked()) {
        std::cout << "pid file is locked, another instance is running?" << std::endl;
        return 0;
    }
    const char *dbpath=NULL;
    for (i=1; i<argc; i++) {
        if (!strcmp(argv[i], "--dbpath")) {
            if (strlen(argv[i+1])>0) {
                dbpath = argv[i+2];
            }
        }
        if (!strcmp(argv[i], "-f")||!strcmp(argv[i], "--daemon")) {
            for (j = i+1; j<argc; j++) {
                argv[j-1] = argv[j];
            }
            argv[argc - 1] = NULL;
            if (fork()!=0) {
                exit(0);
            }
            execv(argv[0], argv);
            exit(0);
        }
    }
    sleep(1);
    if (dbpath==NULL) {
        dbpath = "test.db";
    }
    std::cout << "params:" << std::endl;
    std::cout << "dbpath:" << dbpath << std::endl;
    dbstorage::getInstance().init("test.db");
    mqtt.start();
    httpd::getInstance().init();
    rest::getInstance().init();
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    ml = g_main_loop_new(NULL, FALSE);
    while(run) {
        g_main_loop_run(ml);
	}
    std::cout << "bye bye" << std::endl;
    rest::getInstance().term();
    httpd::getInstance().term();
    mqtt.term();
    dbstorage::getInstance().term();
	return 0;
}
