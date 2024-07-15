#pragma once
#include <queue>
#include <thread>
#include "Msg.h"

using namespace std;

class Service
{
public:
    uint32_t id;//服务编号
    shared_ptr<string> type;
    bool isExiting = false;
    queue<shared_ptr<BaseMsg>> msgQueue;
    pthread_spinlock_t queueLock;//自旋锁：当一个线程获取锁时，如果锁已被其他线程获取，则线程将循环等待。使用锁前必须初始化，使用完后必须销毁
    //标记服务是否在全局队列中，true表示在队列中或正在处理
    bool inGlobal = false;
    pthread_spinlock_t inGlobalLock;

public:
    Service();
    ~Service();

    //callback
    void OnInit();
    void OnMsg(shared_ptr<BaseMsg> msg);
    void OnExit();

    //insert message
    void PushMsg(shared_ptr<BaseMsg> msg);

    //excute message
    bool ProcessMsg();
    void ProcessMsgs(int max);

    //线程安全地设置inGlobal
    void SetInGlobal(bool isIn);

private:
    shared_ptr<BaseMsg> PopMsg();
    void OnServiceMsg(shared_ptr<ServiceMsg> msg);
    void OnAcceptMsg(shared_ptr<SocketAcceptMsg> msg);
    void OnRWMsg(shared_ptr<SocketRWMsg> msg);
    void OnSocketData(int fd, const char* buff, int len);
    void OnSocketWritable(int fd);
    void OnSocketClose(int fd);
};