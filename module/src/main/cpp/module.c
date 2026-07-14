#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/system_properties.h>
#include <android/log.h>
#include "zygisk.h"

#define LOG_TAG "CoreProp-Bridge"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// --- 多包名配置区域 ---
static const char *target_pkgs[] = {
    "com.tencent.tmgp.pubgmhd", // 示例：和平精英
    "com.tencent.tmgp.sgame",   // 示例：王者荣耀
    "com.liuzh.deviceinfo"          // 可以在此添加更多
};

// 检查包名是否匹配列表
static bool is_target(const char *pkg) {
    if (!pkg) return false;
    size_t count = sizeof(target_pkgs) / sizeof(target_pkgs[0]);
    for (size_t i = 0; i < count; i++) {
        if (strstr(pkg, target_pkgs[i]) != NULL) return true;
    }
    return false;
}
// ----------------------

static int (*orig_system_property_get)(const char *, char *);
static int (*orig_system_property_read)(const prop_info *pi, char *name, char *value);

static int my_property_get(const char *key, char *value) {
    if (strcmp(key, "ro.product.cpu.abi") == 0 || strcmp(key, "ro.product.cpu.abilist") == 0) {
        strcpy(value, (strcmp(key, "ro.product.cpu.abi") == 0) ? "x86_64" : "x86_64,x86");
        return (int)strlen(value);
    }
    return orig_system_property_get(key, value);
}

static int my_property_read(const prop_info *pi, char *name, char *value) {
    if (strcmp(name, "ro.product.cpu.abi") == 0 || strcmp(name, "ro.product.cpu.abilist") == 0) {
        strcpy(value, (strcmp(name, "ro.product.cpu.abi") == 0) ? "x86_64" : "x86_64,x86");
        return (int)strlen(value);
    }
    return orig_system_property_read(pi, name, value);
}

static struct rezygisk_api *g_api = NULL;

static void pre_app_specialize(void *self, void *args) {
    (void)self;
    struct app_specialize_args_v5 *app_args = (struct app_specialize_args_v5 *)args;
    const char *pkg = app_args->nice_name ? (const char*)app_args->nice_name : "unknown";

    // 使用白名单过滤
    if (!is_target(pkg)) return;

    LOGI("Target detected: %s, applying hooks...", pkg);

    if (g_api->plt_hook_register) {
        g_api->plt_hook_register(0, 0, "__system_property_get", (void *)my_property_get, (void **)&orig_system_property_get);
        g_api->plt_hook_register(0, 0, "__system_property_read", (void *)my_property_read, (void **)&orig_system_property_read);
        
        g_api->plt_hook_commit();
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
}

__attribute__((visibility("default")))
void zygisk_companion_entry(int client_fd) {
    close(client_fd);
}
