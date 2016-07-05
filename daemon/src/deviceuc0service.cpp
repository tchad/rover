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
#include <sstream>

#include "deviceuc0service.h"
#include "exceptions.h"


DeviceUC0Service::DeviceUC0Service(RoverNet::NetMsgQueueShrPtr incomingQueue,
        RoverNet::NetMsgQueueShrPtr outgoingQueue):
    deviceHandler(nullptr),
    inQueue(incomingQueue),
    outQueue(outgoingQueue)
{
    if(0 != pthread_mutex_init(&deviceLockMutex, NULL))
        throw std::runtime_error("DeviceUC0Service: Unable to initialize deviceLockMutex");

    if(0 != pthread_mutex_init(&distanceMonitorMutex, NULL))
        throw std::runtime_error("DeviceUC0Service: Unable to initialize distanceMonitorMutex");

    if(0 != pthread_cond_init(&distanceMonitorCond, NULL))
        throw std::runtime_error("DeviceUC0Service: Unable to initialize distanceMonitorCond");

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
    if(deviceHandler == NULL) 
        throw std::runtime_error("DeviceUC0Service::Init: Unable to allocate device handler");

    if(EXIT_SUCCESS != init_device_rover(deviceHandler))
        throw std::runtime_error("DeviceUC0Service::Init: Unable to initialize device structure");

    //secure this with exception
    if( 0 != pthread_create(&threadIncomingCommand, NULL, ThreadIncomingCommandProcedure, this))
        throw std::runtime_error("DeviceUC0Service::Init: Failed to start incoming cmd thread");

    if( 0 != pthread_create(&threadDelayedMessage, NULL, ThreadDelayedMessageProcedure, this))
        throw std::runtime_error("DeviceUC0Service::Init: Failed to start delayed message thread");

    if( 0 != pthread_create(&threadDistanceMonitor, NULL, ThreadDistanceMonitorProcedure, this))
        throw std::runtime_error("DeviceUC0Service::Init: Failed to start distance monitor thread");

}

void DeviceUC0Service::Stop()
{
    if( 0 != pthread_cancel(threadDistanceMonitor))
        throw std::runtime_error("DeviceUC0Service::Stop: Failed to cancel distance monitor thread");

    if( 0 != pthread_cancel(threadDelayedMessage))
        throw std::runtime_error("DeviceUC0Service::Stop: Failed to cancel delayed message thread");

    if( 0 != pthread_cancel(threadIncomingCommand))
        throw std::runtime_error("DeviceUC0Service::Stop: Failed to cancel incoming command thread");

    if( 0 != pthread_join(threadDistanceMonitor, NULL))
        throw std::runtime_error("DeviceUC0Service::Stop: Failed to join on distance monitor thread");

    if( 0 != pthread_join(threadDelayedMessage, NULL))
        throw std::runtime_error("DeviceUC0Service::Stop: Failed to join delayed message thread");

    if( 0 != pthread_join(threadIncomingCommand, NULL))
        throw std::runtime_error("DeviceUC0Service::Stop: Failed to join incoming command thread");

}

