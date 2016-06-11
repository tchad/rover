/*
 * uc.c
 * 
 * Interface library to the rover kernel driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Copyright (C) 2016 Tomasz Chadzynski
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/input.h>

#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>

#include "uc.h"
#include "../config.h"

/*
 * TODO:
 * use syslog for logging err for errors reporting to the console (may be)
 * man 3 syslog, man 3 err
 */


struct adsensor* alloc_adsensor()
{
	struct adsensor* ret = (struct adsensor*)malloc(sizeof(struct adsensor));
	ret->initialized = 0;
	return ret;
}

#ifndef SIM_MODE

int init(struct adsensor * dev)
{
    int ret = 0;
    if(dev->initialized) {
        perror("Already initialized");
        return -EPERM;
    }

    dev->dev_file = open(DRV_FILE_PATH, O_RDWR);
    if(dev->dev_file < 0) {
        ret = errno;
        err(1,"%s","opening drv");
        return -ret;
    }

    dev->event_file = open(DRV_POLL_PATH, O_RDONLY);
    if(dev->event_file < 0) {
        ret = errno;
        err(1,"%s","opening event file");
        //close the opened dev file first
        if(close(dev->dev_file) == -1){
            ret = errno;
            err(1,"%s","closing drv");
        }

        return -ret;
    }

    dev->initialized = 1;
    return 0;
}

int release(struct adsensor * dev) 
{
    int ret = 0;

    if(dev->initialized) {
        if(close(dev->dev_file) == -1){
            ret = errno;
            err(1,"%s","closing drv");
        }

        if(close(dev->event_file) == -1){
            ret = errno;
            err(1,"%s","closing event file");
        }
    }
    free(dev);
    
    return -ret;
}

int send_ioctl_command( struct adsensor * dev, enum ioctl_command cmd, int16_t arg1, int16_t arg2)
{
    int ret=0;
    int16_t args[2] = {arg1, arg2};
    
    if(ioctl(dev->dev_file, cmd, args) == -1) {
        ret = errno;
        err(1,"%s","ioctl command");
    }

    return -ret;
}

int get_wheels_state(struct adsensor *dev, struct uc0_wheel_state *device)
{
    int ret = 0;
    ssize_t bytes_read = 0;
    if(!dev->initialized){
        perror("Cant read uninitialized resource");
        return -EIO;
    }

    if(lseek(dev->dev_file, 0, SEEK_SET) == -1) {
       ret = errno;
      err(1,"%s","read wheel state seek");
        return -ret;
    }
   
    bytes_read = read(dev->dev_file, device, sizeof(struct uc0_wheel_state));
    

    if(bytes_read == -1) {
        ret = errno;
        err(1,"%s","read wheel state read");
        return -ret;
    }

    return 0;
}

int read_event(struct adsensor *dev, struct input_event *ev)
{
    int ret =0;
    ssize_t bytes_read = 0;

    if(!dev->initialized){
        perror("Cant read uninitialized resource");
        return -EIO;
    }

    bytes_read = read(dev->event_file, ev, sizeof(struct input_event));

    if(bytes_read == -1) {
        ret = errno;
        err(1,"%s","read event");
        return -ret;
    }

    return 0;
}

int read_distance(struct adsensor *dev, int32_t *distance)
{
    int ret =0;
    struct input_event ev;

    ret = read_event(dev, &ev);

    if(ret < 0) {
        return ret;
    }

    if(ev.type == EV_MSC || ev.code == MSC_RAW) {
        *distance = ev.value;
    }

    return 0;
}

#else //bridge in simulation mode 

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

int get_wheels_state(struct adsensor *dev, struct uc0_wheel_state *device)
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

int read_event(struct adsensor *dev, struct input_event *ev)
{
    printf("SIM: read_event\n");

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

#endif
