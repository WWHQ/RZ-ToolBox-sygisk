#include <stdio.h>
#include <string.h>
#include <sys/system_properties.h>
#include <android/log.h>
#include "zygisk.h"

// 隐藏模块名称，使用通用系统组件名称
#define LOG_TAG "CoreProp-Bridge"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// 原函数指针
static int (*orig_system_property_get)(const char *, char *);

// 欺骗逻辑：仅针对 ABI 相关属性进行伪装
static int my_property_get(const char *key, char *value) {
    if (strcmp(key, "ro.product.cpu.abi") == 0) {
        strcpy(value, "x86_64");
        return 6;
    }
    if (strcmp(key, "ro.product.cpu.abilist") == 0) {
        strcpy(value, "x86_64,x86");
        return 10;
    }
    if (strcmp(key, "ro.product.cpu.abilist32") == 0) {
        strcpy(value, "x86");
        return 3;
    }
    if (strcmp(key, "ro.product.cpu.abilist64") == 0) {
        strcpy(value, "x86_64");
        return 6;
    }
    return orig_system_property_get(key, value);
}

// 模块状态指针
static struct rezygisk_api *g_api = NULL;

static void pre_app_specialize(void *self, void *args) {
    // 这里的 args 结构在 ReZygisk API v5 中对应 app_specialize_args_v5
    struct app_specialize_args_v5 *app_args = (struct app_specialize_args_v5 *)args;

    // 1. 设置目标游戏包名过滤 (请根据实际情况填写)
    const char *target_pkg = "com.your.game.package";
    
    // 注意：nice_name 在 specialize 阶段为 jstring，需处理转换
    // 此处简化为逻辑判断，实际应用中建议检查该指针指向的字符串
    // 若包名匹配，则注册 Hook
    
    if (g_api->plt_hook_register) {
        g_api->plt_hook_register(0, 0, "__system_property_get", 
                                (void *)my_property_get, 
                                (void **)&orig_system_property_get);
        
        if (g_api->plt_hook_commit()) {
            LOGI("CoreProp-Bridge: ABI Hook active for target process.");
        }
    }
}

static struct rezygisk_abi module_abi = {
    .api_version = REZYGISK_API_VERSION,
    .pre_app_specialize = pre_app_specialize,
};

__attribute__((visibility("default")))
void zygisk_module_entry(void *api_ptr, void *env) {
    g_api = (struct rezygisk_api *)api_ptr;
    g_api->register_module(g_api, &module_abi);
    LOGI("CoreProp-Bridge initialized.");
}

__attribute__((visibility("default")))
void zygisk_companion_entry(int client_fd) {
    close(client_fd);
}
