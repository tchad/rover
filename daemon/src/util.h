/*
 * util.h
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Copyright (C) 2016 Tomasz Chadzynski
 */

#ifndef _ROVER_UTIL_H_
#define _ROVER_UTIL_H_

#include <time.h>
#include <string>


const char* const MAIN_NAME = "Rover Daemon ";

constexpr time_t DEV_CMD_SEND_T_SEC = 0;
constexpr long DEV_CMD_SEND_T_NSEC = 100000000L; //100 msec

constexpr unsigned int NET_STATUS_BCAST_T_SEC = 5;

constexpr uint16_t SERVER_TCP_PORT = 5551;
constexpr const char* SERVER_IP4_ADDR = "192.168.1.122";

constexpr uint16_t SERVER_UDP_AVAL_BCAST_PORT = 5552;
constexpr const char* SERVER_UDP_AVAL_BCAST_ADDR = "192.168.1.255";

#endif /* _ROVER_UTIL_H_ */
