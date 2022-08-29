#include "epoller.h"

// 创建epoll对象 epoll_create(512) // size 可忽略的残参数，events_
Epoller::Epoller(int maxEvent):epollFd_(epoll_create(512)), events_(maxEvent){
    assert(epollFd_ >= 0 && events_.size() > 0);
}

Epoller::~Epoller() {
    close(epollFd_);
}

/*
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
添加、修改或移除 epfd对应的监听列表中的实体
This system call is used to add, modify, or remove entries in the interest list of the epoll instance referred to by the file descriptor epfd.
*/
// 添加文件描述符到epoll中进行管理
bool Epoller::AddFd(int fd, uint32_t events) {
    if(fd < 0) return false;
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev);
}

// 修改
bool Epoller::ModFd(int fd, uint32_t events) {
    if(fd < 0) return false;
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev);
}

// 删除
bool Epoller::DelFd(int fd) {
    if(fd < 0) return false;
    epoll_event ev = {0};
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &ev);
}

/*
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);

epoll_wait 等待文件描述符 epfd 引用的 epoll 实例上的事件。
events 为返回值参数，返回就绪列表
maxevents 指定返回最大事件数由
time 指定超时时间

成功返回 就绪的事件数
失败返回 -1，并设置errno
*/

// 调用epoll_wait()进行事件检测，将就绪事件写入events中
int Epoller::Wait(int timeoutMs) {
    return epoll_wait(epollFd_, &events_[0], static_cast<int>(events_.size()), timeoutMs);
}

// 获取产生事件的文件描述符
int Epoller::GetEventFd(size_t i) const {
    assert(i < events_.size() && i >= 0);
    return events_[i].data.fd;
}

// 获取事件
uint32_t Epoller::GetEvents(size_t i) const {
    assert(i < events_.size() && i >= 0);
    return events_[i].events;
}