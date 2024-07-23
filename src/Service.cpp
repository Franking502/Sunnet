#include "Service.h"
#include "Sunnet.h"
#include "LuaAPI.h"
#include <iostream>
#include <unistd.h>
#include <string.h>

Service::Service()
{
    //初始化锁
    pthread_spin_init(&queueLock, PTHREAD_PROCESS_PRIVATE);
    pthread_spin_init(&inGlobalLock, PTHREAD_PROCESS_PRIVATE);
}

Service::~Service()
{
    pthread_spin_destroy(&queueLock);
    pthread_spin_destroy(&inGlobalLock);
}

void Service::PushMsg(shared_ptr<BaseMsg> msg)
{
    pthread_spin_lock(&queueLock);
    {
        msgQueue.push(msg);
    }
    pthread_spin_unlock(&queueLock);
}

shared_ptr<BaseMsg> Service::PopMsg()
{
    shared_ptr<BaseMsg> msg = NULL;

    pthread_spin_lock(&queueLock);
    {
        if(!msgQueue.empty())
        {
            msg = msgQueue.front();
            msgQueue.pop();
        }
    }
    pthread_spin_unlock(&queueLock);
    return msg;
}

//创建服务后触发
void Service::OnInit()
{
    cout << "[" << id << "] OnInit" << endl;

    //新建Lua虚拟机
    luaState = luaL_newstate();
    luaL_openlibs(luaState);//初始化
    //注册Sunnet系统API
    LuaAPI::Register(luaState);
    //执行lua文件
    string filename = "../service/" + *type + "/init.lua";
    int isok = luaL_dofile(luaState, filename.data());
    if(isok == 1)//0-成功 1-失败
    {
        cout << "run lua fail: " << lua_tostring(luaState, -1) << endl;
    }
    //调用lua函数
    lua_getglobal(luaState, "OnInit");
    lua_pushinteger(luaState, id);//参数压栈
    isok = lua_pcall(luaState, 1, 0, 0);
    if(isok != 0)
    {
        cout << "call lua OnInit fail " << lua_tostring(luaState, -1) << endl;
    }

    //开启监听
    //Sunnet::inst->Sunnet::Listen(8002, id);
}

//收到消息时触发
void Service::OnMsg(shared_ptr<BaseMsg> msg)
{
    //SERVICE
    if(msg->type == BaseMsg::TYPE::SERVICE)
    {
        auto m = dynamic_pointer_cast<ServiceMsg>(msg);
        OnServiceMsg(m);
    }

    //SOCKET_ACCEPT
    else if(msg->type == BaseMsg::TYPE::SOCKET_ACCEPT)
    {
        auto m = dynamic_pointer_cast<SocketAcceptMsg>(msg);
        OnAcceptMsg(m);
    }

    //SOCKET_RW
    else if(msg->type == BaseMsg::TYPE::SOCKET_RW)
    {
        auto m = dynamic_pointer_cast<SocketRWMsg>(msg);
        OnRWMsg(m);
    }
}

//退出服务时触发
void Service::OnExit()
{
    cout << "[" << id << "] OnExit" << endl;
    lua_getglobal(luaState, "OnExit");
    int isok = lua_pcall(luaState, 0, 0, 0);
    if(isok != 0)
    {
        cout << "call lua OnExit fail" << lua_tostring(luaState, -1) << endl;
    }
    //关闭虚拟机
    lua_close(luaState);
}

//消息处理，返回消息是否被处理
bool Service::ProcessMsg()
{
    shared_ptr<BaseMsg> msg = PopMsg();
    if(msg)
    {
        OnMsg(msg);
        return true;
    }
    else
    {
        return false;
    }
}

void Service::ProcessMsgs(int max)
{
    for(int i = 0; i < max; i++)
    {
        bool succ = ProcessMsg();
        if(!succ)
        {
            break;
        }
    }
}

void Service::SetInGlobal(bool isIn)
{
    pthread_spin_lock(&inGlobalLock);
    {
        inGlobal = isIn;
    }
    pthread_spin_unlock(&inGlobalLock);
}

//收到其他服务的消息
void Service::OnServiceMsg(shared_ptr<ServiceMsg> msg)
{
    cout << "OnServiceMsg" << endl;
}

//新连接
void Service::OnAcceptMsg(shared_ptr<SocketAcceptMsg> msg)
{
    cout << "OnAcceptMsg " << msg->clientFd << endl;
}

//套接字可读可写
void Service::OnRWMsg(shared_ptr<SocketRWMsg> msg)
{
    int fd = msg->fd;

    if(msg->isRead)
    {
        const int BUFFSIZE = 512;
        char buff[BUFFSIZE];
        int len = 0;
        do
        {
            len = read(fd, &buff, BUFFSIZE);
            if(len > 0)
            {
                OnSocketData(fd, buff, len);
            }
        }while(len == BUFFSIZE);

        //如果上一次读取的长度恰好为512且全部读完，则errno的值为EAGAIN
        if(len <= 0 && errno != EAGAIN)
        {
            if(Sunnet::inst->GetConn(fd))
            {
                //保证OnSocketClose只调用一次
                OnSocketClose(fd);
                Sunnet::inst->CloseConn(fd);
            }
        }
    }
    
    if(msg->isWrite)
    {
        if(Sunnet::inst->GetConn(fd))
        {
            OnSocketWritable(fd);
        }
    }
}

void Service::OnSocketData(int fd, const char* buff, int len)
{
    cout << "OnSocketData: " << fd << "buff: " << buff << endl;
    char writeBuff[4] = {'l', 'p', 'y', '\n'};
    write(fd, &writeBuff, 4);
}

void Service::OnSocketWritable(int fd)
{
    cout << "OnSocketWritable: " << fd << endl;
}

void Service::OnSocketClose(int fd)
{
    cout << "OnSocketClose: " << fd << endl;
}