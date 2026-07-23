# 第1段：生成可执行文件、库（你原本就有的内容，不动）
add_executable(client client.cpp)
add_executable(srv srv.cpp)
add_library(section5 client.cpp conf.lua Config.hpp cpplang.hpp hello.hpp SalesData.hpp SpinLock.hpp srv.cpp Summary.hpp Zmq.hpp)


install (TARGETS section5 DESTINATION bin)
install (FILES client.cpp conf.lua Config.hpp cpplang.hpp hello.hpp SalesData.hpp SpinLock.hpp srv.cpp Summary.hpp Zmq.hpp DESTINATION include)


# 第2段：头文件查找路径 → 放在add_executable前面、或者install前后都可以，推荐放在最上方
# 原来的，保留不动
include_directories(/usr/include/luajit-2.1)
# 新增这一行，用来找 LuaBridge
include_directories(${CMAKE_SOURCE_DIR}/LuaBridge/Source)
include_directories(/usr/include)


# 第3段：链接库命令 → 剪切到【整个CMakeLists.txt的最后一行】，绝对不能放在中间
target_link_libraries(client zmq luajit-5.1 dl pthread)
target_link_libraries(srv zmq cpr curl luajit-5.1 dl pthread ssl crypto)
target_link_libraries(section5 zmq cpr curl luajit-5.1 dl pthread ssl crypto)