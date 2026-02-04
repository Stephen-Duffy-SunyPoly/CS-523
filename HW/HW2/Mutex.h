//Author: Stephen Duffy duffysd
//HW2 pthread count 3s
//CS523 4:00pm tr
#ifndef CS_523_MUTEX_H
#define CS_523_MUTEX_H
#include <pthread.h>

//simple class to manage the mutex and make the code using it look cleaner
class Mutex {
    pthread_mutex_t m_lock_{};
public:
    Mutex() {
        if (pthread_mutex_init(&m_lock_, nullptr) !=0) {
            throw std::runtime_error("mutex init has failed");
        }
    }

    ~Mutex() {
        pthread_mutex_destroy(&m_lock_);
    }

    void lock() {
        pthread_mutex_lock(&m_lock_);
    }

    void unlock() {
        pthread_mutex_unlock(&m_lock_);
    }
};


#endif //CS_523_MUTEX_H