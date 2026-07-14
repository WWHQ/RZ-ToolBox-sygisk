#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/system_properties.h>
#include <android/log.h>
#include "zygisk.h"

#define LOG_TAG "CoreProp-Bridge"
// #define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGI(...) ((void)0)

static int (*orig_system_property_get)(const char *, char *);

// 欺骗逻辑
static int my_property_get(const char *key, char *value) {
    LOGI("Checking prop: %s", key); // 关键调试点：如果游戏调用了，这里会有日志
    
    if (strcmp(key, "ro.product.cpu.abi") == 0) {
        strcpy(value, "x86_64");
        return 6;
    }
    if (strcmp(key, "ro.product.cpu.abilist") == 0) {
        strcpy(value, "x86_64,x86");
        return 10;
    }
    return orig_system_property_get(key, value);
}

static struct rezygisk_api *g_api = NULL;

static void pre_app_specialize(void *self, void *args) {
    (void)self;
    struct app_specialize_args_v5 *app_args = (struct app_specialize_args_v5 *)args;
    
    // 获取包名，请将下方字符串替换为你的游戏包名
    const char *pkg = app_args->nice_name ? (const char*)app_args->nice_name : "";
    if (strcmp(pkg, "com.tencent.tmgp.sgame") != 0 ||
        strcmp(pkg, "com.liuzh.deviceinfo") != 0) return; 

    LOGI("Target detected: %s, applying hook...", pkg);

    if (g_api->plt_hook_register) {
        g_api->plt_hook_register(0, 0, "__system_property_get", 
                                (void *)my_property_get, 
                                (void **)&orig_system_property_get);
        
        // 提交 Hook
        if (g_api->plt_hook_commit()) {
            LOGI("ABI Hook applied successfully.");
        }
    }
}

static struct rezygisk_abi module_abi = {
    .api_version = REZYGISK_API_VERSION,
    .pre_app_specialize = pre_app_specialize,
};

__attribute__((visibility("default")))
void zygisk_module_entry(void *api_ptr, void *env) {
    (void)env;
    g_api = (struct rezygisk_api *)api_ptr;
    g_api->register_module(g_api, &module_abi);
    LOGI("CoreProp-Bridge initialized.");
}

__attribute__((visibility("default")))
void zygisk_companion_entry(int client_fd) {
    close(client_fd);
}
