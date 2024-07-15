#include <iostream>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include "SocketWorker.h"
#include "Sunnet.h"

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
        //事件发生时会被放到events数组中，每次调用最多获取EVENT_SIZE个事件，没获取到的下次调用时获取
        //第四个参数是超时时间，-1为无限等待
        int eventCount = epoll_wait(epollFd, events, EVENT_SIZE, -1);//阻塞等待

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
        ev.events = EPOLLIN | EPOLLET | EPOLLOUT;//EPOLLET：边缘触发方式
    }
    else
    {
        ev.events = EPOLLIN | EPOLLET;
    }
    epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &ev);
}

void SocketWorker::OnEvent(struct epoll_event ev)
{
    int fd = ev.data.fd;
    auto conn = Sunnet::inst->GetConn(fd);
    if(conn == NULL)
    {
        cout << "SocketWorker::OnEvent error, conn == NULL" << endl;
        return;
    }

    bool isRead = ev.events & EPOLLIN;
    bool isWrite = ev.events & EPOLLOUT;
    bool isError = ev.events & EPOLLERR;

    //事件类型
    if(conn->type == Conn::TYPE::LISTEN)
    {
        if(isRead)
        {
            OnAccept(conn);
        }
    }
    //普通socket
    else
    {
        if(isRead || isWrite)
        {
            OnRW(conn, isRead, isWrite);
        }
        if(isError)
        {
            cout << "OnError fd: " << conn->fd << endl;
        }
    }
}

void SocketWorker::OnAccept(shared_ptr<Conn> conn)
{
    cout << "OnAccept fd:" << conn->fd << endl;
    //1.accept
    int clientFd = accept(conn->fd, NULL, NULL);
    if(clientFd < 0)
    {
        cout << "accept error" << endl;
    }

    //2.设置非阻塞
    fcntl(clientFd, F_SETFL, O_NONBLOCK);
    //设置写缓冲区大小为4GB
    unsigned long buffSize = 4294967295;
    if(setsockopt(clientFd, SOL_SOCKET, SO_SNDBUFFORCE, &buffSize, sizeof(buffSize)) < 0)
    {
        cout << "OnAccept setsockopt fail " << strerror(errno) << endl;
    }

    //3.设置连接对象
    Sunnet::inst->AddConn(clientFd, conn->serviceId, Conn::TYPE::CLIENT);

    //4.添加到epoll监听列表
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = clientFd;
    if(epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &ev) == -1)
    {
        cout << "OnAccept epoll_ctl Fail:" << strerror(errno) << endl;
    }

    //5.通知服务
    auto msg = make_shared<SocketAcceptMsg>();
    msg->type = BaseMsg::TYPE::SOCKET_ACCEPT;
    msg->listenFd = conn->fd;
    msg->clientFd = clientFd;
    Sunnet::inst->Send(conn->serviceId, msg);
}

//普通读写事件响应
void SocketWorker::OnRW(shared_ptr<Conn> conn, bool r, bool w)
{
    cout << "OnRW fd:" << conn->fd << endl;
    auto msg = make_shared<SocketRWMsg>();
    msg->type = BaseMsg::TYPE::SOCKET_RW;
    msg->fd = conn->fd;
    msg->isRead = r;
    msg->isWrite = w;
    Sunnet::inst->Send(conn->serviceId, msg);
}