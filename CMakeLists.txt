cmake_minimum_required(VERSION 3.20)
project(CMDClickerX)

set(CMAKE_CXX_STANDARD 20)

add_executable(CMDClickerX main.cpp
        Selector.h
        Selector.cpp
        mytools.cpp
        runner.cpp
        resource.rc
)
if (MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
endif()
# 链接 Windows 加密库
target_link_libraries(CMDClickerX
        advapi32    # 用于 CryptoAPI (CryptAcquireContext 等)
        bcrypt      # 用于 BCrypt 函数 (如果使用)
)