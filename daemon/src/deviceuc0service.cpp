/*
 * deviceuc0service.cpp
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Copyright (C) 2016 Tomasz Chadzynski
 */

#include <syslog.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#include "deviceuc0service.h"
#include "util.h"
#include "logging.h"


DeviceUC0Service::DeviceUC0Service(RoverNet::NetMsgQueueShrPtr incomingQueue,
        RoverNet::NetMsgQueueShrPtr outgoingQueue):
    deviceHandler(nullptr),
    inQueue(incomingQueue),
    outQueue(outgoingQueue),
    distanceRequestPending(false)
{
    PTHREAD_GUARD( pthread_mutex_init(&deviceLockMutex, NULL) );
    PTHREAD_GUARD( pthread_mutex_init(&distanceMonitorMutex, NULL) );
    PTHREAD_GUARD( pthread_cond_init(&distanceMonitorCond, NULL) );

    delayedMessage.msgType = RoverNet::MessageType::INVALID;
}

DeviceUC0Service::~DeviceUC0Service()
{
/*
 * This release call should never happen
 * normally the device resoureces hsould be freed in
 * the Stop() method. This is a safeguard against exceptions
 * raised within the class structure.
 */
    if(deviceHandler != nullptr) {
        release_device_rover(deviceHandler);
    }

    pthread_mutex_destroy(&deviceLockMutex);
    pthread_mutex_destroy(&distanceMonitorMutex);
    pthread_cond_destroy(&distanceMonitorCond);
}

void DeviceUC0Service::Init()
{
    delayedMessage.msgType = RoverNet::MessageType::INVALID;

    deviceHandler = alloc_device_rover();
    if(deviceHandler == NULL) THROW_RUNTIME_MSG("Unable to allocate device handler");

    if(EXIT_SUCCESS != init_device_rover(deviceHandler)) THROW_RUNTIME_MSG("Unable to initialize device");

    PTHREAD_GUARD( pthread_create(&threadIncomingCommand, NULL, ThreadIncomingCommandProcedure, this) );
    PTHREAD_GUARD( pthread_create(&threadDelayedMessage, NULL, ThreadDelayedMessageProcedure, this) );
    PTHREAD_GUARD( pthread_create(&threadDistanceMonitor, NULL, ThreadDistanceMonitorProcedure, this) );
}

void DeviceUC0Service::Stop()
{
/*
 * NOTE: since pthread_cancel will return either 0 or ESRCH which in both cases we do not care
 * therefore pthread_cancel does not have to be guarded
 */
    pthread_cancel(threadDistanceMonitor);
    pthread_cancel(threadDelayedMessage);
    pthread_cancel(threadIncomingCommand);

    PTHREAD_GUARD( pthread_join(threadDistanceMonitor, NULL) );
    PTHREAD_GUARD( pthread_join(threadDelayedMessage, NULL) );
    PTHREAD_GUARD( pthread_join(threadIncomingCommand, NULL) );

    if(deviceHandler != nullptr) {
        release_device_rover(deviceHandler);
    }

}

