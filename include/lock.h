#ifndef LOCK_H
#define	LOCK_H

class lock {
private:
    int m_fd;
    int create_pid_file(const char *progName, const char *pidFile);
    void remove_pid_file(int fd, char const *pidFile);
public:
    static lock& getInstance();
    bool isLocked();
    lock();
	~lock();
};

#endif	/* LOCK_H */

