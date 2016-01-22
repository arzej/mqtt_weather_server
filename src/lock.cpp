#include <unistd.h>
#include <string.h>
#include <iostream>
//for pid file
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include "lock.h"

lock::lock() {
    m_fd=create_pid_file("mqttws", "mqttws.pid");
}

lock::~lock() {
    remove_pid_file(m_fd, "mqttws.pid");
}

lock& lock::getInstance() {
  static lock l;
  return l;
}

bool lock::isLocked() {
    return (m_fd>=0?true:false);
}

int lock::create_pid_file(const char *progName, const char *pidFile) {
    const int BUF_SIZE = 100;
    mode_t m=umask( 0 );
    int fd=open(pidFile, O_RDWR|O_CREAT, 0666);
    umask(m);
    if (fd>=0) {
        if (flock(fd, LOCK_EX|LOCK_NB)<0) {
            std::cout << "Unable to lock PID file" << pidFile << std::endl;
            close(fd);
            fd=-1;
        } else {
            char buf[BUF_SIZE];
            snprintf(buf, BUF_SIZE, "%ld\n", (long) getpid());
            if (write(fd, buf, strlen(buf)) != (signed int)strlen(buf)) {
                std::cout << "Unable to write PID file" << pidFile << std::endl;
                close( fd );
                fd = -1;
            } else {
                std::cout << "PID file is successfully locked" << std::endl;
            }
        }
    } else {
        std::cout << "Could not open PID file " << pidFile << std::endl;
    }
    return fd;
}

void lock::remove_pid_file(int fd, char const *pidFile)
{
    if (fd<0) {
        std::cout << "wrong params " << fd << " " << pidFile << std::endl;
        return;
    }
    remove(pidFile);
    close(fd);
}
