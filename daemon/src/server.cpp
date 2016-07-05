/*
 * server.cpp
 * 
 * Interface library to the rover kernel driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Copyright (C) 2016 Tomasz Chadzynski
 */

#include "server.h"

Server::Server():
    inQueue(std::make_shared<RoverNet::NetMsgQueue>()),
    outQueue(std::make_shared<RoverNet::NetMsgQueue>()),
    uc0Service(std::make_unique<DeviceUC0Service>(inQueue, outQueue)),
    netService(std::make_unique<RoverNet::NetService>(inQueue, outQueue, &videoStreamManager))
{
}

void Server::Start()
{
    videoStreamManager.Start();
    uc0Service->Init();
    netService->Init();
}

void Server::Stop()
{
    netService->Stop();
    uc0Service->Stop();
    videoStreamManager.Stop();
}
