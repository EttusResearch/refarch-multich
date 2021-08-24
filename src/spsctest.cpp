#include <boost/thread/thread.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <iostream>

#include <boost/atomic.hpp>

int producer_count = 0;
boost::atomic_int consumer_count (0);

struct recvStruct{

    std::vector<std::vector<std::complex<short>>>  buffer;
    //std::vector<std::complex<short>*> buff_ptrs;
    size_t received_samples;

    
};

boost::lockfree::spsc_queue<int, boost::lockfree::capacity<1024> > spsc_queue;
boost::lockfree::spsc_queue<recvStruct, boost::lockfree::capacity<100>> recvQueue;

const int iterations = 10000000;

void producer(void)
{
    for (int i = 0; i != iterations; ++i) {
        int value = ++producer_count;
        while (!spsc_queue.push(value))
            ;
    }
}

boost::atomic<bool> done (false);

void consumer(void)
{
    int i;
    while (!done) {
        while (spsc_queue.pop(i))
            ++consumer_count;
    }

    while (spsc_queue.pop(i))
        ++consumer_count;
}

int main(int argc, char* argv[])
{
    using namespace std;
    cout << "boost::lockfree::queue is ";
    if (!spsc_queue.is_lock_free())
        cout << "not ";
    cout << "lockfree" << endl;

    boost::thread producer_thread(producer);
    boost::thread consumer_thread(consumer);

    producer_thread.join();
    done = true;
    consumer_thread.join();

    cout << "produced " << producer_count << " objects." << endl;
    cout << "consumed " << consumer_count << " objects." << endl;
}