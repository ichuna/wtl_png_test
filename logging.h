#ifndef LOGGING_H_
#define LOGGING_H_
void writelog(const char *format, ...);

#ifdef ENABLE_PNGLOG
#define PNG_LOG writelog
#else
#define PNG_LOG writelog
#endif
#define TEST_LOG writelog

#endif // LOGGING_H_
