#pragma once

#include <sys/epoll.h>
#include <memory>
#include "Conn.h"

using namespace std;

class SocketWorker
{
private:
    int epollFd;//epoll描述符

public:
    void Init();//初始化函数，系统启动时调用
    void operator()();//线程函数

    void AddEvent(int fd);
    void RemoveEvent(int fd);
    void ModifyEvent(int fd, bool epollOut);

private:
    void OnEvent(struct epoll_event ev);
    void OnAccept(shared_ptr<Conn> conn);
    void OnRW(shared_ptr<Conn> conn, bool r, bool w);
};