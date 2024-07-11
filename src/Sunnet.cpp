#include <iostream>
#include "Sunnet.h"
using namespace std;

Sunnet* Sunnet::inst;
Sunnet::Sunnet()
{
	inst = this;
}

void Sunnet::Start()
{
	cout << "Hello Sunnet" << endl;
	pthread_rwlock_init(&servicesLock, NULL);
	pthread_spin_init(&globalLock, PTHREAD_PROCESS_PRIVATE);
	pthread_cond_init(&sleepCond, NULL);
    pthread_mutex_init(&sleepMtx, NULL);
	StartWorker();
}

void Sunnet::Wait()
{
	if(workerThreads[0])
	{
		workerThreads[0]->join();
	}
}

uint32_t Sunnet::NewService(shared_ptr<string> type)
{
	auto srv = make_shared<Service>();//用智能指针代替构造函数，当不存在对该对象的弱引用时对象和空间会被释放
	srv->type = type;
	pthread_rwlock_wrlock(&servicesLock);
	{
		srv->id = maxId;
		maxId++;
		services.emplace(srv->id, srv);
	}
	pthread_rwlock_unlock(&servicesLock);
	srv->OnInit();
	return srv->id;
}

void Sunnet::StartWorker()
{
	for(int i = 0; i < WORKER_NUM; i++)
	{
		cout << "start worker thread:" << i << endl;
		Worker* worker = new Worker();
		worker->id = i;
		worker->eachNum = 2 << i;

		thread* wt = new thread(*worker);

		workers.push_back(worker);
		workerThreads.push_back(wt);
	}
}

shared_ptr<Service> Sunnet::GetService(uint32_t id)
{
	shared_ptr<Service> srv = NULL;
	pthread_rwlock_rdlock(&servicesLock);
	{
		unordered_map<uint32_t, shared_ptr<Service>>::iterator iter = services.find(id);
		if(iter != services.end())
		{
			srv = iter->second;
		}
	}
	pthread_rwlock_unlock(&servicesLock);
	return srv;
}

void Sunnet::KillService(uint32_t id)
{
	shared_ptr<Service> srv = GetService(id);
	if(!srv)
	{
		return;
	}

	srv->OnExit();
	srv->isExiting = true;

	pthread_rwlock_wrlock(&servicesLock);
	{
		services.erase(id);
	}
	pthread_rwlock_unlock(&servicesLock);
}

void Sunnet::Send(uint32_t srv_id, shared_ptr<BaseMsg> msg)
{
	//服务间消息发送
	shared_ptr<Service> toSrv = GetService(srv_id);
	if(!toSrv)
	{
		cout << "Send fail, toSrv not exist. srv_id: " << srv_id << endl;
		return;
	}

	toSrv->PushMsg(msg);
	//检查并放入全局队列
	bool hasPush = false;
	pthread_spin_lock(&toSrv->inGlobalLock);
	{
		if(!toSrv->inGlobal)
		{
			PushGlobalQueue(toSrv);
			toSrv->inGlobal = true;
			hasPush = true;
		}
	}
	pthread_spin_unlock(&toSrv->inGlobalLock);

	//唤起进程，不放在临界区里面
    if(hasPush) {
        CheckAndWeakUp();
    }
}

shared_ptr<Service> Sunnet::PopGlobalQueue()
{
	shared_ptr<Service> srv = NULL;
	pthread_spin_lock(&globalLock);
	{
		if(!globalQueue.empty())
		{
			srv = globalQueue.front();
			globalQueue.pop();
			globalLen--;
		}
	}
	pthread_spin_unlock(&globalLock);
	return srv;
}

void Sunnet::PushGlobalQueue(shared_ptr<Service> srv)
{
	pthread_spin_lock(&globalLock);
	{
		globalQueue.push(srv);
		globalLen++;
	}
	pthread_spin_unlock(&globalLock);
}

void Sunnet::CheckAndWeakUp()
{
	//检查并唤醒线程
	//这里没加锁读sleepCount，结果可能不准确。可能导致：
	//1.sleepCount小了->该唤醒时没唤醒
	//2.sleepCount大了->不该唤醒时唤醒了
	if(sleepCount == 0) {
        return;
    }
    if( WORKER_NUM - sleepCount <= globalLen ) {
        cout << "weakup" << endl; 
        pthread_cond_signal(&sleepCond);
    }
}

void Sunnet::WorkerWait()
{
	pthread_mutex_lock(&sleepMtx);
    sleepCount++;
	//pthread_cond_wait先给加了锁的sleepMtx解锁，然后进入休眠。在线程被唤醒，pthread_cond_wait返回前重新给sleepMtx加锁
    pthread_cond_wait(&sleepCond, &sleepMtx);
    sleepCount--;
    pthread_mutex_unlock(&sleepMtx);
}

////////////////////////////////////////////
//辅助函数：创建消息
shared_ptr<BaseMsg> Sunnet::MakeMsg(uint32_t source, char* buff, int len)
{
	auto msg = make_shared<ServiceMsg>();
	msg->type = BaseMsg::TYPE::SERVICE;
	msg->source = source;
	msg->buff = shared_ptr<char>(buff);
	msg->size = len;
	return msg;
}