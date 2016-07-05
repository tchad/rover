/*
 * videostreammanager.cpp
 * 
 * Interface library to the rover kernel driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Copyright (C) 2016 Tomasz Chadzynski
 */

/*
 * NOTE: dummy implementation
 */

#include "videostreammanager.h"

namespace {
    bool running = false;
}


void VideoStreamManager::Start()
{
    running = true;
}

void VideoStreamManager::Stop()
{
    running = false;
}

bool VideoStreamManager::Running() const noexcept
{
    return running;
}

uint16_t VideoStreamManager::Port() const 
{
    return 1234;
}


