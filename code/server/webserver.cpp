#include "webserver.h"

using namespace std;

WebServer::WebServer(
            int port, int trigMode, int timeoutMS, bool OptLinger,
            int sqlPort, const char* sqlUser, const  char* sqlPwd,
            const char* dbName, int connPoolNum, int threadNum,
            bool openLog, int logLevel, int logQueSize):
            port_(port), openLinger_(OptLinger), timeoutMS_(timeoutMS), isClose_(false),
            timer_(new HeapTimer()), threadpool_(new ThreadPool(threadNum)), epoller_(new Epoller())
    {

    srcDir_ = getcwd(nullptr, 256); // 获取当前的工作路径，调用了  malloc
    assert(srcDir_);
    strncat(srcDir_, "/resources/", 16);    // 拼接资源路径
    /* Append no more than N characters from SRC onto DEST.  */

    // 当前所有连接数
    HttpConn::userCount = 0;
    HttpConn::srcDir = srcDir_;

    // 初始化数据库连接池
    SqlConnPool::Instance()->Init("localhost", sqlPort, sqlUser, sqlPwd, dbName, connPoolNum);

    // 初始化事件的模式 ET | LT 
    InitEventMode_(trigMode);
    
    // 初始化网络通信相关的一些内容
    if(!InitSocket_()) { isClose_ = true;}

    if(openLog) {
        // 初始化日志信息
        Log::Instance()->init(logLevel, "./log", ".log", logQueSize);
        if(isClose_) { LOG_ERROR("========== Server init error!=========="); }
        else {
            LOG_INFO("========== Server init ==========");
            LOG_INFO("Port:%d, OpenLinger: %s", port_, OptLinger? "true":"false");
            LOG_INFO("Listen Mode: %s, OpenConn Mode: %s",
                            (listenEvent_ & EPOLLET ? "ET": "LT"),
                            (connEvent_ & EPOLLET ? "ET": "LT"));
            LOG_INFO("LogSys level: %d", logLevel);
            LOG_INFO("srcDir: %s", HttpConn::srcDir);
            LOG_INFO("SqlConnPool num: %d, ThreadPool num: %d", connPoolNum, threadNum);
        }
    }
}

WebServer::~WebServer() {
    close(listenFd_);
    isClose_ = true;
    free(srcDir_);
    SqlConnPool::Instance()->ClosePool();
}

// 设置监听的文件描述符和通信的文件描述符的触发模式
void WebServer::InitEventMode_(int trigMode) {
    listenEvent_ = EPOLLRDHUP;  // 由于监听的 socket 只有一个，所以不需要设置 EPOLLONESHOT
    connEvent_ = EPOLLONESHOT | EPOLLRDHUP;
    /*
        EPOLLRDHUP 在对端关闭时发送事件
        
        epoll模式中事件可能被触发多次，比如socket接收到数据交给一个线程处理数据，在数据没有处理完之前又有新数据达到触发了事件，另一个线程被激活获得该socket，从而产生多个线程操作同一socket，即使在ET模式下也有可能出现这种情况。
        EPOLLONESHOT 使用epoll_wait接收事件后禁用对应的文件描述符

        ET: 边缘触发，被监控的文件描述符变化时发送事件，需要一次性把数据读完，非阻塞文件描述符 
        LT: 水平触发，有就绪的文件描述符就发送事件（默认）
    */
    switch (trigMode)
    {
    case 0:
        break;
    case 1:
        connEvent_ |= EPOLLET;
        break;
    case 2:
        listenEvent_ |= EPOLLET;
        break;
    case 3:
        listenEvent_ |= EPOLLET;
        connEvent_ |= EPOLLET;
        break;
    default:
        listenEvent_ |= EPOLLET;
        connEvent_ |= EPOLLET;
        break;
    }
    HttpConn::isET = (connEvent_ & EPOLLET);
}


