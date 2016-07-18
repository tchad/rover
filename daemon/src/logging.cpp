/*
 * logging.cpp
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Copyright (C) 2016 Tomasz Chadzynski
 */

#include "logging.h"

std::string log_ex(const char* file, const char* func, int line, int _errno)
{
    std::stringstream ss;
    ss << file << " [" << func << ":" << line << "]: " << strerror(_errno);
    return ss.str();
}

std::string log_msg(const char* file, const char* func, int line, const char* msg)
{
    std::stringstream ss;
    ss << file << " [" << func << ":" << line << "]: " << msg;
    return ss.str();
}
