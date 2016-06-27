/*
 * server.h
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Copyright (C) 2016 Tomasz Chadzynski
 */

#ifndef _SERVER_H_
#define _SERVER_H_

#include <memory>

#include "nettypes.h"
#include "deviceuc0service.h"
#include "netservice.h"

class Server
{
    public:
        explicit Server() = default;
        Server(const Server&) = delete;
        Server& operator=(const Server&) = delete;

    private:
        RoverNet::NetMsgQueueShrPtr inQueue;
        RoverNet::NetMsgQueueShrPtr outQueue;

        std::unique_ptr<DeviceUC0Service> uc0Service;
        std::unique_ptr<RoverNet::NetService> netService;

        void Start();
        void Stop();
};

#endif /* _SERVER_H_ */
