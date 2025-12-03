/**
 * 2025/08/24 debug.h 用于c/c++代码个人调试
 */
#ifndef __ANDRA_DEBUG_H__
#define __ANDRA_DEBUG_H__

#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>

#define DEBUG_ENABLE (1)

// GNU 特性, 宏定义转字符串
#define STRINGIZE_NO_EXPANSION(x) #x
// 转字符串，不带其他后缀字符串
#define STRINGIZE(x) STRINGIZE_NO_EXPANSION(x)
// 合并文件名和行数
#define FILE_AND_LINE (__FILE__ ":" STRINGIZE(__LINE__))

// A LOG for Information : just time info
#if DEBUG_ENABLE
#define ALOGI(...) do{\
  struct timeval _tv;\
  gettimeofday(&_tv, NULL);\
  fprintf(stdout, "[\033[2m%ld.%06ld\033[0m] ", _tv.tv_sec, _tv.tv_usec);\
  fprintf(stdout, __VA_ARGS__);\
  fprintf(stdout, "\n");\
}while(0)
#else
#define ALOGI(...) do{}while(0)
#endif

// A LOG for Debug : time, file, line and function info
#if DEBUG_ENABLE
#define ALOGD(...) do{\
  char buf[1024] = {0};\
  int buflen = 0;\
  struct timeval _tv;\
  gettimeofday(&_tv, NULL);\
  buflen += snprintf(buf, sizeof(buf), "[\033[2m%ld.%06ld\033[0m] ", _tv.tv_sec, _tv.tv_usec);\
  buflen += snprintf(buf+buflen, sizeof(buf)-buflen, __VA_ARGS__);\
  buflen += snprintf(buf+buflen, sizeof(buf)-buflen, "\033[2;3m@%s,%s\033[0m", __FUNCTION__, FILE_AND_LINE);\
  fprintf(stdout, "%s\n", buf);\
}while(0)
#else
#define ALOGD(...) do{}while(0)
#endif

// A LOG for Thread : time, thread
#if DEBUG_ENABLE
#define ALOGT(...) do{\
  char buf[1024] = {0};\
  int buflen = 0;\
  struct timeval _tv;\
  gettimeofday(&_tv, NULL);\
  buflen += snprintf(buf, sizeof(buf), "[\033[2m%ld.%06ld\033[0m] ", _tv.tv_sec, _tv.tv_usec);\
  char _name[16] = {0};\
  pthread_t _tid = pthread_self();\
  pthread_getname_np(_tid, _name, sizeof(_name));\
  buflen += snprintf(buf+buflen, sizeof(buf)-buflen, "<\033[1;%lum%s\033[0m> ", 30+_tid%0x7, _name);\
  buflen += snprintf(buf+buflen, sizeof(buf)-buflen, __VA_ARGS__);\
  fprintf(stdout, "%s\n", buf);\
}while(0)
#else
#define ALOGT(...) do{}while(0)
#endif

// A LOG for All information
#if DEBUG_ENABLE
#define ALOGA(...) do{\
  char buf[1024] = {0};\
  int buflen = 0;\
  struct timeval _tv;\
  gettimeofday(&_tv, NULL);\
  buflen += snprintf(buf, sizeof(buf), "[\033[2m%ld.%06ld\033[0m] ", _tv.tv_sec, _tv.tv_usec);\
  char _name[16] = {0};\
  pthread_t _tid = pthread_self();\
  pthread_getname_np(_tid, _name, sizeof(_name));\
  buflen += snprintf(buf+buflen, sizeof(buf)-buflen, "<\033[1;%lum%s\033[0m> ", 30+_tid%0x7, _name);\
  buflen += snprintf(buf+buflen, sizeof(buf)-buflen, __VA_ARGS__);\
  buflen += snprintf(buf+buflen, sizeof(buf)-buflen, "\033[2;3m@%s,%s\033[0m", __FUNCTION__, FILE_AND_LINE);\
  fprintf(stdout, "%s\n", buf);\
}while(0)
#else
#define ALOGA(...) do{}while(0)
#endif

// A LOG for break
#if DEBUG_ENABLE
#define ALOG_BREAK_IF(cond) if(cond){ALOGD("\e[1;37mBreak\e[0m because (%s)", STRINGIZE(cond));break;}
#else
#define ALOG_BREAK_IF(cond) if(cond){break;}
#endif

#if __cplusplus

/**
 * 函数生命期跟踪，利用C++ RAII特性实现
 */
class IFuncTrace {
public:
  IFuncTrace(const char *function, const char *file_and_line)
   : function_(function), file_and_line_(file_and_line) {
#if DEBUG_ENABLE
    ALOGI("Enter %s,%s", function_, file_and_line_);
#endif
  }
  ~IFuncTrace() {
#if DEBUG_ENABLE
    ALOGI("Leave %s,%s", function_, file_and_line_);
#endif
  }
private:
  const char *function_;
  const char *file_and_line_;
};
#define ALOG_TRACE IFuncTrace _lb(__FUNCTION__, FILE_AND_LINE)

#endif // __cplusplus

#endif // __ANDRA_DEBUG_H__