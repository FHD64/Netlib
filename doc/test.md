# Test
测试方法参照了asio的pingpong测试，以进行吞吐量的测试。因为网络部分的主要抽象设计参考了muduo，所以以muduo
作为测试参照对象。测试分为两部分，一部分为长连接测试，另一部分为短连接测试，观察netlib和muduo两者的性能表现。  
# 长连接
测试方法:分别以4K、8K、16K作为发送信息大小，测试服务器在连接数为1、10、100、1000、10000的情况下吞吐量表现。
## 令模拟客户端和服务器均为单线程  
![](https://github.com/FHD64/Netlib/raw/master/doc/test/4K_单线程.png)
![](https://github.com/FHD64/Netlib/raw/master/doc/test/8K_单线程.png)
![](https://github.com/FHD64/Netlib/raw/master/doc/test/16K_单线程.png)   
   
   
## 令模拟客户端和服务器均为双线程
![](https://github.com/FHD64/Netlib/raw/master/doc/test/4K_双线程.png)
![](https://github.com/FHD64/Netlib/raw/master/doc/test/8K_双线程.png)
![](https://github.com/FHD64/Netlib/raw/master/doc/test/16K_双线程.png)

可以看出来netlib与muduo的折线几乎重合，对于长连接的吞吐量性能相当
# 短连接
测试方法：以8K作为发送信息大小，每条连接维持100ms后关闭，每条连接接入、断开5000次。分别测试连接数在1000和2000的情况下吞吐量大小。  
|      | 1000  | 2000  |  
| ---- | ----  |----   |  
|muduo  | 495.5  | 280.2 |  
|netlib | 547.9  | 297.7 |  

在短连接的吞吐量测试中，netlib表现更加优异，比muduo吞吐量约提高5~10%
# 性能分析
短连接吞吐量的优异表现我认为得以于三点：
1. muduo的Acceptor每次仅accept一次，因此对于客户端请求的接入需要反复调用poller；而netlib决定在一次事件中反复accept直至无法接入新的连接，减少了poller的调用次数。
2. Buffer池的使用使得每次buffer申请、释放速度提升
3. 连接池的使用使得每次连接接入无需重新new TcpConnection，减少了连接接入时申请TcpConnection的时间。