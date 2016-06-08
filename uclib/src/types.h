/*
 * types.h
 * 
 * Temporary dummy uc library to satisfy daemon build requirements
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Copyright (C) 2016 Tomasz Chadzynski
 */

#ifndef __DRV_TYPES_
#define __DRV_TYPES_

struct device_state {
    int16_t left_wheel_speed;
    int16_t right_wheel_speed;
    int16_t wheel_max_speed;
    int16_t wheel_min_speed;
};

enum ioctl_command {
    SET_WHEEL_SPEED = 0xF1, // pass int16_t[2] as an argument (0=left 1=right)
    STOP = 0xF2
};

#endif
