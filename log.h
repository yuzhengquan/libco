//by yuzhengquan
#ifndef __CO_LOG_H__
#define __CO_LOG_H__
#include <stdarg.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include "glog/logging.h"
enum {
    FATAL = 0,
    ERROR,
    WARN,
    INFO
};

void inline co_log_print(int level, const char *func_name, const char *fmt, ...) {
    char logbuf[4096];
//    void* buffer[20];
//    size_t size;
//    char **strings = NULL;
//    int pos = 0;
//    size = backtrace(buffer, 20);
//    strings = backtrace_symbols(buffer, size);
//    for (int i = 0; i < size; ++i) {
//        pos += snprintf(logbuf + pos, 4096 - pos, "=>%s", strings[i]);
//    }
//    free(strings);
    logbuf[0] = '\0';
    va_list arg_ptr; 
    va_start(arg_ptr, fmt);
    char buf[2048];
    vsprintf(buf, fmt, arg_ptr);
    switch(level) {
        case FATAL:
            LOG(FATAL) << "[" << func_name << "] " << logbuf << buf;
            break;
        case ERROR:
            LOG(ERROR) << "[" << func_name << "] " <<  logbuf << buf;
            break;
        case WARN:
            LOG(WARNING) << "[" << func_name << "] " <<  logbuf << buf;
            break;
        default:
            LOG(INFO) << "[" << func_name << "] " <<  logbuf << buf;
            break;
    }
    va_end(arg_ptr);
}
#define CO_LOG(level, fmt, ...) \
    do {   \
    co_log_print(level, __FUNCTION__, fmt, ##__VA_ARGS__);\
} while (0)

#define BUG() \
 do {   \
    CO_LOG(FATAL, "BUG");\
    *(int *)0 = 0;\
 } while(0)
     
#define BUG_ON(p) \
do {   \
    if (p) {\
        CO_LOG(FATAL, "BUG");\
        *(int *)0 = 0;  \
    }\
} while(0)

#endif //__CO_LOG_H__
