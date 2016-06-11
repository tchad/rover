/*
 * uc.h
 * 
 * Interface library to the rover kernel driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Copyright (C) 2016 Tomasz Chadzynski
 */

#ifndef __UC_H_
#define __UC_H_

#include <stdint.h>
#include <rover.h>


struct adsensor {
    int initialized;
    int dev_file;
    int event_file;
};

#ifdef __cplusplus
extern "C" {
#endif

struct adsensor* alloc_adsensor();
int init(struct adsensor * dev);
int release(struct adsensor * dev);
int send_ioctl_command( struct adsensor * dev, enum ioctl_command cmd, int16_t arg1, int16_t arg2);
int get_wheels_state(struct adsensor *dev, struct uc0_wheel_state *device);
int read_distance(struct adsensor *dev, int32_t *distance);

#ifdef __cplusplus
}
#endif

#endif
