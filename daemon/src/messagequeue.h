/*
 * messagequeue.h
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Copyright (C) 2016 Tomasz Chadzynski
 */

#ifndef _MESSAGE_QUEUE_H_
#define _MESSAGE_QUEUE_H_

#include <queue>
#include <pthread.h>

template<typename T>
class MessageQueue
{
    public:
        using size_type = typename std::queue<T>::size_type;

        explicit MessageQueue() = default;
        MessageQueue(const MessageQueue<T>&) = delete;
        MessageQueue<T>& operator=(const MessageQueue<T>&) = delete;

        void Enqueue(const T& item);
        void Enqueue(T&& item);
        T Dequeue();
        bool Empty() const noexcept;
        size_type Count() const noexcept;

    private:
        std::queue<T> queue;
        pthread_mutex_t queueMutex;
        pthread_cond_t  queueCond;
};

#endif /* _MESSAGE_QUEUE_H_ */

