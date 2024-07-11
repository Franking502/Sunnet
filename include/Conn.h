#pragma once
using namespace std;

//连接类与套接字一一对应
class Conn
{
public:
    enum TYPE   //消息类型
    {
        LISTEN = 1,
        CLIENT = 2,
    };

    uint8_t type;
    int fd;
    uint32_t serviceId;
};