/*
 * netservice.cpp
 * 
 * Interface library to the rover kernel driver
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
#include <sys/socket.h>
#include <arpa/inet.h>

#include "netservice.h"
#include "util.h"
#include "logging.h"

//thread cleanup routines
namespace {
    void CleanupSocketProc(void *sock)
    {
        int s = *(static_cast<int*>(sock));
        *(static_cast<int*>(sock)) = -1;
        if(0 != close(s)) THROW_RUNTIME();
    }
};

namespace RoverNet 
{
    NetService::NetService(NetMsgQueueShrPtr incomingQueue, 
                           NetMsgQueueShrPtr outgoingQueue,
                           const VideoStreamManager* const vidStreamMgr):
        inQueue(incomingQueue),
        outQueue(outgoingQueue),
        videoStreamManager(vidStreamMgr),
        clientConnectedSocket(-1)
    {
        PTHREAD_GUARD( pthread_mutex_init(&clientConnectedMutex, NULL) );
    }

    NetService::~NetService()
    {
        pthread_mutex_destroy(&clientConnectedMutex);
    }

    void NetService::Init()
    {
        PTHREAD_GUARD( pthread_create(&threadDeviceStatus, NULL, ThreadDeviceStatusProcedure, this) );
        PTHREAD_GUARD( pthread_create(&threadNetworkIncoming, NULL, ThreadNetworkIncomingProcedure, this) );
        PTHREAD_GUARD( pthread_create(&threadNetworkOutgoing, NULL, ThreadNetworkOutgoingProcedure, this) );
    }

    void NetService::Stop()
    {
/*
 * NOTE: since pthread_cancel will return either 0 or ESRCH which in both cases we do not care
 * therefore pthread_cancel does not have to be guarded
 */
        pthread_cancel(threadDeviceStatus);
        pthread_cancel(threadNetworkIncoming);
        pthread_cancel(threadNetworkOutgoing);

        PTHREAD_GUARD( pthread_join(threadDeviceStatus, NULL) );
        PTHREAD_GUARD( pthread_join(threadNetworkIncoming, NULL) );
        PTHREAD_GUARD( pthread_join(threadNetworkOutgoing, NULL) );
    }

    void* NetService::ThreadDeviceStatusProcedure(void *arg)
    {
        NetService *netServ = static_cast<NetService*>(arg);
        try {

            int bcastSocket;
            int bcastEnable = 1;
            sockaddr_in bcastAddr;
            bcastAddr.sin_family = AF_INET;
            bcastAddr.sin_port = htons(SERVER_UDP_AVAL_BCAST_PORT);

            if( 1 != inet_pton(AF_INET, SERVER_UDP_AVAL_BCAST_ADDR, &bcastAddr.sin_addr.s_addr)) THROW_RUNTIME();
            if( -1 == (bcastSocket =  socket(AF_INET, SOCK_DGRAM, 0))) THROW_RUNTIME();
            if( -1 == setsockopt(bcastSocket, SOL_SOCKET, SO_BROADCAST, &bcastEnable, sizeof(bcastEnable))) THROW_RUNTIME();

            pthread_cleanup_push(CleanupSocketProc, &bcastSocket);

            
            while(true) {
                Message msg;
                msg.msgType = MessageType::MSG_DEV_AVAILABILITY;

                PTHREAD_GUARD( pthread_mutex_lock(&(netServ->clientConnectedMutex)) );

                msg.data.deviceAvailability.availability = (netServ->clientConnectedSocket != -1) 
                                                            ? DeviceAvailability::UNAVAILABLE 
                                                            : DeviceAvailability::AVAILABLE;

                PTHREAD_GUARD( pthread_mutex_unlock(&(netServ->clientConnectedMutex)) );

                msg = HostToNet(msg);
                ssize_t ret;
                ret = sendto(bcastSocket, &msg, MESSAGE_STRUCT_SIZE, 0,
                             reinterpret_cast<sockaddr*>(&bcastAddr), sizeof(bcastAddr));

                if( -1 == ret){
                    syslog(LOG_ERR, LOG_MSG_ERR("NetService"));
                }

                sleep(NET_STATUS_BCAST_T_SEC);
            }

            pthread_cleanup_pop(1);
        }
        catch(const std::exception &e) {
            syslog(LOG_ERR, LOG_EXCEPT("NetService", e));
            kill(getpid(), SIGTERM);
        }
        catch(...) {
            syslog(LOG_ERR, LOG_MSG("NetService", "unknown exception"));
            kill(getpid(), SIGTERM);
        }
    }

    void* NetService::ThreadNetworkIncomingProcedure(void *arg)
    {
        NetService *netServ = static_cast<NetService*>(arg);
        try {

            int servSocket;
            sockaddr_in servAddr;
            servAddr.sin_family = AF_INET;
            servAddr.sin_port = htons(SERVER_TCP_PORT);

            if( 1 != inet_pton(AF_INET, SERVER_IP4_ADDR, &servAddr.sin_addr.s_addr)) THROW_RUNTIME();
            if( -1 == (servSocket =  socket(AF_INET, SOCK_STREAM, 0))) THROW_RUNTIME();

            pthread_cleanup_push(CleanupSocketProc, &servSocket);

            if( 0 != bind(servSocket, reinterpret_cast<sockaddr*>(&servAddr), sizeof(servAddr))) THROW_RUNTIME();
            if( 0 != listen(servSocket, 1)) THROW_RUNTIME();
            
            while(true) {
                int clientConnectedSocketLocal;
                bool connectionPending = true;
                Message msg;
                ssize_t recvBytes;
/*
 * TODO: Add logging of the client address that connected
 */
                clientConnectedSocketLocal = accept(servSocket, NULL, NULL);

                if( -1 == clientConnectedSocketLocal) {
                    syslog(LOG_ERR, LOG_MSG_ERR("NetService"));
                    continue; //continue to next teration if accept has failed
                }

                pthread_cleanup_push(CleanupSocketProc, &clientConnectedSocketLocal);
                PTHREAD_GUARD( pthread_mutex_lock(&(netServ->clientConnectedMutex)) );
                netServ->clientConnectedSocket = clientConnectedSocketLocal;
                PTHREAD_GUARD( pthread_mutex_unlock(&(netServ->clientConnectedMutex)) );

                while(connectionPending){
                    recvBytes = recv(clientConnectedSocketLocal, &msg, MESSAGE_STRUCT_SIZE, 0);
                    if(recvBytes == MESSAGE_STRUCT_SIZE) {
                        msg = NetToHost(msg);
                        switch(msg.msgType) {
                            case CMD_SET_LEFT_WHEEL_SPEED:
                            case CMD_SET_RIGHT_WHEEL_SPEED:
                            case CMD_SET_WHEELS_SPEED:
                            case CMD_STOP:
                            case REQ_WHEELS_STATE:
                            case REQ_DISTANCE:
                                netServ->inQueue->Enqueue(msg);
                                break;
                            case REQ_VID_STREAM_PORT:
                                {
/*
 * TODO: review since this might need to be secured by mutex, for now just keep it going unsecured
 */
                                    bool running = netServ->videoStreamManager->Running();
                                    uint16_t port = 0;
                                    if(running) {
                                        port = netServ->videoStreamManager->Port();
                                    } 
                                    Message response;
                                    response.msgType = MSG_VID_STREAM_PORT;
                                    response.data.videoStreamPort.running = running;
                                    response.data.videoStreamPort.port = port;
                                    netServ->outQueue->Enqueue(response);
                                }
                                break;
                            default:
                                {
                                    std::stringstream ss;
                                    ss << "NetService: Unsupported message received " << msg.msgType;
                                    syslog(LOG_ERR, LOG_MSG("NetService", ss.str().c_str()));
                                }
                        }
                    } else if (recvBytes == 0) {
                        /* EOF, Other end has closed connection */
                        connectionPending = false;
                    } else { 
                        /* -1 case for SOCKET_STREAM, Error occured */
                        syslog(LOG_ERR, LOG_MSG_ERR("NetService"));
                        connectionPending = false;
                    }
                }

                PTHREAD_GUARD( pthread_mutex_lock(&(netServ->clientConnectedMutex)) );

                netServ->clientConnectedSocket = -1;

                PTHREAD_GUARD( pthread_mutex_unlock(&(netServ->clientConnectedMutex)) );

                /* Closes the connection socket that originated from accept call */
                pthread_cleanup_pop(1);
            }

            pthread_cleanup_pop(0);
        }
        catch(const std::exception &e) {
            syslog(LOG_ERR, LOG_EXCEPT("NetService", e));
            kill(getpid(), SIGTERM);
        }
        catch(...) {
            syslog(LOG_ERR, LOG_MSG("NetService", "unknown exception"));
            kill(getpid(), SIGTERM);
        }
    }

