#ifndef SEM_H
#define	SEM_H

#include <errno.h>
#include <time.h>
#include <semaphore.h>

class sem
{
public:
    sem( unsigned int initial_value = 1 )
        { sem_init( &_sem, 0, initial_value ); }
    ~sem()
        { sem_destroy( &_sem ); }
    void lock(){sem_wait(&_sem);};
    void unlock(){sem_post(&_sem);};
private:
    sem_t _sem;
};

class slock
{
public:
    slock( sem& sync ) :
        _sync( sync )
        { _sync.lock(); }
    ~slock()
        { _sync.unlock(); }
private:
    sem& _sync;
};

#endif	/* SEM_H */
