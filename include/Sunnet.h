#pragma once
#include <vector>
#include <unordered_map>
#include "Worker.h"
#include "Service.h"

class Worker;

class Sunnet
{
public:
	static Sunnet* inst;
	//服务列表
	unordered_map<uint32_t, shared_ptr<Service>> services;
	uint32_t maxId = 0;//最大ID
	pthread_rwlock_t servicesLock;//读写锁

public:
	Sunnet();
	void Start();
	void Wait();
	//增删服务
	uint32_t NewService(shared_ptr<string> type);
	void KillService(uint32_t id);
	//发送消息
	void Send(uint32_t told, shared_ptr<BaseMsg> msg);
	//全局队列操作
	shared_ptr<Service> PopGlobalQueue();
	void PushGlobalQueue(shared_ptr<Service> srv);
	//唤醒工作线程
    void CheckAndWeakUp();
	//让工作线程等待（仅工作线程调用）
    void WorkerWait();

	//test
	shared_ptr<BaseMsg> MakeMsg(uint32_t source, char* buff, int len);

private:
	int WORKER_NUM = 3;//工作线程数
	vector<Worker*> workers;
	vector<thread*> workerThreads;
	//全局消息队列，存放待处理的消息。由worker线程去除消息进行处理
	//为避免出现重复读取，使用自旋锁控制消息队列读取
	queue<shared_ptr<Service>> globalQueue;
	int globalLen = 0;//队列长度
	pthread_spinlock_t globalLock;//自旋锁
	//休眠和唤醒
    pthread_mutex_t sleepMtx;
    pthread_cond_t sleepCond;
    int sleepCount = 0;//休眠工作线程数

private:
	void StartWorker();
	shared_ptr<Service> GetService(uint32_t id);
};
