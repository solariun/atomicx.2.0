#include "atomicx.h"

#ifndef ARDUINO
#include <iostream>
#include <memory>
#else
#include <Arduino.h>
#endif

#include <stddef.h>

#include <stdio.h>
#include <unistd.h>

#include <string.h>
#include <stdint.h>
#include <setjmp.h>

#include <stdlib.h>

namespace atomicx {

    Context ctx;

    // Context Class Implementation
    
    int Context::start()
    {
        m_running = true;

        while(m_running)
        {
            if (threadCount == 0)
            {
                m_running = false;
                break;
            }

            m_activeThread = m_activeThread == nullptr ? begin : m_activeThread->next;

            if (m_activeThread != nullptr)
            {
                uint8_t kernelPointer = 0xAA;
                m_activeThread->stack.kernelPointer = &kernelPointer;

                if (setjmp(m_activeThread->kernelRegs) == 0)
                {
                    if (m_activeThread->threadState == state::READY)
                    {
                        m_activeThread->threadState = state::RUNNING;
                        m_activeThread->run();
                        m_activeThread->threadState = state::STOPPED;
                    } else {
                        longjmp(m_activeThread->userRegs, 1);
                    }
                }
            }
        }

        return 0;
    }

    void Context::AddThread(Thread* thread)
    {
        if (begin == nullptr)
        {
            begin = thread;
            last = thread;
        } else {
            last->next = thread;
            thread->prev = last;
            last = thread;
        }
        threadCount++;
    }

    void Context::RemoveThread(Thread* thread)
    {
        if (thread == begin)
        {
            begin = thread->next;
        }
        else if (thread == last)
        {
            last = thread->prev;
        }
        else {
            if (thread->prev != nullptr)
                thread->prev->next = thread->next;
            if (thread->next != nullptr)
                thread->next->prev = thread->prev;
        }
        threadCount--;
    }

    bool Context::yield()
    {
        //Create stack protection
        //By backing up data in the stack
        //to be used after the stack is re-written
        uint8_t stackPointer = 0xBB;
        //Context &kernelCtx = m_kernelCtx; //backup;
        
        // Calculate stack size and store are stack.size
        m_activeThread->stack.userPointer = &stackPointer;
        m_activeThread->stack.size = (size_t)(m_activeThread->stack.kernelPointer - m_activeThread->stack.userPointer);

        if (m_activeThread->stack.size > m_activeThread->stack.maxSize)
        {
            // Call the user defined StackOverflow function
            m_activeThread->StackOverflow();
            return false;
        }

        if (setjmp(m_activeThread->userRegs) == 0)
        {            
            memcpy(m_activeThread->stack.vmemory, m_activeThread->stack.userPointer, m_activeThread->stack.size);
            m_activeThread->threadState = state::SLEEPING;
            longjmp(m_activeThread->kernelRegs, 1);
        }

        {
            std::cout << "Thread " << m_activeThread << " is yielding. CTX:" << &ctx << "Stack:" << (size_t*)m_activeThread->stack.userPointer << std::endl;
            memcpy(m_activeThread->stack.userPointer, m_activeThread->stack.vmemory, m_activeThread->stack.size);
            m_activeThread->threadState = state::READY;
        }

        return true;
    }

    // Thread Class Implementation

    bool Thread::yield()
    {
        return _ctx.yield();
    }

}; // namespace atomicx 
