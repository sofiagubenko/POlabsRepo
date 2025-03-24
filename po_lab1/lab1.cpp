#include <iostream>
#include <chrono>
#include <thread>
#include <vector>

using namespace std;

using std::chrono::microseconds;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;


void subtract_rows(const vector<vector<int>>& A, const vector<vector<int>>& B, vector<vector<int>>& C, int start_row, int num_rows, int cols)
{
    for(int i = start_row; i < start_row + num_rows && i < A.size(); i++)
    {
        for(int j = 0; j < cols; j++)
        {
            C[i][j] = A[i][j] - B[i][j];
        }
    }
}


int main() 
{
    const int NUM_THREADS = 128;

     srand(time(nullptr));  
     int n;
 
     cout << "Enter matrix size (n x n): ";
     cin >> n;

     vector<vector<int>> A(n, vector<int>(n));
     vector<vector<int>> B(n, vector<int>(n));
     vector<vector<int>> C(n, vector<int>(n));

     for (int i = 0; i < n; i++) 
     {
        for (int j = 0; j < n; j++) 
        {
            A[i][j] = rand() % 100;
            B[i][j] = rand() % 100;
        }
    }

    for (int i = 0; i < n; i++) 
    {
       for (int j = 0; j < n; j++) 
       {
           C[i][j] = 0;
       }
   }

    cout << "\n----- SEQUENTIAL VERSION -----\n";

    auto seq_begin = high_resolution_clock::now();

    for (int i = 0; i < n; i++) 
    {
        for (int j = 0; j < n; j++) 
        {
            C[i][j] = A[i][j] - B[i][j];
        }
    }

    auto seq_end = high_resolution_clock::now();
    auto seq_time = duration_cast<microseconds>(seq_end - seq_begin);

    cout << "Sequential Time Measurements\n";
    cout << "Matrix subtraction time: " << seq_time.count() << " microseconds (" 
         << seq_time.count() / 1000.0 << " milliseconds)" << endl;


    
    cout << "\n----- PARALLEL VERSION -----\n";
    
     vector<thread> calc_threads;
     int rows_per_thread = (n + NUM_THREADS - 1) / NUM_THREADS;

     auto par_begin = high_resolution_clock::now();

     for (int i = 0; i < NUM_THREADS; i++) 
     {
        int start_row = i * rows_per_thread;
        calc_threads.push_back(thread(subtract_rows, ref(A), ref(B), ref(C), start_row, rows_per_thread, n));
     }

     for(auto& t : calc_threads)
     {
        if(t.joinable())
        {
            t.join();
        }
     }

     auto par_end = high_resolution_clock::now();
     auto par_time = duration_cast<microseconds>(par_end - par_begin);


     cout << "Number of threads used: " << NUM_THREADS;
     cout << "\nParallel Time Measurements\n";
     cout << "Matrix subtraction time: " << par_time.count() << " microseconds (" 
         << par_time.count() / 1000.0 << " milliseconds)" << endl;

    
     return 0;
}