void* DeviceUC0Service::ThreadIncomingCommandProcedure(void *arg)
{
/*
 * NOTE: Thread does not have the ownership over the pointer to DeviceUC0Service
 * and should not free it
 */
    DeviceUC0Service* dev = static_cast<DeviceUC0Service*>(arg);
    try {
        while(true) {
            RoverNet::Message msg = dev->inQueue->Dequeue();
            switch(msg.msgType) {
                case RoverNet::MessageType::CMD_SET_LEFT_WHEEL_SPEED:
                case RoverNet::MessageType::CMD_SET_RIGHT_WHEEL_SPEED:
                case RoverNet::MessageType::CMD_SET_WHEELS_SPEED:
                case RoverNet::MessageType::CMD_STOP:
                    {
                        PTHREAD_GUARD( pthread_mutex_lock(&(dev->deviceLockMutex)) );
                        dev->delayedMessage = msg;
                        PTHREAD_GUARD( pthread_mutex_unlock(&(dev->deviceLockMutex)) );
                    }
                    break;
                case RoverNet::MessageType::REQ_WHEELS_STATE:
                    {
                        device_state devState;
                        int responseStatus;

                        PTHREAD_GUARD( pthread_mutex_lock(&(dev->deviceLockMutex)) );
                        responseStatus = get_device_state(dev->deviceHandler, &devState);
                        PTHREAD_GUARD( pthread_mutex_unlock(&(dev->deviceLockMutex)) );

                        if(EXIT_SUCCESS != responseStatus) THROW_RUNTIME_MSG("error reading device state");

                        RoverNet::Message response;
                        response.msgType = RoverNet::MessageType::MSG_WHEELS_STATE;
                        response.data.wheelsState = { devState.left_wheel_speed,
                                                      devState.right_wheel_speed,
                                                      devState.wheel_max_speed,
                                                      devState.wheel_min_speed };

                        dev->outQueue->Enqueue(response);
                    }
                    break;
                case RoverNet::MessageType::REQ_DISTANCE:
                    {
                        PTHREAD_GUARD( pthread_mutex_lock(&(dev->distanceMonitorMutex)) );
                        dev->distanceRequestPending = true;
                        PTHREAD_GUARD( pthread_mutex_unlock(&(dev->distanceMonitorMutex)) );
                        PTHREAD_GUARD( pthread_cond_signal(&(dev->distanceMonitorCond)) );
                    }
                    break;
                default:
                    {
                        std::stringstream ss;
                        ss << "Unsupported message received " << msg.msgType;
                        THROW_RUNTIME_MSG(ss.str().c_str());
                    }
            };

            pthread_testcancel();
        }
    }
    catch(const std::exception &e) {
        syslog(LOG_ERR, LOG_EXCEPT("DeviceUC0Service", e));
        kill(getpid(), SIGTERM);
    }
    catch(...) {
        syslog(LOG_ERR, LOG_MSG("DeviceUC0Service", "unknown exception"));
        kill(getpid(), SIGTERM);
    }
}

//thread cleanup routines
namespace {
    void CleanupMutexUnlock(void *mutex)
    {
        if(0 != pthread_mutex_unlock(static_cast<pthread_mutex_t*>(mutex)))
            throw std::runtime_error("DeviceUC0Service::CleanupMutexUnlock: Unable to unlock mutex");
    }
};

void* DeviceUC0Service::ThreadDelayedMessageProcedure(void *arg)
{
/*
 * NOTE: The purpose of this thread is to offload the communication between
 * driver and the uc0 occuring for the commands that set wheels speed.
 * Later the driver should implement its own thread that will handle the load
 * Commands that set wheels speed that occur in short amount of time between them
 * can be safely overwritten.
 *
 * NOTE: Thread does not have the ownership over the pointer to DeviceUC0Service
 * and should not free it
 */
    DeviceUC0Service* dev = static_cast<DeviceUC0Service*>(arg);
    int responseStatus;

    try {
        while(true) {

            PTHREAD_GUARD( pthread_mutex_lock(&(dev->deviceLockMutex)) );
            pthread_cleanup_push(CleanupMutexUnlock, &(dev->deviceLockMutex));

            switch(dev->delayedMessage.msgType) {
                case RoverNet::MessageType::INVALID:
                    responseStatus = EXIT_SUCCESS;
                    break;
                case RoverNet::MessageType::CMD_SET_LEFT_WHEEL_SPEED:
                case RoverNet::MessageType::CMD_SET_RIGHT_WHEEL_SPEED:
                    {
                        device_state devState;
                        responseStatus = get_device_state(dev->deviceHandler, &devState);
/*
 * NOTE: If the response was not EXIT_SUCCESS we immediately break and let the later portion to throw 
 */
                        if(EXIT_SUCCESS != responseStatus)
                            break;

                        if(dev->delayedMessage.msgType == RoverNet::MessageType::CMD_SET_LEFT_WHEEL_SPEED) {
                            responseStatus = set_wheel_speed(dev->deviceHandler, 
                                    dev->delayedMessage.data.wheelsState.leftWheelSpeed,
                                    devState.right_wheel_speed);
                        } else {
                            responseStatus = set_wheel_speed(dev->deviceHandler,
                                    devState.left_wheel_speed,
                                    dev->delayedMessage.data.wheelsState.rightWheelSpeed);
                        }
                    }
                    break;
                case RoverNet::MessageType::CMD_SET_WHEELS_SPEED:
                    responseStatus = set_wheel_speed(dev->deviceHandler,
                                dev->delayedMessage.data.wheelsState.leftWheelSpeed,
                                dev->delayedMessage.data.wheelsState.rightWheelSpeed);
                    break;
                case RoverNet::MessageType::CMD_STOP:
                    responseStatus = set_wheel_stop(dev->deviceHandler);
                    break;
                default:
                    {
                        std::stringstream ss;
                        ss << "Unsupported message received " << dev->delayedMessage.msgType;
                        THROW_RUNTIME_MSG(ss.str().c_str());
                    }
            };

            dev->delayedMessage.msgType = RoverNet::MessageType::INVALID;
            pthread_cleanup_pop(0);
            PTHREAD_GUARD( pthread_mutex_unlock(&(dev->deviceLockMutex)) );
            if(EXIT_SUCCESS != responseStatus) THROW_RUNTIME_MSG("error sending command to the device");

/*
 * TODO: Review this section and possibly relax the fail condition
 */
            timespec t1, t2;
            t1.tv_sec = DEV_CMD_SEND_T_SEC;
            t1.tv_nsec = DEV_CMD_SEND_T_NSEC;
            if( -1 == nanosleep(&t1, &t2)) THROW_RUNTIME();
        }
    }
    catch(const std::exception &e) {
        syslog(LOG_ERR, LOG_EXCEPT("DeviceUC0Service", e));
        kill(getpid(), SIGTERM);
    }
    catch(...) {
        syslog(LOG_ERR, LOG_MSG("DeviceUC0Service", "unknown exception"));
        kill(getpid(), SIGTERM);
    }
}