void* DeviceUC0Service::ThreadIncomingCommandProcedure(void *arg)
{
/*
 * NOTE: Thread does not have the ownership over the pointer to DeviceUC0Service
 * and should not free it
 */
    DeviceUC0Service* dev = static_cast<DeviceUC0Service*>(arg);
    try {
        bool run = true;
        while(run) {
            RoverNet::Message msg = dev->inQueue->Dequeue();
            switch(msg.msgType) {
                case RoverNet::MessageType::CMD_SET_LEFT_WHEEL_SPEED:
                case RoverNet::MessageType::CMD_SET_RIGHT_WHEEL_SPEED:
                case RoverNet::MessageType::CMD_SET_WHEELS_SPEED:
                case RoverNet::MessageType::CMD_STOP:
                    {
                        if(0 != pthread_mutex_lock(&(dev->deviceLockMutex)))
                            throw std::runtime_error("DeviceUC0Service: Unable to lock deviceLockMutex");
                        
                        dev->delayedMessage = msg;

                        if(0 != pthread_mutex_unlock(&(dev->deviceLockMutex)))
                            throw std::runtime_error("DeviceUC0Service: Unable to unlock deviceLockMutex");
                    }
                    break;
                case RoverNet::MessageType::REQ_WHEELS_STATE:
                    {
                        device_state devState;
                        int responseStatus;

                        if(0 != pthread_mutex_lock(&(dev->deviceLockMutex)))
                            throw std::runtime_error("DeviceUC0Service: Unable to lock deviceLockMutex");
                        
                        responseStatus = get_device_state(dev->deviceHandler, &devState);

                        if(0 != pthread_mutex_unlock(&(dev->deviceLockMutex)))
                            throw std::runtime_error("DeviceUC0Service: Unable to unlock deviceLockMutex");

                        if(EXIT_SUCCESS != responseStatus)
                            throw std::runtime_error("DeviceUC0Service: error reading device state");

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
                        if(0 != pthread_cond_signal(&(dev->distanceMonitorCond)))
                                throw std::runtime_error("DeviceUC0Service: Unable to signal distance monitor cond");
                    }
                    break;
                default:
                    {
                        std::stringstream ss;
                        ss << "Unsupported message received " << msg.msgType;
                        throw std::runtime_error(ss.str());
                    }
            };

            pthread_testcancel();
        }
    }
    catch(const std::exception &e) {
        syslog(LOG_ERR, "DeviceUC0Service: Incomming command thread raised exception.\n");
        syslog(LOG_ERR, e.what());

        kill(getpid(), SIGTERM);
    }
    catch(...) {
        syslog(LOG_ERR, "DeviceUC0Service: Incomming command thread raised unknown exception.\n");

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
 * NOTE: The purpose of this thread is mainly to offload the communication between
 * driver and the uc0 occuring for the commands that set wheels speed.
 * Later the driver should implement its own thread that will handle the load
 * Commands that set wheels speed that occur in short amount of time between them
 * can be safely overwritten.
 *
 * NOTE: Thread does not have the ownership over the pointer to DeviceUC0Service
 * and should not free it
 */
    DeviceUC0Service* dev = static_cast<DeviceUC0Service*>(arg);
    try {
        while(true) {

            if(0 != pthread_mutex_lock(&(dev->deviceLockMutex)))
                throw std::runtime_error("DeviceUC0Service: Unable to lock deviceLockMutex");

            int responseStatus;
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
                        throw std::runtime_error(ss.str());
                    }
            };

            dev->delayedMessage.msgType = RoverNet::MessageType::INVALID;
            pthread_cleanup_pop(0);

            if(0 != pthread_mutex_unlock(&(dev->deviceLockMutex)))
                throw std::runtime_error("DeviceUC0Service: Unable to unlock deviceLockMutex");

            if(EXIT_SUCCESS != responseStatus)
                throw std::runtime_error("DeviceUC0Service: error sending command to the device");

            timespec t1, t2;
            t1.tv_sec = 0;
            t1.tv_nsec = 100,000,000; // 100 msec
            
/*
 * NOTE: Review this section and possibly relax the fail condition
 */
            if( -1 == nanosleep(&t1, &t2))
                throw std::runtime_error("DeviceUC0Service: nanosleep failed");
            
        }
    }
    catch(const std::runtime_error &e) {
        syslog(LOG_ERR, "DeviceUC0Service: Delayed message thread raised exception.\n");
        syslog(LOG_ERR, e.what());

        kill(getpid(), SIGTERM);
    }
    catch(...) {
        syslog(LOG_ERR, "DeviceUC0Service: Delayed message thread raised unknown exception.\n");

        kill(getpid(), SIGTERM);
    }
}

void* DeviceUC0Service::ThreadDistanceMonitorProcedure(void *arg)
{
    try {
        while(true) {
            //TODO: Implement me
        }
    }
    catch(const std::runtime_error &e) {
        syslog(LOG_ERR, "DeviceUC0Service: Distance monitor thread raised exception.\n");
        syslog(LOG_ERR, e.what());

        kill(getpid(), SIGTERM);
    }
    catch(...) {
        syslog(LOG_ERR, "DeviceUC0Service: Distance monitor thread raised unknown exception.\n");

        kill(getpid(), SIGTERM);
    }
}

