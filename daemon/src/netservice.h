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
#include "videostreammanager.h"


namespace RoverNet
{
    class NetService
    {
        public:
            explicit NetService(NetMsgQueueShrPtr incomingQueue,
                    NetMsgQueueShrPtr outgoingQueue, 
                    const VideoStreamManager* const vidStreamMgr);
            NetService(const NetService&) = delete;
            NetService& operator=(const NetService&) = delete;

            void Init();
            void Stop();

        private:
            NetMsgQueueShrPtr inQueue;
            NetMsgQueueShrPtr outQueue;

            pthread_t threadDeviceStatus;
            pthread_t threadNetworkIncoming;
            pthread_t threadNetworkOutgoing;

            static void* ThreadDeviceStatusProcedure(void *arg);
            static void* ThreadNetworkIncomingProcedure(void *arg);
            static void* ThreadNetworkOutgoingProcedure(void *arg);

            /* NetService does not hold ownership iver this pointer */
            const VideoStreamManager* const videoStreamManager;
    };
};


#endif /* _NET_SERVICE_H_ */
