#include <iostream>
#include <winsock2.h>
#include <vector>
#include <thread>
#include <chrono>
#include <string>
#include <map>

using namespace std;
using namespace chrono;

struct MatrixHeader 
{
    uint32_t n;
    uint32_t threads;
    uint32_t len;
};

struct ClientData 
{
    vector<vector<int>> A, B;
    vector<int> thread_config;
    vector<double> results;
    int current_thread = 0;
    bool is_processing = false;
};

map<SOCKET, ClientData> clients;

vector<vector<int>> unflatten(const vector<int>& flat, int n) 
{
    vector<vector<int>> mat(n, vector<int>(n));
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            mat[i][j] = flat[i * n + j];
    return mat;
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

void compute(const vector<vector<int>>& A, const vector<vector<int>>& B, vector<vector<int>>& C, int start, int end) 
{
    int n = A.size();
    for (int i = start; i < end; i++)
        for (int j = 0; j < n; j++)
            C[i][j] = A[i][j] - B[i][j];
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


void handle_client(SOCKET client) 
{
    ClientData& data = clients[client];
    char buffer[8192];

    try {
        while (true) 
        {
            string cmd;
            if (!recv_command(client, cmd))
                break; 

            cout << "[CLIENT #" << client << "] " << cmd << endl;

            if (cmd == "CONNECT") 
            {
                send_command(client, "CONNECTED");
            } else if (cmd == "SEND_DATA") 
            {
                MatrixHeader header;
                if (!recv_all(client, (char*)&header, sizeof(header))) 
                    throw runtime_error("header failed");

                int n = ntohl(header.n);
                int tcount = ntohl(header.threads);
                int len = ntohl(header.len);

                vector<int> config(tcount);
                if (!recv_all(client, (char*)config.data(), tcount * sizeof(int))) 
                    throw runtime_error("thread config failed");

                for (int i = 0; i < tcount; i++) 
                    config[i] = ntohl(config[i]);

                vector<int> flatA(n * n), flatB(n * n);
                if (!recv_all(client, (char*)flatA.data(), len)) 
                    throw runtime_error("matrix A failed");
                if (!recv_all(client, (char*)flatB.data(), len)) 
                    throw runtime_error("matrix B failed");

                for (int i = 0; i < n * n; i++) 
                {
                    flatA[i] = ntohl(flatA[i]);
                    flatB[i] = ntohl(flatB[i]);
                }

                data.A = unflatten(flatA, n);
                data.B = unflatten(flatB, n);
                data.thread_config = config;
                send_command(client, "DATA_RECEIVED");
            } else if (cmd == "START_SUBTRACTING") 
            {
                if (data.A.empty() || data.B.empty()) 
                    throw runtime_error("no matrices");

                data.is_processing = true;
                data.results.clear();

                send_command(client, "SUBTRACTING_STARTED");

                thread([&data, client]() 
                {
                    int n = data.A.size();
                    for (int i = 0; i < data.thread_config.size(); i++) 
                    {
                        data.current_thread = i;
                        int threads = data.thread_config[i];
                        vector<vector<int>> C(n, vector<int>(n));
                        int rows = (n + threads - 1) / threads;

                        auto begin = high_resolution_clock::now();

                        vector<thread> th;
                        for (int j = 0; j < threads; j++) 
                        {
                            int start = j * rows;
                            int end = min(start + rows, n);
                            if (start < n)
                                th.push_back(thread(compute, cref(data.A), cref(data.B), ref(C), start, end));
                        }

                        for (int j = 0; j < th.size(); j++) 
                            th[j].join();

                        auto end = high_resolution_clock::now();
                        double seconds = duration_cast<milliseconds>(end - begin).count() / 1000.0;
                        data.results.push_back(seconds);

                        string msg = "PROGRESS: " + to_string(threads) + " threads, time: " + to_string(seconds);
                        send_command(client, msg);
                    }

                    data.is_processing = false;
                    send_command(client, "SUBTRACTING_COMPLETE");
                }).detach();

            } else if (cmd == "GET_RESULT") 
            {
                string result = "RESULT:\nMatrix size: " + to_string(data.A.size()) + "x" + to_string(data.A.size());
                for (int i = 0; i < data.thread_config.size(); i++) 
                    result += "\n" + to_string(data.thread_config[i]) + " threads: " + to_string(data.results[i]) + " sec";
                
                send_command(client, result);
            }
        }
    } catch (const exception& e) 
    {
        cerr << "[SERVER ERROR] " << e.what() << endl;
        send_command(client, "ERROR");
    }

    closesocket(client);
    clients.erase(client);
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
    listen(serv, 5);

    cout << "Server running on port 12345\n";

    while (true) 
    {
        SOCKET client = accept(serv, 0, 0);
        cout << "[SERVER] New client connected: " << client << endl;
        thread(handle_client, client).detach();
    }

    WSACleanup();
    return 0;
}
