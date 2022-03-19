# Model
# 1. reactor 
本项目是基于reactor模式的，所以在详细描述网络库的框架之前，我想先大致总结下reactor模式。服务器处理客户端请求时，最经典的处理方式如下所示
> while(fd = accept(fd, &addr, &addrlen)) {  
>　　　//创建线程  
>　　　create_thread(fd, &addr, &addrlen, threadfn);  
>　　　...  
> }  

即对于每个客户端请求，创建一个线程处理对应的请求。但线程的创建与销毁损耗过大，且即使使用线程池的方式仍然会因线程之间的切换而造成性能损失，同时大量线程的存在也会使计算机负载过大，难以支持目前高并发的需求。因此reactor模式即为了解决这种问题而生，即如何在一个线程(进程)内并发处理多个客户端请求，同时框架化以使开发人员更多的关注业务处理。  
reactor模式采用的也是计算机领域的一个常见的思想————中断，相对于用户不停的轮询事件是否发生，不如等待事件发生时通知用户进行处理，因此在我看来reactor模式总共分三步处理客户端请求：  
1. poller等待事件发生，读/写/错误(通常使用epoll/select/poll)  
2. 将事件及事件处理器(event handler)分发(dispatch)给相应的处理线程  
3. 相应线程完成事件处理(调用event handler)，可能会注册/注销poller中的事件，再返回至步骤1  

我认为另一种经常提到的proactor模式与reactor模式最大的不同在于对IO事件的处理顺序不同，但本质上都是利用了中断思想，举例来讲：  
reactor处理读事件是：  
1. 内核通知用户事件可读  
2. 调用read()  
3. 等待read完成后进行相应处理  

proactor处理读事件是：  
1. 调用read()不等待直接返回  
2. 内核处理read  
3. read完成后内核通知用户进行相应处理  

# 2. 网络库中需要的元素
另一个问题就是网络库中需要提供哪些功能。在此我想使用饿了么的corvus(redis集群代理)来进行分析网络库中必备的元素，corvus在饿了么内部有着一定的部署量同时代码质量也比较高，我认为是一个比较成熟的工业软件，因此其功能上的实现也比较有参考意义，netlib中的一些功能实现也采用了corvus的方案。因为主要关注于corvus的网络部分，所以后面的叙述中会略过其业务功能的实现。  
## 2.1 循环
corvus启动初始化之后，所保留的线程一直运行着一个while循环，即event_wait函数，主要代码如下所示
> while(true) {  
> 　　nevents = epoll\_wait(loop->epfd, loop->events, loop->nevent, timeout);  
> 　　if(nevents >= 0) {  
> 　　　　//判断事件类型  
> 　　　　...  
> 　　　　if (e->events & EPOLLIN) mask |= E_READABLE;  
> 　　　　if (e->events & EPOLLOUT) mask |= E_WRITABLE;  
> 　　　　if (e->events & EPOLLHUP) mask |= E_READABLE;   
> 　　　　if (e->events & EPOLLERR) mask |= E_ERROR;  
> 　　　　//调用对应事件的回调函数  
> 　　　　c->ready(c, mask);  
> 　　}  
> 　　...  
> }  

### 一个基本的服务器框架剥离业务功能之后就如上所示：  
1. 一个poller等待事件到来  
2. 判断事件类型  
3. 调用对应事件的回调函数  

## 2.2 poller
第一个可以从业务功能中抽象出来的功能即poller，他至少需要负责2个功能：  
1. 等待事件到来，即epoll\_wait函数  
2. 提供向poller中添加/修改/删除文件描述符(fd)的接口  
corvus中利用event\_reregister、event\_deregister、event\_register三个函数完成上述的第二功能。
> int event\_register(struct event\_loop *loop, struct connection *c, int mask) {  
> 　　struct epoll\_event event;  
> 　　//编写需要添加的事件  
> 　　...  
> 　　if (epoll\_ctl(loop->epfd, EPOL\L_CTL\_ADD, c->fd, &event) == -1) {  
> 　　　　　...  
> 　　}  
> 　　...  
> }  
> int event\_register(struct event\_loop *loop, struct connection *c, int mask) {  
> 　　struct epoll\_event event;  
> 　　//编写需要变更的事件  
> 　　...  
> 　　if (epoll\_ctl(loop->epfd, op, c->fd, &event) == -1) {  
> 　　　　　...  
> 　　}  
> 　　...  
> }  
> int event\_deregister(struct event\_loop *loop, struct connection *c, int mask) {  
> 　　struct epoll\_event event;  
> 　　//编写需要删除的事件  
> 　　...  
> 　　if (epoll_ctl(loop->epfd, EPOL\_CTL\_DEL, c->fd, &event) == -1) {  
> 　　　　　...  
> 　　}  
> 　　...  
> }  

