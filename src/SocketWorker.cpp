#include "SocketWorker.h"
#include <iostream>
#include <unistd.h>

void SocketWorker::Init()
{
    cout << "SocketWorker Init" << endl;
}

void SocketWorker::operator()()
{
    while(true)
    {
        cout << "SocketWorker working" << endl;
        usleep(1000);
    }
}