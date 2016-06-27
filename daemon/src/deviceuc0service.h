/*
 * deviceuc0service.h
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Copyright (C) 2016 Tomasz Chadzynski
 */

#ifndef _DEVICE_UC0_SERVICE_
#define _DEVICE_UC0_SERVICE_

#include <pthread.h>
#include <uc.h>

#include "nettypes.h"

class DeviceUC0Service
{
    public:
        explicit DeviceUC0Service(RoverNet::NetMsgQueueShrPtr incomingQueue,
                RoverNet::NetMsgQueueShrPtr outgoingQueue);
        DeviceUC0Service(const DeviceUC0Service&) = delete;
        DeviceUC0Service& operator=(const DeviceUC0Service&) = delete;

        void Init();
        void Stop();
        bool Running() const noexcept;
        void ThreadJoin();

    private:
        RoverNet::NetMsgQueueShrPtr inQueue;
        RoverNet::NetMsgQueueShrPtr outQueue;

        pthread_t threadIncomingQueue;
        pthread_t threadDeviceControl;
        pthread_t threadDistanceMonitor;

        static void ThreadIncomingQueueProcedure(void *arg);
        static void ThreadDeviceControlProcedure(void *arg);
        static void ThreadDistanceMonitorProcedure(void *arg);

        device_rover* deviceHandler;

        RoverNet::Message currentMessage;
        pthread_mutex_t currentMessageMutex;
};


#endif /* _DEVICE_UC0_SERVICE_ */
