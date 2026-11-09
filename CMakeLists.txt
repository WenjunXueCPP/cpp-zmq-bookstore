# 可执行目标：客户端、服务端、压测客户端
add_executable(client client.cpp)
add_executable(srv srv.cpp)
add_executable(bench_client bench_client.cpp)

# 静态库：把全部源码打进去（保留原有做法）
add_library(section5
    client.cpp conf.lua Config.hpp cpplang.hpp hello.hpp
    ThreadPool.hpp SalesData.hpp SpinLock.hpp srv.cpp Summary.hpp Zmq.hpp)

install(TARGETS section5 DESTINATION bin)
install(FILES
    client.cpp conf.lua Config.hpp cpplang.hpp hello.hpp
    ThreadPool.hpp SalesData.hpp SpinLock.hpp srv.cpp Summary.hpp Zmq.hpp
    DESTINATION include)

# 头文件查找路径
include_directories(/usr/include/luajit-2.1)
include_directories(${CMAKE_SOURCE_DIR}/LuaBridge/Source)
include_directories(/usr/include)

# 链接库
target_link_libraries(client zmq luajit-5.1 dl pthread)
target_link_libraries(srv zmq cpr curl luajit-5.1 dl pthread ssl crypto)
target_link_libraries(bench_client zmq pthread)
target_link_libraries(section5 zmq cpr curl luajit-5.1 dl pthread ssl crypto)
