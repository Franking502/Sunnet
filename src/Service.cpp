#include "Service.h"
#include "Sunnet.h"
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
    //开启监听
    Sunnet::inst->Sunnet::Listen(8002, id);
}

//收到消息时触发
void Service::OnMsg(shared_ptr<BaseMsg> msg)
{
    //SOCKET_ACCEPT
    if(msg->type == BaseMsg::TYPE::SOCKET_ACCEPT)
    {
        auto m = dynamic_pointer_cast<SocketAcceptMsg>(msg);
        cout << "new conn " << m->clientFd << endl;
    }

    //SOCKET_RW
    if(msg->type == BaseMsg::TYPE::SOCKET_RW)
    {
        auto m = dynamic_pointer_cast<SocketRWMsg>(msg);
        if(m->isRead)
        {
            char buff[512];
            int len = read(m->fd, &buff, 512);
            if(len > 0)
            {
                char writeBuff[4] = {'l', 'p', 'y', '\n'};
                write(m->fd, &writeBuff, 4);
            }
            else
            {
                cout << "close " << m->fd << strerror(errno) << endl;
                Sunnet::inst->CloseConn(m->fd);
            }
        }
    }

    /*if(msg->type == BaseMsg::TYPE::SERVICE)
    {
        auto m = dynamic_pointer_cast<ServiceMsg>(msg);
        cout << "[" << id << "] OnMsg " << m->buff << endl;

        auto msgRet = Sunnet::inst->MakeMsg(id, new char[9999999] {'p', 'i', 'n', 'g', '\0'}, 9999999);

        Sunnet::inst->Send(m->source, msgRet);
    }
    else
    {
        cout << "[" << id << "] OnMsg" << endl;
    }*/
}

//退出服务时触发
void Service::OnExit()
{
    cout << "[" << id << "] OnExit" << endl;
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