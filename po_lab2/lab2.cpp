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
long long ParEvenSum = 0;
int ParMinEven = -1;

atomic<long long> AtomicEvenSum(0);
atomic<int> AtomicMinEven;

void processArrayPart(const vector<int>& arr, int start, int end) 
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

    lock_guard<mutex> lock(mtx);
    ParEvenSum += localEvenSum;
    if (localMinEven != -1) 
    {
        if (ParMinEven == -1 || localMinEven < ParMinEven) 
        {
            ParMinEven = localMinEven;
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
    int size;
    cout << "Enter the size of the array: ";
    cin >> size;

    if(size <= 0)
    {
        cout << "Invalid array size" << endl;
        return 0;
    }

    vector<int> arr(size);
    srand(static_cast<unsigned>(time(nullptr)));

    for(int i = 0; i < size; i++)
    {
        arr[i] = rand() % 10000;
    }
    
    cout << "\n----- SEQUENTIAL VERSION -----\n";

    long long evenSum = 0;
    int minEven = -1;

    auto seq_begin = high_resolution_clock::now();

    for (int i = 0; i < size; i++) 
    {
        if (arr[i] % 2 == 0) 
        {
            evenSum += arr[i];
            if (minEven == -1 || arr[i] < minEven) 
            {
                minEven = arr[i];
            }
        }
    }
    auto seq_end = high_resolution_clock::now();
    auto seq_time = duration_cast<microseconds>(seq_end - seq_begin);

    //
    cout << "Sum of even numbers: " << evenSum << endl;
    if (minEven != -1) 
    {
        cout << "Smallest even number: " << minEven << endl;
    } 
    cout << "Time taken: " << seq_time.count() << " microseconds" << endl;
    //


    const int numThreads = 128;
    int chunkSize = size / numThreads;

    cout << "\n----- PARALLEL VERSION(blocking primitives) -----\n";

    ParEvenSum = 0;
    ParMinEven = -1;

    vector<thread> threads;

    auto parallel_begin = high_resolution_clock::now();

    for (int i = 0; i < numThreads; i++)
    {
        int start = i * chunkSize;
        int end;
        if (i == numThreads - 1) 
        {
            end = size;
        } else 
        {
            end = (i + 1) * chunkSize;
        }
        threads.push_back(thread(processArrayPart, ref(arr), start, end));
    }
    
    for (auto& t : threads) 
    {
        if (t.joinable()) 
        {
            t.join();
        } 
    }
    auto parallel_end = high_resolution_clock::now();
    auto parallel_time = duration_cast<microseconds>(parallel_end - parallel_begin);

    //
    cout << "Sum of even numbers: " << ParEvenSum << endl;
    if (ParMinEven != -1) 
    {
        cout << "Smallest even number: " << ParMinEven << endl;
    } 
    cout << "Time taken: " << parallel_time.count() << " microseconds" << endl;
    //
    
    
    cout << "\n----- PARALLEL VERSION(atomic CAS) -----\n";

    AtomicEvenSum.store(0);
    AtomicMinEven.store(-1);
    vector<thread> atomicThreads;

    auto parallel_atomic_begin = high_resolution_clock::now();

    for (int i = 0; i < numThreads; i++) 
    {
        int start = i * chunkSize;
        int end;
    
        if (i == numThreads - 1) 
        {
            end = size;
        } else 
        {
            end = (i + 1) * chunkSize;
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
    
    cout << "Sum of even numbers: " << AtomicEvenSum.load() << endl;
    if (AtomicMinEven.load() != -1)
    {
        cout << "Smallest even number: " << AtomicMinEven.load() << endl;
    }
    cout << "Time taken: " << parallel_atomic_time.count() << " microseconds" << endl;

    return 0;
}