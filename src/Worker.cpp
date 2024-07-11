#include <iostream>
#include <unistd.h>
#include "Worker.h"
#include "Service.h"
using namespace std;

//线程函数
void Worker::operator()()
{
    while(true)
    {
        shared_ptr<Service> srv = Sunnet::inst->PopGlobalQueue();
        if(!srv)
        {
            Sunnet::inst->WorkerWait();
        }
        else
        {
            srv->ProcessMsgs(eachNum);
            //判断服务是否还有未处理的消息，若有则放回队列，等待下次处理
            CheckAndPutGlobal(srv);
        }
    }
}

void Worker::CheckAndPutGlobal(shared_ptr<Service> srv)
{
    if(srv->isExiting)
    {
        return;
    }

    pthread_spin_lock(&srv->queueLock);
    {
        //服务还有未处理的消息，将服务重新放回全局队列
        if(!srv->msgQueue.empty())
        {
            //该服务此时处于“正在处理消息”的状态，其inGlobal必为true
            Sunnet::inst->PushGlobalQueue(srv);
        }
        //不在队列中，重新设置inGlobal的值
        else
        {
            srv->SetInGlobal(false);
        }
    }
    pthread_spin_unlock(&srv->queueLock);
}