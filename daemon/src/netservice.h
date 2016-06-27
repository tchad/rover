/*
 * netservice.h
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Copyright (C) 2016 Tomasz Chadzynski
 */

#ifndef _NET_SERVICE_H_
#define _NET_SERVICE_H_

#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "nettypes.h"


namespace RoverNet
{
    class NetService
    {
        public:
            explicit NetService(NetMsgQueueShrPtr incomingQueue,
                    NetMsgQueueShrPtr outgoingQueue);
            NetService(const NetService&) = delete;
            NetService& operator=(const NetService&) = delete;

            void Init();
            void Stop();
            bool Running() const noexcept;
            void ThreadJoin();

        private:
            NetMsgQueueShrPtr inQueue;
            NetMsgQueueShrPtr outQueue;

            pthread_t threadDeviceStatus;
            pthread_t threadNetworkIncoming;
            pthread_t threadNetworkOutgoing;

            static void ThreadDeviceStatusProcedure(void *arg);
            static void ThreadNetworkIncomingProcedure(void *arg);
            static void ThreadNetworkOutgoingProcedure(void *arg);
    };
};


#endif /* _NET_SERVICE_H_ */