void* DeviceUC0Service::ThreadDistanceMonitorProcedure(void *arg)
{
    DeviceUC0Service* dev = static_cast<DeviceUC0Service*>(arg);
    bool proceed = false;
/*
 * TODO: Improvement needed
 * NOTE: Need a copy of the structure that describe the device to avoid the need
 * to lock the deviceMutex every time we obtain the distance. 
 * The distance is coming from different device file than the general commands
 * so it does not present any collsion problem
 */
    device_rover device;

    try {
        PTHREAD_GUARD( pthread_mutex_lock(&(dev->deviceLockMutex)) );
        device = *(dev->deviceHandler);
        PTHREAD_GUARD( pthread_mutex_unlock(&(dev->deviceLockMutex)) );

        while(true) {
            PTHREAD_GUARD( pthread_mutex_lock(&(dev->distanceMonitorMutex)) );
            pthread_cleanup_push(CleanupMutexUnlock, &(dev->distanceMonitorMutex));

/*
 * NOTE: We need to release the command procedure thread as soon as possible
 * Instead requesting the distance in this conditional, defer it and release distance mutex immediately
 */
            if(dev->distanceRequestPending) {
                proceed = true;
                dev->distanceRequestPending = false;
            }
            else {
                PTHREAD_GUARD( pthread_cond_wait(&(dev->distanceMonitorCond), &(dev->distanceMonitorMutex)) );
            }

            pthread_cleanup_pop(0);

            if(0 != pthread_mutex_unlock(&(dev->distanceMonitorMutex)))
                    throw std::runtime_error("DeviceUC0Service: Unable to unlock distanceMonitorMutex");

            if(proceed) {
                proceed = false;

                RoverNet::Message response;
                response.msgType = RoverNet::MessageType::MSG_DISTANCE;

                if(EXIT_SUCCESS != read_distance(&device, &response.data.distance.distanceCM))
                    THROW_RUNTIME_MSG("Unable to obtain distance reading from device");
                
                dev->outQueue->Enqueue(response);
            }
        }
    }
    catch(const std::exception &e) {
        syslog(LOG_ERR, LOG_EXCEPT("DeviceUC0Service", e));
        kill(getpid(), SIGTERM);
    }
    catch(...) {
        syslog(LOG_ERR, LOG_MSG("DeviceUC0Service", "unknown exception"));
        kill(getpid(), SIGTERM);
    }
}

