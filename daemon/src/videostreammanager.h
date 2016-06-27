/*
 * videostreammanager.h
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Copyright (C) 2016 Tomasz Chadzynski
 */

#ifndef _VIDEO_STREAM_MANAGER_H_
#define _VIDEO_STREAM_MANAGER_H_

#include <cstdint>

class VideoStreamManager
{
    public:
        explicit VideoStreamManager() = default;
        VideoStreamManager(const VideoStreamManager&) = delete;
        VideoStreamManager& operator=(const VideoStreamManager&) = delete;

        void Start();
        void Stop();
        bool Running() const noexcept;

        uint16_t Port() const;
};


#endif /* _VIDEO_STREAM_MANAGER_H_ */
