#ifndef __MY_LOG_H__
#define __MY_LOG_H__

#include <android/log.h>

#define ENABLE_DEBUG 1

#if ENABLE_DEBUG
#define LOG_TAG "hideroot"
#define LOGD(fmt, args...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)
#define DEBUG_PRINT(format,args...) LOGD(format, ##args)
#else
#define DEBUG_PRINT(format,args...)
#endif

#endif // __MY_LOG_H__
