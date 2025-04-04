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

    cout << "Sum of even numbers: " << evenSum << endl;
    if (minEven != -1) 
    {
        cout << "Smallest even number: " << minEven << endl;
    } else 
    {
        cout << "No even numbers found." << endl;
    }

    cout << "Time taken: " << seq_time.count() << " microseconds" << endl;

    return 0;
}