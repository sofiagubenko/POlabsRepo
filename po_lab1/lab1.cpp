#include <iostream>
#include <chrono>
#include <thread>
#include <vector>

using namespace std;

using std::chrono::nanoseconds;
using std::chrono::microseconds;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;


int main() 
{
     srand(time(nullptr));  
     int n;
 
     cout << "Enter matrix size (n x n): ";
     cin >> n;

     auto creation_begin = high_resolution_clock::now();
     vector<vector<int>> A(n, vector<int>(n));
     vector<vector<int>> B(n, vector<int>(n));
     vector<vector<int>> C(n, vector<int>(n));
     auto creation_end = high_resolution_clock::now();
     auto creation_time = duration_cast<microseconds>(creation_end - creation_begin);
    
     auto init_begin = high_resolution_clock::now();
     for (int i = 0; i < n; i++) {
         for (int j = 0; j < n; j++) {
             A[i][j] = rand() % 100;
             B[i][j] = rand() % 100;
         }
     }
     auto init_end = high_resolution_clock::now();
     auto init_time = duration_cast<microseconds>(init_end - init_begin); 


     auto calc_begin = high_resolution_clock::now();
     for (int i = 0; i < n; i++) {
         for (int j = 0; j < n; j++) {
             C[i][j] = A[i][j] - B[i][j];
         }
     }
     auto calc_end = high_resolution_clock::now();
     auto calc_time = duration_cast<microseconds>(calc_end - calc_begin);
 
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

     cout << "\n----- Time Measurements -----\n";
     cout << "Matrix creation time: " << creation_time.count() << " microseconds" << endl; 
     cout << "Matrix initialization time: " << init_time.count() << " microseconds" << endl;
     cout << "Matrix subtraction time: " << calc_time.count() << " microseconds" << endl;

     auto total_time = creation_time + init_time + calc_time;
     cout << "Total proccesing time: " << total_time.count() << " microseconds (" << total_time.count() / 1000.0 << " milliseconds)" << endl;

     return 0;
}