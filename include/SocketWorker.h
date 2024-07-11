#pragma once
using namespace std;

class SocketWorker
{
public:
    void Init();//初始化函数，系统启动时调用
    void operator()();//线程函数
};