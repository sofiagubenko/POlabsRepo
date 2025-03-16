#include <iostream>
#include <chrono>
#include <thread>
#include <vector>

using namespace std;

int main() 
{
     srand(time(nullptr));  
     int n;
 
     cout << "Enter matrix size (n x n): ";
     cin >> n;
 
     vector<vector<int>> A(n, vector<int>(n));
     vector<vector<int>> B(n, vector<int>(n));
     vector<vector<int>> C(n, vector<int>(n));
 
     for (int i = 0; i < n; i++) {
         for (int j = 0; j < n; j++) {
             A[i][j] = rand() % 100;
             B[i][j] = rand() % 100;
         }
     }
 
     for (int i = 0; i < n; i++) {
         for (int j = 0; j < n; j++) {
             C[i][j] = A[i][j] - B[i][j];
         }
     }
 
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

     return 0;
}