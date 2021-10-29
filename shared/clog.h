#ifndef _C_LOG_H
#define _C_LOG_H

/**
 * logging with timestamp with format like this
 *
 * print lable and timestamp eg: " 2021-10-13T15:05:04.841Z    INFO] msg "
 */

void infof(const char* fmt, ...);
void errorf(const char* fmt, ...);
void debugf(const char* fmt, ...);
void warnf(const char* fmt, ...);

#endif
