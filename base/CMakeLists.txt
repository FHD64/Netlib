set(BASE_SRCS
  Timestamp.cpp
  CurrentThread.cpp
  ThreadPool.cpp
  Thread.cpp
  Logger.cpp
  LogStream.cpp
  Condition.cpp
  CountDownLatch.cpp
  ./test/Timestamp_test.cpp
  ./test/Thread_test.cpp
  ./test/Mutex_test.cpp
  ./test/LogStream_test.cpp
  ./test/Logger_test.cpp
  ./test/ThreadPool.cpp
)

include_directories(./)
include_directories(./test)

add_library(netlib_base ${BASE_SRCS})
target_link_libraries(netlib_base pthread rt)

install(TARGETS netlib_base DESTINATION lib)

add_executable(Timestamp_test ./test/Timestamp_test.cpp)
target_link_libraries(Timestamp_test netlib_base)

add_executable(Thread_test ./test/Thread_test.cpp)
target_link_libraries(Thread_test netlib_base)

add_executable(Mutex_test ./test/Mutex_test.cpp)
target_link_libraries(Mutex_test netlib_base)

add_executable(LogStream_test ./test/LogStream_test.cpp)
target_link_libraries(LogStream_test netlib_base)

add_executable(Logger_test ./test/Logger_test.cpp)
target_link_libraries(Logger_test netlib_base)

add_executable(ThreadPool_test ./test/ThreadPool_test.cpp)
target_link_libraries(ThreadPool_test netlib_base)

add_executable(BlockingQueue_test ./test/BlockingQueue_test.cpp)
target_link_libraries(BlockingQueue_test netlib_base)