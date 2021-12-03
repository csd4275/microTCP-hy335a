/*
 * microtcp, a lightweight implementation of TCP for teaching,
 * and academic purposes.
 *
 * Copyright (C) 2017  Manolis Surligas <surligas@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef UTILS_LOG_H_
#define UTILS_LOG_H_

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/syscall.h>

/* Set to 0 to disable debug messages at compile time ;) */
#define ENABLE_DEBUG_MSG 1

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#if ENABLE_DEBUG_MSG
#define LOG_INFO(M, ...)                                                        \
                fprintf(stderr, "[INFO]: %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#else
#define LOG_INFO(M, ...)
#endif

#define LOG_ERROR(M, ...)                                                       \
        fprintf(stderr, "[ERROR] %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define LOG_WARN(M, ...)                                                                \
        fprintf(stderr, "[WARNING] %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#if ENABLE_DEBUG_MSG
#define LOG_DEBUG(M, ...)                                                       \
        fprintf(stderr, "\033[1m[\033[0;31mDEBUG\033[0;1m]\033[0m: \033[93m%s\033[0m::\033[93m%d\033[0m: " M "\n", __FILENAME__, __LINE__, ##__VA_ARGS__)
#else
#define LOG_DEBUG(M, ...)
#endif

#endif /* UTILS_LOG_H_ */
