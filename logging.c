
#include "logging.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>


FILE* pFile = NULL;
void writelog(const char *format, ...) {
  if (pFile == NULL) {
    fopen_s(&pFile, "png_log.txt", "w");
    if (pFile == NULL)
      return;
    fclose(pFile);
    fopen_s(&pFile, "png_log.txt", "a");
  }

  va_list arg;
  int done;

  va_start(arg, format);
  //done = vfprintf (stdout, format, arg);

  time_t time_log = time(NULL);
  struct tm tm_log;
  localtime_s(&tm_log, &time_log);
  fprintf(pFile, "%04d-%02d-%02d %02d:%02d:%02d ", tm_log.tm_year + 1900, tm_log.tm_mon + 1, tm_log.tm_mday, tm_log.tm_hour, tm_log.tm_min, tm_log.tm_sec);

  done = vfprintf(pFile, format, arg);
  va_end(arg);

  fflush(pFile);
}
