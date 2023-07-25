#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string>
#include <fstream>
#include <vector>
#include <fstream>
#include <unordered_set>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <netinet/in.h>
#include <arpa/inet.h>

class Server
{
    struct ThreadArgs
    {
        int client_sock;
        Server *server;
        ThreadArgs(int client_sock, Server *server)
        {
            this->client_sock = client_sock;
            this->server = server;
        }
    };
    size_t server_sock = -1; // 服务端的socket号
    u_short port = 1234;     // 服务器打开的端口号
    size_t client_sock = -1; // 客户端的socket号
    // 客户端的socket地址信息
    sockaddr_in client_name;
    socklen_t client_name_len = sizeof(client_name);
    pthread_t new_thread;                    // 线程号
    std::vector<ThreadArgs *> client_thread; // 存放定义的线程方法的参数
    std::unordered_set<size_t> client_set;   // 存放活跃的客户端socket号
    size_t max_buf_len = 1024;
    constexpr static char kServerName[] = "Server: XianYu's http/1.0.0\r\n";
    inline bool IsSpace(char) const;

public:
    Server();
    Server(u_short);
    ~Server();
    int StartUpServer(u_short *);
    static void *AcceptRequest(void *);
    void FinisServer(const char *) const;
    size_t GetLine(int sock, std::string &buf, size_t size) const;
    void Unimplemented(int) const;
    void NotFound(int) const;
    void ServeFile(int client, std::string &file_name);
    void Headers(int) const;
    void Cat(int client, std::fstream &fs) const;
    void SingleThread();
    void MultiThread();
    size_t QueryClientNumber() const;
};

#endif // SERVER_H
