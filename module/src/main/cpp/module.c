#include <stdio.h>
#include <string.h>
#include <unistd.h>               // 解决 close() 未定义错误
#include <sys/system_properties.h>
#include <android/log.h>
#include "zygisk.h"

// 隐藏模块名称，使用通用系统组件名称
#define LOG_TAG "CoreProp-Bridge"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// 原函数指针
static int (*orig_system_property_get)(const char *, char *);

// 欺骗逻辑：ABI 伪装
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
    (void)self; // 消除 unused 参数警告
    struct app_specialize_args_v5 *app_args = (struct app_specialize_args_v5 *)args;
    
    // 确保 nice_name 存在
    if (!app_args->nice_name) return;

    // 转换为 char* 字符串 (注意：ReZygisk 中 nice_name 类型根据 API 版本可能不同)
    // 此处直接比较 nice_name 逻辑，若你需要精确匹配包名，请改为 strcmp
    // 示例包名: "com.your.game.package"
    
    if (g_api->plt_hook_register) {
        g_api->plt_hook_register(0, 0, "__system_property_get", 
                                (void *)my_property_get, 
                                (void **)&orig_system_property_get);
        
        if (g_api->plt_hook_commit()) {
            LOGI("ABI Hook active.");
        }
    }
}

static struct rezygisk_abi module_abi = {
    .api_version = REZYGISK_API_VERSION,
    .pre_app_specialize = pre_app_specialize,
};

__attribute__((visibility("default")))
void zygisk_module_entry(void *api_ptr, void *env) {
    (void)env; // 消除 unused 参数警告
    g_api = (struct rezygisk_api *)api_ptr;
    g_api->register_module(g_api, &module_abi);
    LOGI("CoreProp-Bridge initialized.");
}

__attribute__((visibility("default")))
void zygisk_companion_entry(int client_fd) {
    close(client_fd);
}
