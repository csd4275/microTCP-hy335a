#ifndef _UTILS_ERRORC_H
#define _UTILS_ERRORC_H


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define check(x) _check(x, __LINE__, __FUNCTION__);
#define ENABLE_DEBUG_MSG

static inline void _check(int retval, int line, char * funct){

    if ( retval < 0 ) {

        #ifdef ENABLE_DEBUG_MSG
        printf("\033[93m%d\033[0m::\033[0;91m%s\033[0m() failed: \033[4m%s\033[0m\n", line, funct, strerror(errno));
        #endif
        exit(EXIT_FAILURE);
    }
}

#endif /* _UTILS_ERRORC_H */