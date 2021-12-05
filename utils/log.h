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
#include <sys/syscall.h>


#define ENABLE_DEBUG_MSG


#ifdef ENABLE_DEBUG_MSG
#define LOG_INFO(M, ...)                                                        \
                fprintf(stderr, "[INFO]: %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define LOG_INFO(M, ...)
#endif

#define LOG_ERROR(M, ...)                                                       \
        fprintf(stderr, "[ERROR] %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define LOG_WARN(M, ...)                                                                \
        fprintf(stderr, "[WARNING] %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#ifdef ENABLE_DEBUG_MSG

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define __FILENAME__   \
        (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)


#define check(x) _check(x, __LINE__, __FUNCTION__);

static inline void _check(int retval, int line, const char * funct){

    if ( retval < 0 ) {

        #ifdef ENABLE_DEBUG_MSG
        printf("\033[93m%d\033[0m::\033[0;91m%s\033[0m() failed: \033[4m%s\033[0m\n", line, funct, strerror(errno));
        #endif
        exit(EXIT_FAILURE);
    }
}
#define LOG_DEBUG(M, ...)                                                       \
        fprintf(stderr, "\033[1m[\033[0;31mDEBUG\033[0;1m]\033[0m: \033[93m%s\033[0m::\033[93m%s\033[0m::\033[93m%d\033[0m -> " M "\n", __FILENAME__ , __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define LOG_DEBUG(M, ...)
#define check(x)
#endif

#endif /* UTILS_LOG_H_ */
