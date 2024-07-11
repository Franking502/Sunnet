#pragma once
#include <memory>
using namespace std;

class BaseMsg
{
public:
    enum TYPE
    {
        SERVICE = 1,
        SOCKET_ACCEPT = 2,//有新客户端连接
        SOCKET_RW = 3,//客户端可读可写
    };
    uint8_t type;
    virtual ~BaseMsg(){};
};

class ServiceMsg : public BaseMsg
{
public:
    uint32_t source;//消息发送方
    shared_ptr<char> buff;//消息内容
    size_t size;//消息大小
};

//有新客户端连接
class SocketAcceptMsg : public BaseMsg
{
public:
    int listenFd;
    int clientFd;
};

//客户端可读可写
class SocketRWMsg : public BaseMsg
{
public:
    int fd;
    bool isRead = false;
    bool isWrite = false;
};