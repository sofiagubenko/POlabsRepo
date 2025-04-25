#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>

#pragma comment(lib, "ws2_32.lib")
#define PORT 8080

using namespace std;

void sendResponse(SOCKET clientSocket, const string& response) 
{
    send(clientSocket, response.c_str(), static_cast<int>(response.size()), 0);
}

string readFile(const string& path) 
{
    ifstream file(path);
    if (!file.is_open()) 
        return "";
    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void handleRequest(SOCKET clientSocket) 
{
    char buffer[4096];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived <= 0) 
    {
        closesocket(clientSocket);
        return;
    }
    buffer[bytesReceived] = '\0';
    string request(buffer);

    cout << "Request:\n" << request << endl;

    string path = "/index.html"; 
    size_t pos = request.find("GET ");
    if (pos != string::npos) 
    {
        size_t start = pos + 4;
        size_t end = request.find(" ", start);
        path = request.substr(start, end - start);
        if (path == "/") 
            path = "/index.html";
    }

    string fileContent = readFile("." + path);
    string response;

    if (!fileContent.empty()) 
    {
        response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/html\r\n";
        response += "Content-Length: " + to_string(fileContent.size()) + "\r\n";
        response += "Connection: close\r\n\r\n";
        response += fileContent;
    } else 
    {
        string notFound = "<html><body><h1>404 Not Found</h1></body></html>";
        response = "HTTP/1.1 404 Not Found\r\n";
        response += "Content-Type: text/html\r\n";
        response += "Content-Length: " + to_string(notFound.size()) + "\r\n";
        response += "Connection: close\r\n\r\n";
        response += notFound;
    }

    sendResponse(clientSocket, response);
    closesocket(clientSocket);
}

int main() 
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) 
    {
        cerr << "Socket creation failed\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) 
    {
        cerr << "Bind failed\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, 10) == SOCKET_ERROR) 
    {
        cerr << "Listen failed\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    cout << "Server listening on port " << PORT << "...\n";

    while (true) 
    {
        sockaddr_in clientAddr;
        int clientSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (SOCKADDR*)&clientAddr, &clientSize);
        if (clientSocket == INVALID_SOCKET) 
        {
            cerr << "Accept failed\n";
            continue;
        }

        thread t(handleRequest, clientSocket);
        t.detach(); 
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}