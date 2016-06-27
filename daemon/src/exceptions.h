/*
 * exceptions.h
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Copyright (C) 2016 Tomasz Chadzynski
 */

#ifndef _EXCEPRIONS_H_
#define _EXCEPRIONS_H_

#include <exception>

namespace RoverExceptions
{
    class uninitialized : public std::exception
    {
        public:
            virtual ~uninitialized() = default;
    };

    class service_unavailable : public std::exception
    {
        public:
            virtual ~service_unavailable() = default;
    };
};

#endif /* _EXCEPRIONS_H_ */
