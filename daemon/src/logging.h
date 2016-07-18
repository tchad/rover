/*
 * logging.h
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Copyright (C) 2016 Tomasz Chadzynski
 */

#ifndef _LOGGING_H_
#define _LOGGING_H

#include <string>
#include <string.h>
#include <sstream>
#include <stdexcept>

/* Function forming error messages, not to be used directly */
std::string log_ex(const char* file, const char* func, int line, int _errno);
std::string log_msg(const char* file, const char* func, int line, const char* msg);

/* Form error message based on errno */
#define FORM_ERROR() log_ex(__FILE__, __func__, __LINE__, errno)

/* Form error message based on custom error id that is treated as errno */
#define FORM_ERROR_EID(_ERR_) log_ex(__FILE__, __func__, __LINE__, _ERR_)

/* Form log message based on custom message passed */
#define FORM_MSG(__MSG__) log_msg(__FILE__, __func__, __LINE__, __MSG__)

/* Throw runtime_error with message formulated based on errno */
#define THROW_RUNTIME() throw std::runtime_error(FORM_ERROR())

/* Throw runtime_error exception based on custom error number that is process as errno */
#define THROW_RUNTIME_EID(__ERR__) throw std::runtime_error(FORM_ERROR_EID(__ERR__))

/* Throw runtime_error exception with custome message */
#define THROW_RUNTIME_MSG(_MSG_) throw std::runtime_error(FORM_MSG(_MSG_))

/* Default guard function for pthread calls, used since pthread api does not directly modify errno.  */
#define PTHREAD_GUARD(__FCALL__) { int r; if(0!= (r = __FCALL__)) THROW_RUNTIME_EID(r); }

/* Form log message from the content of exception message */
#define LOG_EXCEPT(_MODULE_, ex) (std::string("(") + _MODULE_ + std::string(") ") + ex.what()).c_str()

/* Form log message from errno variable */
#define LOG_MSG_ERR(_MODULE_) (std::string("(") + _MODULE_ + std::string(") ") + FORM_ERROR()).c_str()

/* Form log message from custom text passed as argument */
#define LOG_MSG(_MODULE_, _MSG_) (std::string("(") + _MODULE_ + std::string(") ") + FORM_MSG(_MSG_)).c_str()

#endif //_LOGGING_H_