// 启动服务器
void WebServer::Start() {
    int timeMS = -1;  /* epoll wait timeout == -1 无事件将阻塞 */
    if(!isClose_) { LOG_INFO("========== Server start =========="); }
    while(!isClose_) {

        // 如果设置了超时时间，例如60s,则只要一个连接60秒没有读写操作，则关闭
        if(timeoutMS_ > 0) {
            // 通过定时器GetNextTick(),清除超时的节点，然后获取最先要超时的连接的超时的时间
            timeMS = timer_->GetNextTick();
        }

        // timeMS是最先要超时的连接的超时的时间，传递到epoll_wait()函数中
        // 当timeMS时间内有事件发生，epoll_wait()返回，否则等到了timeMS时间后才返回
        // 这样做的目的是为了让epoll_wait()调用次数变少，提高效率
        int eventCnt = epoller_->Wait(timeMS);

        // 循环处理每一个事件
        for(int i = 0; i < eventCnt; i++) {
            /* 处理事件 */
            int fd = epoller_->GetEventFd(i);   // 获取事件对应的fd
            uint32_t events = epoller_->GetEvents(i);   // 获取事件的类型
            
            // 监听的文件描述符有事件，说明有新的连接进来
            if(fd == listenFd_) {
                DealListen_();  // 处理监听的操作，接受客户端连接
            }
            
            // 错误的一些情况
            else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                assert(users_.count(fd) > 0);
                CloseConn_(&users_[fd]);    // 关闭连接
            }

            // 有数据到达
            else if(events & EPOLLIN) {
                assert(users_.count(fd) > 0);
                DealRead_(&users_[fd]); // 处理读操作
            }
            
            // 可以发送数据
            else if(events & EPOLLOUT) {
                assert(users_.count(fd) > 0);
                DealWrite_(&users_[fd]);    // 处理写操作
            } else {
                LOG_ERROR("Unexpected event");
            }
        }
    }
}

// 发送错误提示信息
void WebServer::SendError_(int fd, const char*info) {
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    if(ret < 0) {
        LOG_WARN("send error to client[%d] error!", fd);
    }
    close(fd);
}

// 关闭连接（从epoll中删除，解除响应对象中的内存映射，用户数递减，关闭文件描述符）
void WebServer::CloseConn_(HttpConn* client) {
    assert(client);
    LOG_INFO("Client[%d] quit!", client->GetFd());
    epoller_->DelFd(client->GetFd());
    client->Close();
}

// 添加客户端
void WebServer::AddClient_(int fd, sockaddr_in addr) {
    assert(fd > 0);
    users_[fd].init(fd, addr); // 该key不存在，访问仍然成功，取得HttpConn对象默认构造的值
    if(timeoutMS_ > 0) {
        // 添加到定时器对象中，当检测到超时时执行CloseConn_函数进行关闭连接
        timer_->add(fd, timeoutMS_, std::bind(&WebServer::CloseConn_, this, &users_[fd]));
    }
    // 添加到epoll中进行管理
    epoller_->AddFd(fd, EPOLLIN | connEvent_);
    // 设置文件描述符非阻塞
    SetFdNonblock(fd);
    LOG_INFO("Client[%d] in!", users_[fd].GetFd());
}

void WebServer::DealListen_() {
    struct sockaddr_in addr; // 保存连接的客户端的信息
    socklen_t len = sizeof(addr);
    // 如果监听文件描述符设置的是 ET模式，则需要循环把所有连接处理了
    do {
        int fd = accept(listenFd_, (struct sockaddr *)&addr, &len);
        if(fd <= 0) { return;}
        else if(HttpConn::userCount >= MAX_FD) {
            SendError_(fd, "Server busy!");
            LOG_WARN("Clients is full!");
            return;
        }
        AddClient_(fd, addr);   // 添加客户端
    } while(listenEvent_ & EPOLLET);
}

// 处理读
void WebServer::DealRead_(HttpConn* client) {
    assert(client);
    ExtentTime_(client);   // 延长这个客户端的超时时间
    // 加入到队列中等待线程池中的线程处理（读取数据）
    threadpool_->AddTask(std::bind(&WebServer::OnRead_, this, client));
}

// 处理写
void WebServer::DealWrite_(HttpConn* client) {
    assert(client);
    ExtentTime_(client);// 延长这个客户端的超时时间
    // 加入到队列中等待线程池中的线程处理（写数据）
    threadpool_->AddTask(std::bind(&WebServer::OnWrite_, this, client));
}

// 延长客户端的超时时间
void WebServer::ExtentTime_(HttpConn* client) {
    assert(client);
    if(timeoutMS_ > 0) { timer_->adjust(client->GetFd(), timeoutMS_); }
}

// 这个方法是在子线程中执行的（读取数据）
void WebServer::OnRead_(HttpConn* client) {
    assert(client);
    int ret = -1;
    int readErrno = 0;
    ret = client->read(&readErrno); // 读取客户端的数据
    if(ret <= 0 && readErrno != EAGAIN) {
        CloseConn_(client);
        return;
    }

    // 业务逻辑的处理
    OnProcess(client);
}

