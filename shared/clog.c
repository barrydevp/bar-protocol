#include "clog.h"

#include <stdarg.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

void plabel(const char* label) {
  char date[20];
  struct timeval tv;

  // print lable and timestamp eg: " 2021-10-13T15:05:04.841Z    INFO] msg "
  gettimeofday(&tv, NULL);
  strftime(date, sizeof(date) / sizeof(*date), "%Y-%m-%dT%H:%M:%S", gmtime(&tv.tv_sec));
  printf(" %s.%03ldZ%6s] ", date, tv.tv_usec / 1000, label);
}

void debugf(const char* fmt, ...) {
  plabel("DEBUG");

  /* printf like normal */
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
  printf("\n");
}

void infof(const char* fmt, ...) {
  plabel("INFO");

  /* printf like normal */
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
  printf("\n");
}

void warnf(const char* fmt, ...) {
  plabel("WARN");

  /* printf like normal */
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
  printf("\n");
}

void errorf(const char* fmt, ...) {
  plabel("ERROR");

  /* printf like normal */
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
  printf("\n");
}

