/*
* roveruclibtest.c
 * 
 * Utility test app for the rever control library.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Copyright (C) 2016 Tomasz Chadzynski
 */


#include <stdio.h>
#include <stdlib.h>
#include <uc.h>

void print_usage()
{
    printf("\n\n############### USAGE ###############\n");
    printf("q-quit, w-left speedup, e-right speedup\n");
    printf("s-left speeddown, d-right speeddown\n");
    printf("z-stop, x-print device state, c-print distance\n");
    printf("#####################################\n\n");
}

void print_device_state(struct device_rover *dev)
{
    struct device_state dev_state;
    get_device_state(dev, &dev_state);
    printf("\n Device state:\n left speed: %i\n right speed:%i\n max: %i min: %i\n",
        dev_state.left_wheel_speed,
        dev_state.right_wheel_speed,
        dev_state.wheel_max_speed,
        dev_state.wheel_min_speed);
}

void set_speed(struct device_rover *dev, int16_t d_left, int16_t d_right)
{
    struct device_state dev_state;
    int16_t speed_left;
    int16_t speed_right;

    get_device_state(dev, &dev_state);
    speed_left = dev_state.left_wheel_speed + d_left;
    if(speed_left < dev_state.wheel_min_speed || speed_left > dev_state.wheel_max_speed) {
        printf("Left speed out of range!\n");
        speed_left = dev_state.left_wheel_speed;
    }

    speed_right = dev_state.right_wheel_speed + d_right;
    if(speed_right < dev_state.wheel_min_speed || speed_right > dev_state.wheel_max_speed) {
        printf("Right speed out of range!\n");
        speed_right = dev_state.right_wheel_speed;
    }

    set_wheel_speed(dev, speed_left, speed_right);
}


int main()
{
    int run;
    int ret;
    struct device_rover* dev;
    char buff;
    int32_t dist;

    dev = alloc_device_rover();
    if(dev == NULL) {
        fprintf(stderr,"Unable to allocate memory for device_rover structure %s:%d \n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }

    ret = init_device_rover(dev);
    if(ret != EXIT_SUCCESS) {
        fprintf(stderr,"Failed to initialize device %s:%d \n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }

#define STEP 10

    run = 1;
    while(run){
        print_usage();
        printf("cmd: ");
        scanf("%c1",&buff);
        switch(buff) {
            case 'q':
                run = 0;
                printf("\nBye!\n");
                break;
            case 'w':
                set_speed(dev,STEP,0);
                break;
            case 'e':
                set_speed(dev,0,STEP);
                break;
            case 's':
                set_speed(dev,-STEP,0);
                break;
            case 'd':
                set_speed(dev,0,-STEP);
                break;
            case 'z':
                set_wheel_stop(dev);
                break;
            case 'x':
                print_device_state(dev);
                break;
            case 'c':
                read_distance(dev, &dist);
                printf("Distance: %i\n", dist);
                break;
            default:
                printf("\nUnknown cmd: %c\n", buff);
                break;
        };
    }

    ret = release_device_rover(dev);
    if(ret != EXIT_SUCCESS) {
        fprintf(stderr,"Failed to release device %s:%d \n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    

    return EXIT_SUCCESS;
}
