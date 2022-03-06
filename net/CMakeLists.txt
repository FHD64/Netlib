set(NET_SRCS
  Buffer.cpp
  Acceptor.cpp
  Channel.cpp
  Connector.cpp
  Epoller.cpp
  EventLoop.cpp
  EventLoopThread.cpp
  EventLoopThreadPool.cpp
  InetAddress.cpp
  Socket.cpp
  TcpClient.cpp
  TcpConnection.cpp
  TcpServer.cpp
  Timer.cpp
  TimerQueue.cpp
)

include_directories(./)
include_directories(../base)

add_library(netlib_net ${NET_SRCS})
target_link_libraries(netlib_net netlib_base pthread rt)

install(TARGETS netlib_net DESTINATION lib)

add_executable(Buffer_test ./test/Buffer_test.cpp)
target_link_libraries(Buffer_test netlib_net)

add_executable(InetAddress_test ./test/InetAddress_test.cpp)
target_link_libraries(InetAddress_test netlib_net)

add_executable(EventLoop_test ./test/EventLoop_test.cpp)
target_link_libraries(EventLoop_test netlib_net)

add_executable(EventLoopThread_test ./test/EventLoopThread_test.cpp)
target_link_libraries(EventLoopThread_test netlib_net)

add_executable(EventLoopThreadPool_test ./test/EventLoopThreadPool_test.cpp)
target_link_libraries(EventLoopThreadPool_test netlib_net)

add_executable(TimerQueue_test ./test/TimerQueue_test.cpp)
target_link_libraries(TimerQueue_test netlib_net)

add_executable(EchoServer_test ./test/EchoServer_test.cpp)
target_link_libraries(EchoServer_test netlib_net)

add_executable(EchoClient_test ./test/EchoClient_test.cpp)
target_link_libraries(EchoClient_test netlib_net)