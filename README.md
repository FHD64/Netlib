# Netlib
---
## Introduction
本项目为c++11编写的多线程高性能网络库。该项目运行于linux平台下，支持Tcp连接。同时提供了Mutex、condvar等多种线程同步手段。
实现了时间戳、线程池、同步日志记录等功能。
## Build
./netlib/build.sh
## Usage
可参考./netlib/net/test/目录下的EchoClient.cpp和EchoServer.cpp，实现了一个基本的Echo服务器及客户端。
## Technical points
* 使用Epoll边沿触发的IO多路复用，非阻塞IO，基于Reactor模式
* 使用线程池、连接池、内存池，避免反复申请释放而带来的开销
* 使用linux特性timerfd和set进行定时器管理
* 采用one loop one thread的方式，主线程进行accept，以Round Robin的方式分发其他IO线程，将资源申请锁定在同一线程内，减少锁相互争用带来的开销
* 提供Mutex、CondVar、BlockQueue等多种线程同步手段
* 利用shared_ptr<>、unique_ptr<>，实现内存自动管理
* 利用RAII机制，将Tcp连接生命周期与对象生命周期同步
## Others
1.本项目网络部分主要参考了muduo的抽象设计，十分感谢chen shuo  
2.项目的一些实现思想参考了leveldb、corvus(饿了么实现的redis集群代理)，感谢这些项目  
3.性能测试部分参考了asio的测试方法
