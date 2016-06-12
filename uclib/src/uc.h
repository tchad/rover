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


struct device_rover {
    int initialized;
    int dev_file;
    int event_file;
};

struct device_state {
    int16_t left_wheel_speed;
    int16_t right_wheel_speed;
    int16_t wheel_max_speed;
    int16_t wheel_min_speed;
};

#ifdef __cplusplus
extern "C" {
#endif

struct device_rover* alloc_device_rover();
int init_device_rover(struct device_rover *dev);
int release_device_rover(struct device_rover *dev);

int set_wheel_speed(struct device_rover *dev, int16_t left, int16_t right);
int set_wheel_stop(struct device_rover *dev);

int get_device_state(struct device_rover *dev, struct device_state *dev_state);
int read_distance(struct device_rover *dev, int32_t *distance);

#ifdef __cplusplus
}
#endif

#endif
