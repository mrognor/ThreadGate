#include <thread>
#include <condition_variable>

class Gate
{
private:
    std::mutex mtx, lockMutex;
    std::unique_lock<std::mutex> lk = std::unique_lock<std::mutex>(lockMutex);
    std::condition_variable cv;
    bool IsActivated = true;
public:
    // Blocks the execution of the thread until the Run method is called. 
    // If the Run method was called before this method, then the thread is not blocked.
    // The Close and Open methods are synchronized with each other using a mutex
    void Close()
    {
        mtx.lock();
        if (IsActivated)
            cv.wait(lk);
        else
            IsActivated = true;
        mtx.unlock();
    }

    // Causes the thread to continue executing after the Close method. 
    // If called before the Sleep method, then the Sleep method will not block the thread.
    // The Close and Open methods are synchronized with each other using a mutex
    void Open()
    {
        if (mtx.try_lock())
        {
            IsActivated = false;
            mtx.unlock();
        }
        else
        {
            cv.notify_one();
        }
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

    std::thread th2([&]()
        {
            g.Close();
        });

    g.Open();

    th1.join();
    th2.join();
}