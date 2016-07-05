/*
 * nettypes.h
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Copyright (C) 2016 Tomasz Chadzynski
 */

#ifndef _NET_TYPES_H_
#define _NET_TYPES_H_

#include <cstdint>
#include <memory>
#include "messagequeue.h"

namespace RoverNet
{
    enum MessageType : uint8_t
    {
        INVALID = 0x00,

        CMD_SET_LEFT_WHEEL_SPEED = 0x01,
        CMD_SET_RIGHT_WHEEL_SPEED = 0x02,
        CMD_SET_WHEELS_SPEED = 0x03,
        CMD_STOP = 0x04,

        REQ_WHEELS_STATE = 0x11,
        REQ_DISTANCE = 0x12,
        REQ_VID_STREAM_PORT = 0x13,

        MSG_WHEELS_STATE = 0x21,
        MSG_DISTANCE = 0x22,
        MSG_VID_STREAM_PORT = 0x23,
        MSG_DEV_AVAILABILITY = 0x24
    };

    enum DeviceAvailability : uint8_t
    {
        UNAVAILABLE = 0x0,
        AVAILABLE = 0x1
    };

    struct DataWheelsState
    {
        int16_t leftWheelSpeed;
        int16_t rightWheelSpeed;
        int16_t wheelMaxSpeed;
        int16_t wheelMinSpeed;
    };

    struct DataDistance
    {
        uint32_t distanceCM;
    };

    struct DataVideoStreamPort
    {
        uint16_t port;
        uint8_t running;
    };

    struct DataDeviceAvailability
    {
        uint8_t availability;
    };

    struct Message
    {
        MessageType msgType;

        union 
        {
           DataWheelsState wheelsState;
           DataDistance distance;
           DataVideoStreamPort videoStreamPort;
           DataDeviceAvailability deviceAvailability;
        } data;
    };

    using NetMsgQueue = MessageQueue<Message>;
    using NetMsgQueueShrPtr = std::shared_ptr<NetMsgQueue>;
};

#endif /* _NET_TYPES_H_ */
