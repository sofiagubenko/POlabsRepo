#include <iostream>
#include <winsock2.h>
#include <vector>
#include <ctime>
#include <thread>

using namespace std;

struct MatrixHeader 
{
    uint32_t n;
    uint32_t threads;
    uint32_t type;
};

vector<vector<int>> generate_matrix(int n) 
{
    vector<vector<int>> matrix(n, vector<int>(n));
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            matrix[i][j] = rand() % 100;
    return matrix;
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

vector<vector<int>> unflatten(const vector<int>& flat, int n) 
{
    vector<vector<int>> mat(n, vector<int>(n));
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            mat[i][j] = flat[i * n + j];
    return mat;
}

bool send_all(SOCKET s, const char* buf, int len) 
{
    int total = 0;
    while (total < len) 
        total += send(s, buf + total, len - total, 0);
    
    return true;
}

bool recv_all(SOCKET s, char* buf, int len) 
{
    int total = 0;
    while (total < len) 
        total += recv(s, buf + total, len - total, 0);
    
    return true;
}

bool send_matrix(SOCKET sock, const vector<int>& flat, int n, int threads, int type) 
{
    MatrixHeader header = { htonl(n), htonl(threads), htonl(type) };
    send_all(sock, (char*)&header, sizeof(header));

    vector<int> net_flat = flat;
    for (int i = 0; i < n * n; i++) 
        net_flat[i] = htonl(net_flat[i]);

    send_all(sock, (char*)net_flat.data(), n * n * sizeof(int));
    return true;
}

void print_matrix(const vector<vector<int>>& mat) 
{
    int n = mat.size();
    for (int i = 0; i < n; i++, cout << "\n")
        for (int j = 0; j < n; j++)
            cout << mat[i][j] << "\t";
}

int main() {
    srand((unsigned)time(0));

    WSADATA w;
    WSAStartup(MAKEWORD(2, 2), &w);
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serv = {};
    serv.sin_family = AF_INET;
    serv.sin_port = htons(12345);
    serv.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(sock, (sockaddr*)&serv, sizeof(serv));

    int n, threads;
    cout << "Enter matrix size n: ";
    cin >> n;
    cout << "Enter number of threads: ";
    cin >> threads;

    vector<vector<int>> A = generate_matrix(n);
    vector<vector<int>> B = generate_matrix(n);

    vector<int> A_flat = flatten(A);
    vector<int> B_flat = flatten(B);

    cout << "Matrix A (generated):\n";
    print_matrix(A);

    cout << "Matrix B (generated):\n";
    print_matrix(B);

    send_matrix(sock, A_flat, n, threads, 0);
    send_matrix(sock, B_flat, n, threads, 1);

    vector<int> C_flat(n * n);
    recv_all(sock, (char*)C_flat.data(), n * n * sizeof(int));
    for (int i = 0; i < n * n; i++) 
        C_flat[i] = ntohl(C_flat[i]);

    vector<vector<int>> C = unflatten(C_flat, n);

    cout << "Matrix C = A - B (from server):\n";
    print_matrix(C);

    closesocket(sock);
    WSACleanup();
    return 0;
}
