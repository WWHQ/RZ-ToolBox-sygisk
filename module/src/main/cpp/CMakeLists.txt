# CMakeLists.txt for CoreProp-Bridge Zygisk Module

cmake_minimum_required(VERSION 3.10)

# 设置模块名称为 coreprop_bridge
set(MODULE_NAME coreprop_bridge)
project(${MODULE_NAME} C)

# 源文件列表：将你的 module.c 放在此处
set(SOURCES
    module.c
)

# 构建共享库 (Zygisk 模块必须是 .so)
add_library(${MODULE_NAME} SHARED ${SOURCES})

# 编译器标志优化：增加性能与隐蔽性
target_compile_options(${MODULE_NAME} PRIVATE
    -Wall
    -Wextra
    -O3                # 使用最高优化等级，减小内存占用
    -fvisibility=hidden # 隐藏非必要符号，只暴露 Zygisk 入口
    -ffunction-sections
    -fdata-sections
)

# 发布模式：禁用 Debug 日志，进一步减小体积和隐藏行为
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(${MODULE_NAME} PRIVATE NDEBUG)
endif()

# 链接器优化
target_link_options(${MODULE_NAME} PRIVATE
    -Wl,--gc-sections  # 移除未使用的代码段
    -Wl,--strip-all    # 去除调试符号，防止 TP 通过静态分析抓包
)

# 链接 Android 日志库
find_library(LOG_LIB log)
target_link_libraries(${MODULE_NAME} PRIVATE ${LOG_LIB})

# 安装路径配置
install(TARGETS ${MODULE_NAME}
    LIBRARY DESTINATION lib/${ANDROID_ABI}
)
