#include <mutex>
#include <condition_variable>
#include <atomic>
#include <iostream>
#include <thread>
#include <unistd.h>

#define GCC_VERSION (__GNUC__ * 10000 \
                       + __GNUC_MINOR__ * 100 \
                       + __GNUC_PATCHLEVEL__)

std::mutex global_mutex;
std::condition_variable cv[2];
std::mutex slot_mutex[2];
std::atomic_int reader_count[2];
int read_slot{ 0 };
int write_slot{ 1 };

void produce(std::function<void(int index_to_use)>& callback)
{
        {
            std::lock_guard<std::mutex> guard(global_mutex);
            read_slot = write_slot;
            write_slot = (write_slot + 1) % 2;
        }
    
        std::unique_lock<std::mutex> lock(slot_mutex[write_slot]);
        cv[write_slot].wait(lock, []{ return reader_count[write_slot] == 0;} );

        callback(write_slot);
}

void consume(std::function<void(int index_to_use)>& callback)
{
    int read_slot_copy;
    
    while (1)
    {
        {
            std::lock_guard<std::mutex> guard(global_mutex);
            read_slot_copy = read_slot;
        }
        
        std::unique_lock<std::mutex> lock(slot_mutex[read_slot_copy], std::defer_lock);
        bool slot_acquired = lock.try_lock();
        if (slot_acquired)
        {
            ++reader_count[read_slot_copy];
            break;
        }
        else
        {
#if GCC_VERSION > 40603
            std::this_thread::yield();
#else
            usleep(10);
#endif            
        }
    }

    callback(read_slot_copy);
    
    --reader_count[read_slot_copy];
    cv[read_slot_copy].notify_one();  // Too many notifications
}