/*
 * NOTE: It is by design that this function will drop any messages in the queue 
 * any data carried by any message is invalid since the receiving thread will send 
 * a stop command to the device when detect a disconnect
 */
    void* NetService::ThreadNetworkOutgoingProcedure(void *arg)
    {
        NetService *netServ = static_cast<NetService*>(arg);
        try {
            bool clientConnectedSocketLocal;
            while(true) {
/*
 * NOTE: Dequeue will put thread to sleep waiting for new messages to arrive
 */
                Message msg = netServ->outQueue->Dequeue();

                PTHREAD_GUARD( pthread_mutex_lock(&(netServ->clientConnectedMutex)) );
                clientConnectedSocketLocal = netServ->clientConnectedSocket;
                PTHREAD_GUARD( pthread_mutex_unlock(&(netServ->clientConnectedMutex)) );

                if(-1 == clientConnectedSocketLocal) {
                    syslog(LOG_NOTICE, LOG_MSG("NetService","Client not connected, discard outgoing message"));
                }
                else {
                    msg = HostToNet(msg);
                    ssize_t ret;
                    ret = send(clientConnectedSocketLocal, &msg, MESSAGE_STRUCT_SIZE, 0);

                    if( -1 == ret){
                        syslog(LOG_ERR, LOG_MSG_ERR("NetService"));
                    }
                }
            }
        }
        catch(const std::exception &e) {
            syslog(LOG_ERR, LOG_EXCEPT("NetService", e));
            kill(getpid(), SIGTERM);
        }
        catch(...) {
            syslog(LOG_ERR, LOG_MSG("NetService", "unknown exception"));
            kill(getpid(), SIGTERM);
        }
    }

    Message NetService::HostToNet(Message src)
    {
        switch(src.msgType) {
            case CMD_SET_LEFT_WHEEL_SPEED:
            case CMD_SET_RIGHT_WHEEL_SPEED:
            case CMD_SET_WHEELS_SPEED:
            case MSG_WHEELS_STATE:
                src.data.wheelsState.leftWheelSpeed = htons(src.data.wheelsState.leftWheelSpeed);
                src.data.wheelsState.rightWheelSpeed = htons(src.data.wheelsState.rightWheelSpeed);
                src.data.wheelsState.wheelMaxSpeed = htons(src.data.wheelsState.wheelMaxSpeed);
                src.data.wheelsState.wheelMinSpeed = htons(src.data.wheelsState.wheelMinSpeed);
                break;
            case MSG_DISTANCE:
                src.data.distance.distanceCM = htonl(src.data.distance.distanceCM);
                break;
            case MSG_VID_STREAM_PORT:
                src.data.videoStreamPort.port = htonl(src.data.videoStreamPort.port);
                break;
            default:
                // no action needed
                break;
        }
        return src;
    }

    Message NetService::NetToHost(Message src)
    {
        switch(src.msgType) {
            case CMD_SET_LEFT_WHEEL_SPEED:
            case CMD_SET_RIGHT_WHEEL_SPEED:
            case CMD_SET_WHEELS_SPEED:
            case MSG_WHEELS_STATE:
                src.data.wheelsState.leftWheelSpeed = ntohs(src.data.wheelsState.leftWheelSpeed);
                src.data.wheelsState.rightWheelSpeed = ntohs(src.data.wheelsState.rightWheelSpeed);
                src.data.wheelsState.wheelMaxSpeed = ntohs(src.data.wheelsState.wheelMaxSpeed);
                src.data.wheelsState.wheelMinSpeed = ntohs(src.data.wheelsState.wheelMinSpeed);
                break;
            case MSG_DISTANCE:
                src.data.distance.distanceCM = ntohl(src.data.distance.distanceCM);
                break;
            case MSG_VID_STREAM_PORT:
                src.data.videoStreamPort.port = ntohl(src.data.videoStreamPort.port);
                break;
            default:
                // no action needed
                break;
        }
        return src;
    }
};
