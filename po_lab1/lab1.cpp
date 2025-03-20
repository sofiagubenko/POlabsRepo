#include <iostream>
#include <chrono>
#include <thread>
#include <vector>

using namespace std;

using std::chrono::nanoseconds;
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

void initialize_rows(vector<vector<int>>& A, vector<vector<int>>& B, int start_row, int num_rows, int cols)
{
    for(int i = start_row; i < start_row + num_rows && i < A.size(); i++)
    {
        for(int j = 0; j < cols; j++)
        {
            A[i][j] = rand() % 100;
            B[i][j] = rand() % 100;
        }
    }
}

int main() 
{
    const int NUM_THREADS = 16;

     srand(time(nullptr));  
     int n;
 
     cout << "Enter matrix size (n x n): ";
     cin >> n;

     ///
     auto creation_begin = high_resolution_clock::now();
     vector<vector<int>> A(n, vector<int>(n));
     vector<vector<int>> B(n, vector<int>(n));
     vector<vector<int>> C(n, vector<int>(n));
     auto creation_end = high_resolution_clock::now();
     auto creation_time = duration_cast<microseconds>(creation_end - creation_begin);
    ///

    ///
     vector<thread> init_threads;
     int rows_per_thread = (n + NUM_THREADS - 1) / NUM_THREADS;

     auto init_begin = high_resolution_clock::now();

     for (int i = 0; i < NUM_THREADS; i++) 
     {
        int start_row = i * rows_per_thread;
        init_threads.push_back(thread(initialize_rows, ref(A), ref(B), start_row, rows_per_thread, n));
     }

     for(auto& t : init_threads)
     {
        if(t.joinable())
        {
            t.join();
        }
     }

     auto init_end = high_resolution_clock::now();
     auto init_time = duration_cast<microseconds>(init_end - init_begin); 
    ///

    ///
     vector<thread> calc_threads;

     auto calc_begin = high_resolution_clock::now();

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

     auto calc_end = high_resolution_clock::now();
     auto calc_time = duration_cast<microseconds>(calc_end - calc_begin);
    ///


     cout << "Matrix A:\n";
     for (int i = 0; i < n; i++) {
         for (int j = 0; j < n; j++) {
             cout << A[i][j] << "\t";
         }
         cout << endl;
     }
 
     cout << "\nMatrix B:\n";
     for (int i = 0; i < n; i++) {
         for (int j = 0; j < n; j++) {
             cout << B[i][j] << "\t";
         }
         cout << endl;
     }
 
     cout << "\nMatrix C (A - B):\n";
     for (int i = 0; i < n; i++) {
         for (int j = 0; j < n; j++) {
             cout << C[i][j] << "\t";
         }
         cout << endl;
     }

     
     cout << endl << "Number of threads used: " << NUM_THREADS << endl;

     cout << "\n----- Time Measurements -----\n";
     cout << "Matrix creation time: " << creation_time.count() << " microseconds" << endl; 
     cout << "Matrix initialization time: " << init_time.count() << " microseconds" << endl;
     cout << "Matrix subtraction time: " << calc_time.count() << " microseconds" << endl;

     auto total_time = creation_time + init_time + calc_time;
     cout << "Total proccesing time: " << total_time.count() << " microseconds (" << total_time.count() / 1000.0 << " milliseconds)" << endl;

     return 0;
}