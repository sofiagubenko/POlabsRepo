#include <iostream>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <ctime>
#include <cstdlib> 
#include <chrono>

using namespace std;

using std::chrono::microseconds;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;

mutex mtx;
long long evenSum = 0;
int minEven = -1;

atomic<long long> AtomicEvenSum(0);
atomic<int> AtomicMinEven;

void processArrayPart(const vector<int>& arr, int start, int end, bool useMutex) 
{
    long long localEvenSum = 0;
    int localMinEven = -1;
    
    for (int i = start; i < end; i++) 
    {
        if (arr[i] % 2 == 0) 
        {
            localEvenSum += arr[i];
            if (localMinEven == -1 || arr[i] < localMinEven) 
            {
                localMinEven = arr[i];
            }
        }
    }

    if (useMutex)
    {
        lock_guard<mutex> lock(mtx);
        evenSum += localEvenSum;
        if (localMinEven != -1 && (minEven == -1 || localMinEven < minEven)) 
        {
                minEven = localMinEven;
        }
    } else 
    {
        evenSum += localEvenSum;
        if (localMinEven != -1 && (minEven == -1 || localMinEven < minEven)) 
        {
                minEven = localMinEven;
        }
    }
}

void processArrayPartAtomic(const vector<int>& arr, int start, int end) 
{
    long long localEvenSum = 0;

    for (int i = start; i < end; i++) 
    {
        if (arr[i] % 2 == 0) 
        {
            localEvenSum += arr[i];

            int currentMin = AtomicMinEven.load();
            while (currentMin == -1 || arr[i] < currentMin) 
            {
                if (AtomicMinEven.compare_exchange_weak(currentMin, arr[i])) 
                {
                    break;
                }
            }
        }
    }

    AtomicEvenSum.fetch_add(localEvenSum);
}

int main()
{
    vector<int> sizes = {100000, 1000000, 10000000, 100000000, 1000000000, 2000000000};
    vector<int> threadsList = {2, 4, 8, 16, 32, 64, 128};

    for (int i = 0; i < sizes.size(); i++) 
    {
        int size = sizes[i];

        cout << "\n===============================\n";
        cout << "Array size: " << size << endl;
        
        vector<int> arr(size);
        srand(static_cast<unsigned>(time(nullptr)));

        for(int j = 0; j < size; j++)
        {
            arr[j] = rand() % 10000;
        }
        
        cout << "\nSEQUENTIAL VERSION\n";

        evenSum = 0;
        minEven = -1;

        auto seq_begin = high_resolution_clock::now();

        processArrayPart(arr, 0, size, false);
    
        auto seq_end = high_resolution_clock::now();
        auto seq_time = duration_cast<microseconds>(seq_end - seq_begin);
        double seconds = seq_time.count() / 1000000.0;

        //
        cout << "Sum of even numbers: " << evenSum << endl;
        cout << "Smallest even number: " << minEven << endl;
        cout << "Time taken: " << seq_time.count() << " microseconds" << endl;
        cout << "Time taken: " << seconds << " seconds" << endl;
        //


        cout << "\nPARALLEL VERSION(blocking primitives)\n";

        for (int t = 0; t < threadsList.size(); t++)
        {
            int numThreads = threadsList[t];
            evenSum = 0;
            minEven = -1;

            vector<thread> threads;
            int chunkSize = size / numThreads;

            auto parallel_begin = high_resolution_clock::now();

            for (int k = 0; k < numThreads; k++)
            {
                int start = k * chunkSize;
                int end;
                if (k == numThreads - 1) 
                {
                    end = size;
                } else 
                {
                    end = (k + 1) * chunkSize;
                }
                threads.push_back(thread(processArrayPart, ref(arr), start, end, true));
            }
            
            for (auto& g : threads) 
            {
                if (g.joinable()) 
                {
                    g.join();
                } 
            }
            auto parallel_end = high_resolution_clock::now();
            auto parallel_time = duration_cast<microseconds>(parallel_end - parallel_begin);
            double seconds = parallel_time.count() / 1000000.0;

            // 
            cout << numThreads << " threads - time " << parallel_time.count() << " microseconds\n";
            cout << "Time taken: " << seconds << " seconds" << endl;
            //
        }
        
        cout << "\nPARALLEL VERSION(atomic CAS)\n";

        for (int t = 0; t < threadsList.size(); t++)
        {
            int numThreads = threadsList[t];
            AtomicEvenSum.store(0);
            AtomicMinEven.store(-1);

            vector<thread> atomicThreads;
            int chunkSize = size / numThreads;

            auto parallel_atomic_begin = high_resolution_clock::now();

            for (int k = 0; k < numThreads; k++) 
            {
                int start = k * chunkSize;
                int end;
            
                if (k == numThreads - 1) 
                {
                    end = size;
                } else 
                {
                    end = (k + 1) * chunkSize;
                }

                atomicThreads.push_back(thread(processArrayPartAtomic, ref(arr), start, end));
            }    

            for (auto& t : atomicThreads) 
            {
                if (t.joinable()) 
                {
                    t.join();
                }
            }

            auto parallel_atomic_end = high_resolution_clock::now();
            auto parallel_atomic_time = duration_cast<microseconds>(parallel_atomic_end - parallel_atomic_begin);
            double seconds = parallel_atomic_time.count() / 1000000.0;
            
            cout << numThreads << " threads - time " << parallel_atomic_time.count() <<  " microseconds\n";
            cout << "Time taken: " << seconds << " seconds" << endl;
        }
    }

    return 0;
}