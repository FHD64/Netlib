#ifndef __NET_CALLBACK__
#define __NET_CALLBACK__

#include <memory>
#include <functional>

namespace netlib {

namespace net {
 class TcpConnection;
 class Connector;
 class Buffer;
 typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
 typedef std::shared_ptr<Connector> ConnectorPtr;
 typedef std::function<void (const TcpConnectionPtr&)> ConnectionCallback;
 typedef std::function<void (const TcpConnectionPtr&)> CloseCallback;
 typedef std::function<void (const TcpConnectionPtr&)> WriteCompleteCallback;
 typedef std::function<void (const TcpConnectionPtr&, size_t)> HighWaterMarkCallback;
 typedef std::function<void (const TcpConnectionPtr&, Buffer*)> MessageCallback;
 typedef std::function<void()> TimerCallback;
}
}
#endif