可以看到，三个函数的核心均为epoll\_ctl函数，即对epoll\_ctl函数一个小小的封装。
## 2.3 Acceptor
接下来可以关注的是epoll\_wait中等待的文件描述符(fd),对于服务器来说最显而易见的就是服务器本身的socket，他负责监听端口并接收客户端处理，因此可以只需关注读事件的处理：
> void proxy\_ready(struct connection *self, uint32_t mask){  
> 　　if (mask & E_READABLE) {  
> 　　　　int status;  
> 　　　　while (1) {  
> 　　　　　　status = proxy\_accept(self);  
> 　　　　　　...  
> 　　　　}  
> 　　}  
> 　　...  
> }  
> int proxy_accept(struct connection *proxy){  
> 　　...  
> 　　//调用accept函数接收客户端连接  
> 　　int port, fd = socket\_accept(proxy->fd, ip, sizeof(ip), &port);  
> 　　...  
> 　　//处理新客户端连接  
> 　　if ((client = client\_create(ctx, fd)) == NULL) {   
> 　　　　...  
> 　　}  
> 　　...  
> } 
 
所以，Acceptor的功能有两个：  
1. 接收客户端连接(调用accept函数)  
2. 处理新到来的连接(自定义)
## 2.4 Connection
epoll\_wait中第二种类型的文件描述符为连接的文件描述符，即调用accept后返回的fd,通过分析它的事件处理函数来观察在网络处理中是否有通用的地方。
>void client\_ready(struct connection *self, uint32_t mask) {
>　　...  
>　　if (mask & E\_ERROR) {  
>　　　　//错误事件处理  
>　　}  
>　　if (mask & E\_READABLE) {  
>　　　　//读事件处理  
>　　　　int status = client\_read(self, true);  
>　　　　...  
>　　}  
>　　if (mask & E_WRITABLE) {  
>　　　　//写事件处理  
>　　　　if (client\_write(self) == CORVUS\_ERR) {  
>　　　　　　...  
>　　　　}  
>　　}  
>}  
>int client_read(struct connection *client, bool read_socket) {  
>　　...  
>　　//读数据  
>　　if (read\_socket) {  
>　　　　status = client\_read\_socket(client);  
>　　　　...    
>　　}
>
>　　//处理数据  
>　　...  
>}  
>int client_write(struct connection *client) {  
>　　...  
>　　//写数据  
>　　int status = conn_write(client, 1);  
>
>　　//写完成后进行相应处理  
>　　...  
>}

对于写事件来说，需要完成两步:  
1. 写数据  
2. 写完成后调用写完成处理函数  
对于读数据来说，需要完成两步:  
1. 读数据  
2. 读完成后调用读完成处理函数  

# 3. Model
## 3.1 Channel类
Channel表示一个文件描述符和相应的读、写、错误事件回调处理函数
## 3.2 poller类
poller与上述的功能相同，提供等待事件、修改/添加/删除事件两种功能
## 3.3 EventLoop类和EventLoopThread类
EventLoop即上述的循环代码，集合了poller并利用std::vector管理channel。EventLoopThread将这段循环代码与线程进行绑定，实现one loop one thread的模型。
## 3.4 Acceptor类
负责处理客户端连接，接收并建立新连接，用户可自定义newConnection回调函数，实现新连接接收完成后的处理
## 3.5 TcpConnection类
对应上述的Connect，对应于每条客户端连接，利用C++的RAII机制实现类的生命周期与连接的生命周期同步。用户可自定义messageCallback(读完成后处理函数)和writeCompleteCallback(写完成后处理函数).
## 3.6 TimerQueue类
实现定时器管理。目前定时器管理大致有以下两种方式：  
1. 通过指定epoll\_wait中的timeout实现阶段性时间同步，当epoll\_wait返回后进行定时器校验，判断是否有定时器到时进而执行定时器的回调函数  
2. 利用linux的timerfd机制进行定时器管理，将timerfd与其他文件描述符一同利用epoll管理。timerfd到时后会触发timerfd的可读事件，可通过设置timerfd的读事件完成对定时器的管理  
本项目采用和corvus相同的第二种方法，利用timerfd和c++标准库的std::set完成对定时器的管理
## 3.7 Logger类
实现了同步日志功能，重载了<<符号以实现类c++ stream的使用方式。默认输出为stderr，提供接口可令用户自定义输出目的地。