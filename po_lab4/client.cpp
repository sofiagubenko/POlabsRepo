#include <iostream>
#include <winsock2.h>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>

using namespace std;

struct MatrixHeader 
{
    uint32_t n;
    uint32_t threads;
    uint32_t len;
};

vector<int> flatten(const vector<vector<int>>& mat) 
{
    int n = mat.size();
    vector<int> flat(n * n);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            flat[i * n + j] = mat[i][j];
    return flat;
}

bool send_all(SOCKET s, const char* buf, int len) 
{
    int total = 0;
    while (total < len) 
    {
        int bytes = send(s, buf + total, len - total, 0);
        if (bytes <= 0) 
            return false;
        total += bytes;
    }
    return true;
}

bool recv_all(SOCKET s, char* buf, int len) 
{
    int total = 0;
    while (total < len) 
    {
        int bytes = recv(s, buf + total, len - total, 0);
        if (bytes <= 0) 
            return false; 
        total += bytes;
    }
    return true;
}

bool send_command(SOCKET sock, const string& cmd)
{
    uint32_t len = htonl(cmd.size());
    if (!send_all(sock, (char*)&len, sizeof(len)))
        return false;
    if (!send_all(sock, cmd.data(), cmd.size()))
        return false;
    return true;
}

bool recv_command(SOCKET sock, string& cmd)
{
    uint32_t len = 0;
    if (!recv_all(sock, (char*)&len, sizeof(len)))
        return false;
    len = ntohl(len);

    vector<char> buf(len);
    if (!recv_all(sock, buf.data(), len))
        return false;

    cmd.assign(buf.begin(), buf.end());
    return true;
}


void clientThread() 
{
    WSADATA w;
    WSAStartup(MAKEWORD(2, 2), &w);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serv = {};
    serv.sin_family = AF_INET;
    serv.sin_port = htons(12345);
    serv.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(sock, (sockaddr*)&serv, sizeof(serv));
    send_command(sock, "CONNECT");
    char buffer[4096];
    string server_response;
    recv_command(sock, server_response);
    cout << "[SERVER] " << server_response << endl;

    int n;
    cout << "Enter matrix size n: ";
    cin >> n;

    vector<vector<int>> A(n, vector<int>(n));
    vector<vector<int>> B(n, vector<int>(n));

    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) 
        {
            A[i][j] = rand() % 100;
            B[i][j] = rand() % 100;
        }

    vector<int> thread_config = { 1, 2, 4, 8, 16, 32, 64, 128 };

    send_command(sock, "SEND_DATA");

    MatrixHeader header;
    header.n = htonl(n);
    header.threads = htonl(thread_config.size());
    header.len = htonl(n * n * sizeof(int));

    send_all(sock, (char*)&header, sizeof(header));

    for (int i = 0; i < thread_config.size(); i++)
        thread_config[i] = htonl(thread_config[i]);
    send_all(sock, (char*)thread_config.data(), thread_config.size() * sizeof(int));

    vector<int> flatA = flatten(A);
    vector<int> flatB = flatten(B);

    for (int i = 0; i < n * n; i++) 
    {
        flatA[i] = htonl(flatA[i]);
        flatB[i] = htonl(flatB[i]);
    }

    send_all(sock, (char*)flatA.data(), n * n * sizeof(int));
    send_all(sock, (char*)flatB.data(), n * n * sizeof(int));

    recv_command(sock, server_response);
    cout << "[SERVER] " << server_response << endl;

    send_command(sock, "START_SUBTRACTING");
    
    atomic<bool> is_done(false);

    thread listener([&]() 
    {
        char buf[4096];
        while (!is_done) 
        {
            string msg;
            if (!recv_command(sock, msg))
                break;
            cout << "[SERVER] " << msg << endl;
            if (msg.find("SUBTRACTING_COMPLETE") != string::npos)
                is_done = true;
        }
    });

    while (!is_done) 
        this_thread::sleep_for(chrono::milliseconds(500));

    send_command(sock, "GET_RESULT");
    recv_command(sock, server_response);
    cout << "[SERVER] Final results:\n" << server_response << endl;


    listener.join();
    closesocket(sock);
    WSACleanup();
}

int main() 
{
    srand((unsigned)time(nullptr));
    thread(clientThread).join();
    return 0;
}
