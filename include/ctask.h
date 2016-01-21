#ifndef CTASK_H
#define	CTASK_H

#include <unistd.h>
#include <iostream>
#include <thread>

class ctask{
public:
    ctask() : t() {}
    ~ctask(){
        stop();
    }
    void start() {
        // This will start the thread. Notice move semantics!
        t = std::thread(&ctask::doTask,this);
    }
    void stop() {
        stop_thread = true;
        if(t.joinable()) {
            t.join();
        }
    }
private:
    bool stop_thread = false; // Super simple thread stopping.
    std::thread t;
    virtual void doTask() {
        while (!stop_thread) {
            // Do something useful, e.g:
            std::this_thread::sleep_for( std::chrono::seconds(1) );
        }
    }
};



#endif	/* CTASK_H */

