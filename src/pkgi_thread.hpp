#pragma once

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

#include <psp2/kernel/threadmgr.h>

extern "C" {
#include "pkgi.h"
}

class ScopeProcessLock
{
public:
    ScopeProcessLock(const ScopeProcessLock&) = delete;
    ScopeProcessLock(ScopeProcessLock&&) = delete;
    ScopeProcessLock& operator=(const ScopeProcessLock&) = delete;
    ScopeProcessLock& operator=(ScopeProcessLock&&) = delete;

    ScopeProcessLock()
    {
        pkgi_lock_process();
    }
    ~ScopeProcessLock()
    {
        pkgi_unlock_process();
    }
};

class Mutex
{
public:
    Mutex(const Mutex&) = delete;
    Mutex(Mutex&&) = delete;
    Mutex& operator=(const Mutex&) = delete;
    Mutex& operator=(Mutex&&) = delete;

    Mutex(const std::string& name)
    {
        // I don't know what this 2 is
        const auto res =
                sceKernelCreateLwMutex(&_mutex, name.c_str(), 0, 0, nullptr);
        if (res < 0)
        {
            // TODO throw
            LOG("create mutex failed error=0x%08x", res);
        }
    }

    ~Mutex()
    {
        const auto res = sceKernelDeleteLwMutex(&_mutex);
        if (res < 0)
        {
            // TODO assert
            LOG("delete mutex failed error=0x%08x", res);
        }
    }

    void lock()
    {
        const auto res = sceKernelLockLwMutex(&_mutex, 1, nullptr);
        if (res < 0)
        {
            // TODO throw
            LOG("lock failed error=0x%08x", res);
        }
    }

    bool try_lock()
    {
        // I don't know how to handle errors
        // const auto res = sceKernelTryLockLwMutex(&_mutex);
        // if (res < 0)
        //{
        //    // TODO throw
        //    LOG("lock failed error=0x%08x", res);
        //}
        throw std::runtime_error("try_lock not implemented");
    }

    void unlock()
    {
        const auto res = sceKernelUnlockLwMutex(&_mutex, 1);
        if (res < 0)
        {
            // TODO throw
            LOG("unlock failed error=0x%08x", res);
        }
    }

private:
    SceKernelLwMutexWork _mutex;

    friend class Cond;
};

class Cond
{
public:
    Cond(const Cond&) = delete;
    Cond(Cond&&) = delete;
    Cond& operator=(const Cond&) = delete;
    Cond& operator=(Cond&&) = delete;

    Cond(const std::string& name) : _mutex(name + "_mutex")
    {
        const auto res = sceKernelCreateLwCond(
                &_cond, name.c_str(), 0, &_mutex._mutex, nullptr);
        if (res < 0)
        {
            // TODO throw
            LOG("create cond failed error=0x%08x", res);
        }
    }

    ~Cond()
    {
        const auto res = sceKernelDeleteLwCond(&_cond);
        if (res < 0)
        {
            // TODO assert
            LOG("delete cond failed error=0x%08x", res);
        }
    }

    void notify_one()
    {
        const auto res = sceKernelSignalLwCond(&_cond);
        if (res < 0)
        {
            // TODO throw
            LOG("cond signal failed error=0x%08x", res);
        }
    }

    void wait()
    {
        const auto res = sceKernelWaitLwCond(&_cond, nullptr);
        if (res < 0)
        {
            // TODO throw
            LOG("wait cond failed error=0x%08x", res);
        }
    }

    Mutex& get_mutex()
    {
        return _mutex;
    }

private:
    Mutex _mutex;
    SceKernelLwCondWork _cond;
};

class Thread
{
public:
    using EntryPoint = std::function<void()>;

    Thread(const Thread&) = delete;
    Thread(Thread&&) = delete;
    Thread& operator=(const Thread&) = delete;
    Thread& operator=(Thread&&) = delete;

    Thread(const std::string& name, EntryPoint entry)
    {
        _tid = sceKernelCreateThread(
                name.c_str(), &entry_point, 0xb0, 0x4000, 0, 0, nullptr);
        if (_tid < 0)
        {
            // TODO throw
            LOG("create thread failed error=0x%08x", _tid);
        }
        auto entryp = new EntryPoint(std::move(entry));
        const auto res = sceKernelStartThread(_tid, sizeof(entryp), &entryp);
        if (res < 0)
        {
            delete entryp;
            // TODO throw
            LOG("start thread failed error=0x%08x", res);
        }
    }

    ~Thread()
    {
        const auto res = sceKernelDeleteThread(_tid);
        if (res < 0)
        {
            // TODO assert
            LOG("thread delete failed error=0x%08x", res);
        }
    }

    void join()
    {
        int stat;
        const auto res = sceKernelWaitThreadEnd(_tid, &stat, nullptr);
        if (res < 0)
        {
            // TODO assert
            LOG("thread join failed error=0x%08x", res);
        }
    }

private:
    SceUID _tid;

    static int entry_point(SceSize, void* argp)
    {
        try
        {
            auto entryp = std::unique_ptr<EntryPoint>(
                    *static_cast<EntryPoint**>(argp));
            (*entryp)();
            LOG("thread successfully terminated");
        }
        catch (const std::exception& e)
        {
            LOG("got exception from thread: %s", e.what());
        }
        catch (...)
        {
            LOG("got unknown exception from thread");
        }
        return 0;
    }
};
