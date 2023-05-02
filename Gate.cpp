#include <thread>
#include <condition_variable>
#include <chrono>
#include <iostream>

// This class implements a gate for a thread. It works as condition variable, 
// but if the Open method was called in another thread before the Close method, 
// then the Close method will not block the thread. Doesn't make sense when working in more than two threads
class Gate
{
private:
    std::mutex mtx, condVarMutex, lockMutex;
    std::unique_lock<std::mutex> lk = std::unique_lock<std::mutex>(lockMutex);
    std::condition_variable cv;
    bool IsActivated = true;
public:
    // Blocks the execution of the thread until the Open method is called. 
    // If the Run method was called before this method, then the thread is not blocked.
    // The Close and Open methods are synchronized with each other using a mutex
    void Close()
    {
        mtx.lock();
        if (IsActivated)
        {
            condVarMutex.lock();
            mtx.unlock();
            cv.wait(lk);
            condVarMutex.unlock();
            mtx.lock();
        }
        else
        {
            IsActivated = true;
        }
        mtx.unlock();
    }

    // Blocks the execution of the thread until the Open method is called or time's not up.
    // If the Run method was called before this method, then the thread is not blocked.
    // The Close and Open methods are synchronized with each other using a mutex
    template<class T>
    void CloseFor(std::chrono::duration<T> duration)
    {
        mtx.lock();
        if (IsActivated)
        {
            condVarMutex.lock();
            mtx.unlock();
            cv.wait_for(lk, duration);
            condVarMutex.unlock();
            mtx.lock();
        }
        else
        {
            IsActivated = true;
        }
        mtx.unlock();
    }

    // Blocks the execution of the thread until the Open method is called or the time has come for.
    // If the Run method was called before this method, then the thread is not blocked.
    // The Close and Open methods are synchronized with each other using a mutex
    template<class T>
    void CloseUntil(std::chrono::time_point<T> timePoint)
    {
        mtx.lock();
        if (IsActivated)
        {
            condVarMutex.lock();
            mtx.unlock();
            cv.wait_until(lk, timePoint);
            condVarMutex.unlock();
            mtx.lock();
        }
        else
        {
            IsActivated = true;
        }
        mtx.unlock();
    }

    // Causes the thread to continue executing after the Close method. 
    // If called before the Sleep method, then the Sleep method will not block the thread.
    // The Close and Open methods are synchronized with each other using a mutex
    void Open()
    {
        mtx.lock();
        if (condVarMutex.try_lock())
        {
            IsActivated = false;
            condVarMutex.unlock();
        }
        else
        {
            while (condVarMutex.try_lock() != true)
                cv.notify_all();

            condVarMutex.unlock();
        }
        mtx.unlock();
    }
};

// This class implements a gate for a thread. It works as condition variable, 
// but if the Open method was called in another thread before the Close method, 
// then the Close method will not block the thread. Doesn't make sense when working in more than two threads. 
// Supports opening and closing counter. You can call the Open method N times and then the 
// Close method will block the execution of the thread only for N call
class RecursiveGate
{
private:
    std::mutex mtx, condVarMutex, lockMutex;
    std::unique_lock<std::mutex> lk = std::unique_lock<std::mutex>(lockMutex);
    std::condition_variable cv;
    int ClosingAmount = 0;
public:
    // Blocks the execution of the thread until the Open method is called. 
    // If the Run method was called before this method, then the thread is not blocked.
    // The Close and Open methods are synchronized with each other using a mutex
    void Close()
    {
        mtx.lock();
        if (ClosingAmount <= 0)
        {
            condVarMutex.lock();
            mtx.unlock();
            cv.wait(lk);
            condVarMutex.unlock();
            mtx.lock();
        }
        --ClosingAmount;
        mtx.unlock();
    }

    // Blocks the execution of the thread until the Open method is called or time's not up.
    // If the Run method was called before this method, then the thread is not blocked.
    // The Close and Open methods are synchronized with each other using a mutex
    template<class T>
    void CloseFor(std::chrono::duration<T> duration)
    {
        mtx.lock();
        if (ClosingAmount <= 0)
        {
            condVarMutex.lock();
            mtx.unlock();
            cv.wait_for(lk, duration);
            condVarMutex.unlock();
            mtx.lock();
        }
        --ClosingAmount;
        mtx.unlock();
    }

    // Blocks the execution of the thread until the Open method is called or the time has come for.
    // If the Run method was called before this method, then the thread is not blocked.
    // The Close and Open methods are synchronized with each other using a mutex
    template<class T>
    void CloseUntil(std::chrono::time_point<T> timePoint)
    {
        mtx.lock();
        if (ClosingAmount <= 0)
        {
            condVarMutex.lock();
            mtx.unlock();
            cv.wait_until(lk, timePoint);
            condVarMutex.unlock();
            mtx.lock();
        }
        --ClosingAmount;
        mtx.unlock();
    }

    // Causes the thread to continue executing after the Close method. 
    // If called before the Sleep method, then the Sleep method will not block the thread.
    // The Close and Open methods are synchronized with each other using a mutex
    void Open()
    {
        mtx.lock();
        if (!condVarMutex.try_lock())
        {
            while (condVarMutex.try_lock() != true)
                cv.notify_all();

            condVarMutex.unlock();
        }
        else
            condVarMutex.unlock();

        ++ClosingAmount;
        mtx.unlock();
    }
};

int main()
{
    Gate g;

    g.Open();

    std::thread th1([&]()
        {
            g.Close();
        });   

    th1.join();

    std::thread th2([&]()
    {
        g.Close();
    });   
    g.Open();

    th2.join();

    std::cout << "Regular gates" << std::endl;

    RecursiveGate rg;
    
    std::thread th3([&]()
    {
        rg.Close();
        rg.Close();
    });
    rg.Open();
    rg.Open();

    th3.join();

    std::cout << "Recursive gates" << std::endl;
    
    g.CloseFor(std::chrono::seconds(4));

    std::cout << "Regular time gate for" << std::endl;

    rg.CloseFor(std::chrono::seconds(4));

    std::cout << "Recursive time gate for" << std::endl;
    
    auto now = std::chrono::system_clock::now();

    g.CloseUntil(now + std::chrono::seconds(5));
    std::cout << "Regular time gate until" << std::endl;
    
    rg.CloseUntil(now + std::chrono::seconds(5));
    std::cout << "Recursive time gate until" << std::endl;  
}