// 业务逻辑的处理
void WebServer::OnProcess(HttpConn* client) {
    if(client->process()) {
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
    } else {
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLIN);
    }
}

// 写数据
void WebServer::OnWrite_(HttpConn* client) {
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->write(&writeErrno);   // 写数据

    // 如果将要写的字节等于0，说明写完了，判断是否要保持连接，保持连接继续去处理
    if(client->ToWriteBytes() == 0) {
        /* 传输完成 */
        if(client->IsKeepAlive()) {
            OnProcess(client);
            return;
        }
    }
    else if(ret < 0) {
        if(writeErrno == EAGAIN) {
            /* 继续传输 */
            epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
            return;
        }
    }
    CloseConn_(client);
}

/* Create listenFd */
bool WebServer::InitSocket_() {
    // 初始化监听socket
    int ret;
    struct sockaddr_in addr;
    if(port_ > 65535 || port_ < 1024) {
        LOG_ERROR("Port:%d error!",  port_);
        return false;
    }

    /*
    socket INADDR_ANY 监听0.0.0.0地址 socket只绑定端口让路由表决定传到哪个ip

    其中INADDR_ANY就是指定地址为0.0.0.0的地址,这个地址事实上表示不确定地址,或“所有地址”、“任意地址”。
    如果指定ip地址为通配地址(INADDR_ANY)，那么内核将等到套接字已连接(TCP)或已在套接字上发出数据报时才选择一个本地IP地址。
    */
    addr.sin_family = AF_INET; // 地址族： IPV4
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // 字节序转换，hostlong to network byte
    addr.sin_port = htons(port_);
    struct linger optLinger = { 0 }; 
    
    if(openLinger_) {
        /* 优雅关闭: 直到所剩数据发送完毕或超时 */
        optLinger.l_onoff = 1; // 在closesocket 时调用， 但是还有数据没发送完毕时允许逗留
        optLinger.l_linger = 1; // 允许逗留时间为 1s
    }


    // 监听套接字文件描述符
    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if(listenFd_ < 0) {
        LOG_ERROR("Create socket error!", port_);
        return false;
    }

    // 设置socket属性优雅关闭与允许逗留时间
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
    if(ret < 0) {
        close(listenFd_);
        LOG_ERROR("Init linger error!", port_);
        return false;
    }

    int optval = 1;
    /* 端口复用 */
    /* 只有最后一个套接字会正常接收数据。 */
    /*
        端口复用最常用的用途应该是防止服务器重启时之前绑定的端口还未释放或者程序突然退出而系统没有释放端口。
        这种情况下如果设定了端口复用，则新启动的服务器进程可以直接绑定端口。
        如果没有设定端口复用，绑定会失败，提示ADDR已经在使用中。
    */
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
    if(ret == -1) {
        LOG_ERROR("set socket setsockopt error !");
        close(listenFd_);
        return false;
    }

    // 绑定 ip port
    ret = bind(listenFd_, (struct sockaddr *)&addr, sizeof(addr));
    if(ret < 0) {
        LOG_ERROR("Bind Port:%d error!", port_);
        close(listenFd_);
        return false;
    }

    // 监听listenFd_套接字，维护的最大还未处理的链接数 
    ret = listen(listenFd_, 6);
    if(ret < 0) {
        LOG_ERROR("Listen port:%d error!", port_);
        close(listenFd_);
        return false;
    }

    // 将listenFd_ 添加到 epoller 的 insert list 里，并设置对应的事件
    ret = epoller_->AddFd(listenFd_,  listenEvent_ | EPOLLIN); // listenFd_ 上监听的事件有 IN RDHUP ET模式
    if(ret == 0) {
        LOG_ERROR("Add listen error!");
        close(listenFd_);
        return false;
    }
    SetFdNonblock(listenFd_);
    LOG_INFO("Server port:%d", port_);
    return true;
}

// 设置文件描述符非阻塞
int WebServer::SetFdNonblock(int fd) {
    assert(fd > 0);
    // int flag = fcntl(fd, F_GETFD, 0);
    // flag = flag  | O_NONBLOCK;
    // // flag  |= O_NONBLOCK;
    // fcntl(fd, F_SETFL, flag);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}


