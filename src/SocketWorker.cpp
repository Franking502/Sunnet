#include "SocketWorker.h"
#include <iostream>
#include <unistd.h>
#include <assert.h>
#include <string.h>

void SocketWorker::Init()
{
    cout << "SocketWorker Init" << endl;

    epollFd = epoll_create(1024);
    assert(epollFd > 0);
    cout << "epollFd:" << epollFd << endl;
}

void SocketWorker::operator()()
{
    while(true)
    {
        const int EVENT_SIZE = 64;
        struct epoll_event events[EVENT_SIZE];
        //事件发生时会被放到events数组中，每次调用最后获取EVENT_SIZE个事件，没获取到的下次调用时获取
        int eventCount = epoll_wait(epollFd, events, EVENT_SIZE, -1);

        for(int i = 0; i < eventCount; i++)
        {
            epoll_event ev = events[i];
            OnEvent(ev);
        }
    }
}

void SocketWorker::AddEvent(int fd)
{
    cout << "AddEvent fd " << fd << endl;

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = fd;
    if(epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &ev) == -1)//epoll_ctl：修改epoll对象监听列表，参数3是要操作的文件描述符，此处是Socket
    {
        cout << "AddEvent epoll_ctl Fail:" << strerror(errno) << endl;
    }
}

void SocketWorker::RemoveEvent(int fd)
{
    cout << "RemoveEvent fd " << fd << endl;
    epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, NULL);
}

void SocketWorker::ModifyEvent(int fd, bool epollOut)
{
    cout << "ModifyEvent fd" << fd << " " << epollOut << endl;
    struct epoll_event ev;
    ev.data.fd = fd;

    if(epollOut)
    {
        ev.events = EPOLLIN | EPOLLET | EPOLLOUT;
    }
    else
    {
        ev.events = EPOLLIN | EPOLLET;
    }
    epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &ev);
}

void SocketWorker::OnEvent(struct epoll_event ev)
{
    cout << "OnEvent" << endl;
}