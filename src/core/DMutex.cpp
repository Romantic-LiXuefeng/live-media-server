#include "DMutex.hpp"

DMutex::DMutex()
{
    pthread_cond_init(&m_cond, NULL);
    pthread_mutex_init(&m_mutex, NULL);
}

DMutex::~DMutex()
{
    pthread_cond_destroy(&m_cond);
    pthread_mutex_destroy(&m_mutex);
}

bool DMutex::lock()
{
   if (pthread_mutex_lock(&m_mutex) != 0) {
       return false;
   }
   return true;
}

void DMutex::unlock()
{
    pthread_mutex_unlock(&m_mutex);
}

bool DMutex::wait()
{
    if(pthread_cond_wait(&m_cond, &m_mutex) != 0){
        return false;
    }
    return true;
}

void DMutex::signal()
{
    pthread_cond_signal(&m_cond);
}

/***************************************************************/

DMutexLocker::DMutexLocker(DMutex *mutex)
    : m_mutex(mutex)
{
    m_mutex->lock();
}

DMutexLocker::~DMutexLocker()
{
    m_mutex->unlock();
}
