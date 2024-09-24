#ifndef ARDUINO

#include <iostream>
#include <unistd.h>
#include "atomicx/atomicx.h"

static atomicx::Context ctx;

class contextThread : public atomicx::Thread
{
public:
    template<size_t stackSize>
    contextThread(size_t (&vmemory)[stackSize]) : atomicx::Thread(vmemory, stackSize)
    {
    }

    ~contextThread() override
    {
    }

protected:
    virtual bool run() override = 0;
    virtual bool StackOverflow() override = 0;
    
    atomicx::Context& getCtx() override
    {
        return ctx;
    }
};

class testThread : public contextThread
{
public:
    testThread(size_t start) : contextThread(vmemory), start(start)
    {
        id = counterId++;
        std::cout << "Thread " << id << " created" << std::endl;
    }

    ~testThread()
    {
    }
protected:
    bool run() override
    {
        std::cout << "Thread is running" << std::endl;
        size_t nCount=start;

        while(yield())
        {
            std::cout << id << ": Thread is running " << nCount++ << std::endl;
            usleep(500000); // Sleep for 1/2 second (by instance 1,000,000 microseconds)
        }

        std::cout << "Thread " << id << " stopped" << std::endl;

        return true;
    }

    bool StackOverflow() override
    {
        return false;
    }
private:
    size_t vmemory[1024];
    size_t start;
    static size_t counterId;
    size_t id{0};
};

// Initialize the static counter
size_t testThread::counterId = 0;

int main() 
{
    testThread test1(0);
    testThread test2(2000);
    testThread test3(3000);

    // Test the thread pool deletion
    {
        testThread test4(4000);
        testThread test5(5000);
        testThread test6(6000);
    }

    testThread test7(7000);
    testThread test8(8000);


    for(atomicx::Thread* thread = test1.begin(); thread != nullptr; thread = (*thread)++)
    {
        std::cout << "Thread " << thread << " is in the thread pool" << std::endl;
    }
    
    ctx.start();

    return 0;
}

#endif