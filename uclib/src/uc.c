/*
 * uc.c
 * 
 * Temporary dummy uc library to satisfy daemon build requirements
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Copyright (C) 2016 Tomasz Chadzynski
 */

#include "uc.h"

struct adsensor* alloc_adsensor()
{
	struct adsensor* ret = (struct adsensor*)malloc(sizeof(struct adsensor));
	ret->initialized = 0;
	return ret;
}

int init(struct adsensor * dev)
{
    printf("SIM: init\n");
    return 0;
}

int release(struct adsensor * dev) 
{
    if(dev != NULL) {
        free(dev);
    }
    
    return 0;
}

int send_ioctl_command( struct adsensor * dev, enum ioctl_command cmd, int16_t arg1, int16_t arg2)
{
    printf("SIM: send_ioctl_command IOCTL: %i arg1: %i arg2: %i\n", cmd, arg1, arg2);

    return 0;
}

int get_wheels_state(struct adsensor *dev, struct device_state *device)
{
    device->left_wheel_speed = 0;
    device->right_wheel_speed = 0;
    device->wheel_max_speed = 255;
    device->wheel_min_speed = 0;

    printf("SIM: get_wheels_state lf: %i rw: %i wmax: %i wmin: %i\n", 
    device->left_wheel_speed,
    device->right_wheel_speed,
    device->wheel_max_speed,
    device->wheel_min_speed);

    return 0;
}

int read_distance(struct adsensor *dev, int32_t *distance)
{

    static uint16_t i=0;
    static int32_t last = 50;
    if(i==100) {
        i=0;
        if(last == 50) {
            last = 10;
        } else {
            last = 50;
        }
    }
    *distance = last;
    i++;

    usleep(100000);
    return 0;
}

