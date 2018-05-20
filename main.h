#pragma once

#include <jni.h>
#include <android/log.h>
#include <v8.h>

#define TAG "HC"

#define LOGV(fmt, args...) (__android_log_print(ANDROID_LOG_ERROR, TAG, fmt, ##args))
