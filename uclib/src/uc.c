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
#include <config.h>
#include <rover.h>

/*
 * TODO:
 * use syslog for logging err for errors reporting to the console (may be)
 * man 3 syslog, man 3 err
 */


struct device_rover* alloc_device_rover()
{
	struct device_rover* ret = (struct device_rover*)malloc(sizeof(struct device_rover));
	ret->initialized = 0;

	return ret;
}

#ifndef SIM_MODE

#define DRV_FILE_PATH "/dev/roveruc0"
#define DRV_POLL_PATH "/dev/input/event0"

int init_device_rover(struct device_rover * dev)
{
    int ret = EXIT_SUCCESS;

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

    return ret;
}

int release_device_rover(struct device_rover * dev) 
{
    int ret = EXIT_SUCCESS;

    if(dev->initialized) {
        if(close(dev->dev_file) == -1){
            ret = -errno;
            err(1,"%s","closing drv");
        }

        if(close(dev->event_file) == -1){
            ret = -errno;
            err(1,"%s","closing event file");
        }
    }

    free(dev);
    
    return ret;
}

static int send_ioctl_command( struct device_rover * dev, enum ioctl_command cmd, int16_t arg1, int16_t arg2)
{
    int ret=EXIT_SUCCESS;
    int16_t args[2] = {arg1, arg2};
    
    if(ioctl(dev->dev_file, cmd, args) == -1) {
        ret = -errno;
        err(1,"%s","ioctl command");
    }

    return ret;
}


int set_wheel_speed(struct device_rover *dev, int16_t left, int16_t right)
{
    int ret;
    ret = send_ioctl_command(dev, SET_WHEEL_SPEED, left, right);

    return ret;
}

int set_wheel_stop(struct device_rover *dev)
{
    int ret;
    ret = send_ioctl_command(dev, STOP, 0, 0);

    return ret;
}

int get_device_state(struct device_rover *dev, struct device_state *device)
{
    int ret = EXIT_SUCCESS;
    ssize_t bytes_read = 0;
    struct uc0_wheel_state uc0_dev_state;

    if(!dev->initialized){
        perror("Cant read uninitialized resource");
        return -EIO;
    }

    if(lseek(dev->dev_file, 0, SEEK_SET) == -1) {
       ret = -errno;
      err(1,"%s","read wheel state seek");
        return ret;
    }
   
    bytes_read = read(dev->dev_file, &uc0_dev_state, sizeof(struct uc0_wheel_state));
    

    if(bytes_read == -1) {
        ret = -errno;
        err(1,"%s","read wheel state read");
        return ret;
    }

    device->left_wheel_speed = uc0_dev_state.left_wheel_speed;
    device->right_wheel_speed = uc0_dev_state.right_wheel_speed;
    device->wheel_max_speed = uc0_dev_state.wheel_max_speed;
    device->wheel_min_speed = uc0_dev_state.wheel_min_speed;

    return ret;
}

static int read_event(struct device_rover *dev, struct input_event *ev)
{
    int ret = EXIT_SUCCESS;
    ssize_t bytes_read = 0;

    if(!dev->initialized){
        perror("Cant read uninitialized resource");
        return -EIO;
    }

    bytes_read = read(dev->event_file, ev, sizeof(struct input_event));

    if(bytes_read == -1) {
        ret = -errno;
        err(1,"%s","read event");
        return ret;
    }

    return ret;
}

int read_distance(struct device_rover *dev, int32_t *distance)
{
    int ret;
    struct input_event ev;

    ret = read_event(dev, &ev);

    if(ret != EXIT_SUCCESS) {
        return ret;
    }

    if(ev.type == EV_MSC || ev.code == MSC_RAW) {
        *distance = ev.value;
    }

    return ret;
}

#else //in simulation mode 

static struct device_state dev_state;

int init_device_rover(struct device_rover * dev)
{
    printf("SIM: init\n");
    dev_state.left_wheel_speed = 0;
    dev_state.right_wheel_speed = 0;
    dev_state.wheel_max_speed = 255;
    dev_state.wheel_min_speed = -255;
    return EXIT_SUCCESS;
}

int release_device_rover(struct device_rover * dev) 
{
    if(dev != NULL) {
        free(dev);
    }
    
    return EXIT_SUCCESS;
}

int set_wheel_speed(struct device_rover *dev, int16_t left, int16_t right)
{
    printf("SIM: set_wheel_speed: left: %i right: %i\n", left, right);

    dev_state.left_wheel_speed = left;
    dev_state.right_wheel_speed = right;

    return EXIT_SUCCESS;
}

int set_wheel_stop(struct device_rover *dev)
{
    printf("SIM: set_wheel_stop\n");

    dev_state.left_wheel_speed = 0;
    dev_state.right_wheel_speed = 0;

    return EXIT_SUCCESS;
}

int get_device_state(struct device_rover *dev, struct device_state *device)
{
    device->left_wheel_speed = dev_state.left_wheel_speed;
    device->right_wheel_speed = dev_state.right_wheel_speed;
    device->wheel_max_speed = dev_state.wheel_max_speed;
    device->wheel_min_speed = dev_state.wheel_min_speed;

    printf("SIM: get_wheels_state lf: %i rw: %i wmax: %i wmin: %i\n", 
    device->left_wheel_speed,
    device->right_wheel_speed,
    device->wheel_max_speed,
    device->wheel_min_speed);

    return EXIT_SUCCESS;
}

int read_distance(struct device_rover *dev, int32_t *distance)
{
    static int32_t last = 50;
    if(last == 100) {
        last = 50;
    }
    else {
        last += 10;
    }
    *distance = last;

    return EXIT_SUCCESS;
}

#endif
