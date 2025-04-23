#include <iostream>
#include <winsock2.h>
#include <vector>
#include <thread>

using namespace std;

struct MatrixHeader 
{
    uint32_t n;
    uint32_t threads;
    uint32_t type;
};

vector<vector<int>> unflatten(const vector<int>& flat, int n) 
{
    vector<vector<int>> mat(n, vector<int>(n));
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            mat[i][j] = flat[i * n + j];
    return mat;
}

vector<int> flatten(const vector<vector<int>>& mat) 
{
    int n = mat.size();
    vector<int> flat(n * n);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            flat[i * n + j] = mat[i][j];
    return flat;
}

bool recv_all(SOCKET s, char* buf, int len) 
{
    int total = 0;
    while (total < len) 
        total += recv(s, buf + total, len - total, 0);

    return true;
}

bool send_all(SOCKET s, const char* buf, int len) 
{
    int total = 0;
    while (total < len) 
        total += send(s, buf + total, len - total, 0);
    
    return true;
}

void compute(const vector<vector<int>>& A, const vector<vector<int>>& B, vector<vector<int>>& C, int start, int end) 
{
    int n = A.size();
    for (int i = start; i < end; i++)
        for (int j = 0; j < n; j++)
            C[i][j] = A[i][j] - B[i][j];
}

void handle_client(SOCKET client) 
{
    MatrixHeader h1, h2;
    recv_all(client, (char*)&h1, sizeof(h1));
    int n = ntohl(h1.n);
    int threads = ntohl(h1.threads);

    vector<int> flatA(n * n), flatB(n * n);
    recv_all(client, (char*)flatA.data(), n * n * sizeof(int));
    recv_all(client, (char*)&h2, sizeof(h2));
    recv_all(client, (char*)flatB.data(), n * n * sizeof(int));

    for (int i = 0; i < n * n; i++) 
    {
        flatA[i] = ntohl(flatA[i]);
        flatB[i] = ntohl(flatB[i]);
    }

    vector<vector<int>> A = unflatten(flatA, n);
    vector<vector<int>> B = unflatten(flatB, n);
    vector<vector<int>> C(n, vector<int>(n));

    cout << "Matrix A:\n";
    for (int i = 0; i < n; i++, cout << "\n")
        for (int j = 0; j < n; j++)
            cout << A[i][j] << "\t";

    cout << "Matrix B:\n";
    for (int i = 0; i < n; i++, cout << "\n")
        for (int j = 0; j < n; j++)
            cout << B[i][j] << "\t";

    int rows = (n + threads - 1) / threads;
    vector<thread> th;
    for (int i = 0; i < threads; i++) 
    {
        int start = i * rows;
        int end = min(start + rows, n);
        if (start < n)
            th.push_back(thread(compute, cref(A), cref(B), ref(C), start, end));
    }
    for (int i = 0; i < th.size(); i++) 
        th[i].join();

    cout << "Matrix C:\n";
    for (int i = 0; i < n; i++, cout << "\n")
        for (int j = 0; j < n; j++)
            cout << C[i][j] << "\t";

    vector<int> flatC = flatten(C);
    for (int i = 0; i < n * n; i++) 
        flatC[i] = htonl(flatC[i]);
    send_all(client, (char*)flatC.data(), n * n * sizeof(int));
    closesocket(client);
}

int main() 
{
    WSADATA w;
    WSAStartup(MAKEWORD(2, 2), &w);
    SOCKET serv = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(serv, (sockaddr*)&addr, sizeof(addr));
    listen(serv, SOMAXCONN);

    while (true) 
    {
        SOCKET client = accept(serv, 0, 0);
        thread(handle_client, client).detach();
    }

    closesocket(serv);
    WSACleanup();
    return 0;
}
