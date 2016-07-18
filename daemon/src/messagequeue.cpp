/*
 * messagequeue.cpp
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Copyright (C) 2016 Tomasz Chadzynski
 */

#include <stdexcept>

#include "messagequeue.h"
#include "logging.h"

//thread cleanup routines
namespace {
    void CleanupMutexUnlock(void *mutex)
    {
        PTHREAD_GUARD( pthread_mutex_unlock(static_cast<pthread_mutex_t*>(mutex)) );
    }
};

template<typename T>
MessageQueue<T>::MessageQueue()
{

    PTHREAD_GUARD( pthread_mutex_init(&queueMutex, NULL) );
    PTHREAD_GUARD( pthread_cond_init(&queueCond, NULL) );
}

template<typename T>
MessageQueue<T>::~MessageQueue()
{
    pthread_cond_destroy(&queueCond);
    pthread_mutex_destroy(&queueMutex);
}

template<typename T>
void MessageQueue<T>::Enqueue(const T& item)
{
    PTHREAD_GUARD( pthread_mutex_lock(&queueMutex) );

    queue.push(item);

    PTHREAD_GUARD( pthread_mutex_unlock(&queueMutex) );
    PTHREAD_GUARD( pthread_cond_signal(&queueCond) );
}

template<typename T>
T MessageQueue<T>::Dequeue()
{
    T retval;
    PTHREAD_GUARD( pthread_mutex_lock(&queueMutex) );

    pthread_cleanup_push(CleanupMutexUnlock, &queueMutex);

    while(queue.empty()) {
        PTHREAD_GUARD( pthread_cond_wait(&queueCond, &queueMutex) );
    }

    retval = queue.front();
    queue.pop();

    pthread_cleanup_pop(0);

    PTHREAD_GUARD( pthread_mutex_unlock(&queueMutex) );

    return retval;
}

template<typename T>
void MessageQueue<T>::Clear()
{
    PTHREAD_GUARD( pthread_mutex_lock(&queueMutex) );

    while(!queue.empty()) {
        queue.pop();
    }

    PTHREAD_GUARD( pthread_mutex_unlock(&queueMutex) );
}

template<typename T>
bool MessageQueue<T>::Empty()
{
    bool ret;

    PTHREAD_GUARD( pthread_mutex_lock(&queueMutex) );

    ret = queue.empty();

    PTHREAD_GUARD( pthread_mutex_unlock(&queueMutex) );

    return ret;
}
