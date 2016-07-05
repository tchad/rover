/*
 * messagequeue.cpp
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Copyright (C) 2016 Tomasz Chadzynski
 */

#include "messagequeue.h"
#include "exceptions.h"

//thread cleanup routines
namespace {
    void CleanupMutexUnlock(void *mutex)
    {
        if(0 != pthread_mutex_unlock(static_cast<pthread_mutex_t*>(mutex)))
            throw std::runtime_error("MessageQueue::CleanupMutexUnlock: Unable to unlock queue mutex");
    }
};

template<typename T>
MessageQueue<T>::MessageQueue()
{

    if(0 != pthread_mutex_init(&queueMutex, NULL))
       throw std::runtime_error("MessageQueue: Unable to initialize mutex");

    if(0 != pthread_cond_init(&queueCond, NULL))
       throw std::runtime_error("MessageQueue: Unable to initialize condition variable");
}

template<typename T>
void MessageQueue<T>::Enqueue(const T& item)
{
    if(0 != pthread_mutex_lock(&queueMutex))
        throw std::runtime_error("MessageQueue::Enqueue: Failed to lock the queue mutex");

    queue.push(item);

    if( 0 != pthread_mutex_unlock(&queueMutex))
        throw std::runtime_error("MessageQueue::Enqueue: Failed to unlock the queue mutex");

    if( 0 != pthread_cond_signal(&queueCond))
        throw std::runtime_error("MessageQueue::Enqueue: Failed to signal cond variable");


}

template<typename T>
T MessageQueue<T>::Dequeue()
{
    T retval;
    if( 0 != pthread_mutex_lock(&queueMutex))
        throw std::runtime_error("MessageQueue::Dequeue: Failed to lock the queue mutex");

    pthread_cleanup_push(CleanupMutexUnlock, &queueMutex);

    while(queue.empty()) {
        if( 0 != pthread_cond_wait(&queueCond, &queueMutex)) {
            throw std::runtime_error("MessageQueue::Dequeue: Failed to execute cond wait");
        }
    }

    retval = queue.front();
    queue.pop();

    pthread_cleanup_pop(0);

    if(0 != pthread_mutex_unlock(&queueMutex))
        throw std::runtime_error("MessageQueue::Dequeue: Failed to unlock the queue mutex");

    return retval;
}

template<typename T>
MessageQueue<T>::~MessageQueue()
{
    pthread_cond_destroy(&queueCond);
    pthread_mutex_destroy(&queueMutex);
}
