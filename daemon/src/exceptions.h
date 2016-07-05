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
#include <stdexcept>

namespace RoverExceptions
{
    class uninitialized : public std::runtime_error
    {
        public:
            explicit uninitialized(const std::string &what_arg):
                runtime_error(what_arg) {}

            explicit uninitialized(const char* what_arg):
                runtime_error(what_arg) {}

            virtual ~uninitialized() = default;
    };

    class service_unavailable : public std::runtime_error
    {
        public:
            explicit service_unavailable(const std::string &what_arg):
                runtime_error(what_arg) {}

            explicit service_unavailable(const char* what_arg):
                runtime_error(what_arg) {}

            virtual ~service_unavailable() = default;
    };
};

#endif /* _EXCEPRIONS_H_ */
