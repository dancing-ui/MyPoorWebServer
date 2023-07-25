#include "server.h"

bool Server::IsSpace(char x) const
{
    return isspace((int)x);
}

Server::Server()
{
}

Server::Server(u_short port)
{
    this->port = port;
}

Server::~Server()
{
    for (auto &thread : client_thread)
    {
        delete thread;
    }
}
// 启动服务端
int Server::StartUpServer(u_short *port)
{
    int httpd = 0; // socket文件描述符
    int option;
    sockaddr_in name;
    // PF_INET表示使用TCP/IP协议族
    // SOCK_STREAM表示传输层使用TCP协议
    // 最后一个参数默认为0
    // 该函数的作用：创建一个socket，并返回一个socket文件描述符
    httpd = socket(PF_INET, SOCK_STREAM, 0);
    if (httpd == -1)
        FinisServer("Failed to create socket");
    socklen_t option_len;
    option_len = sizeof(option);
    option = 1;
    // 该函数的作用：若httpd处于TIME_WAIT状态，那么立即回收该socket
    setsockopt(httpd, SOL_SOCKET, SO_REUSEADDR, (void *)&option, option_len);
    // 下面是创建一个IPv4 socket地址
    memset(&name, 0, sizeof(name));
    name.sin_family = AF_INET; // AF_INET表示使用 IPv4 进行通信
    name.sin_port = htons(*port);
    name.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY表示接受本机上所有网卡接收到的、端口为port的报文
    // 将socket和IPv4地址绑定，客户端通过IPv4地址与服务器端的socket连接
    if (bind(httpd, (sockaddr *)&name, sizeof(name)) < 0)
        FinisServer("Failed to bind socket");
    // 如果端口为0，那么动态分配一个端口
    if (*port == 0)
    {
        socklen_t name_len = sizeof(name);
        // 该函数的作用：获取httpd绑定的socket地址，也就是IPv4地址，并把该地址赋给name
        if (getsockname(httpd, (sockaddr *)&name, &name_len) == -1)
            FinisServer("Failed to get local socket address");
        *port = ntohs(name.sin_port);
    }
    // 该函数的作用：创建大小为5的监听队列，将处于完全连接的状态的socket放入队列中，也就是说服务器最多支持5个客户端的连接
    if (listen(httpd, 5) < 0)
        FinisServer("Failure to listen to socket");
    return httpd;
}
// 处理监听到的HTTP请求
void *Server::AcceptRequest(void *from_client)
{
    // 初始化客户端的socket号和服务器类对象指针
    ThreadArgs *thread_args = static_cast<ThreadArgs *>(from_client);
    int client = thread_args->client_sock;
    Server *This = thread_args->server;

    std::string buf;
    std::string method;
    std::string url;
    std::string path;

    // 读取来自客户端的一行报文，一行最多max_buf_len个字符
    // 同时返回实际读取到的字符数
    size_t num_chars = This->GetLine(client, buf, This->max_buf_len);
    if(num_chars <= 0)
        This->FinisServer("The message content is empty");
    // buf数组的下标位置;
    size_t pos = 0;
    // 读取方法名
    while (pos < num_chars && !This->IsSpace(buf[pos]))
        method.push_back(buf[pos++]);
    // 目前只有GET，其它没实现
    if (method != "GET")
    {
        This->Unimplemented(client);
        return nullptr;
    }
    // 跳过空格
    while (pos < num_chars && This->IsSpace(buf[pos]))
        pos++;
    // 读取url
    while (pos < num_chars && !This->IsSpace(buf[pos]))
        url.push_back(buf[pos++]);
    // 文件路径以生成的exe文件位置作为起点，所以httpdocs目录与exe文件在同一目录下
    path = "httpdocs" + url;
    // 默认页面为index.html
    if (path[path.size() - 1] == '/')
        path += "index.html";
    struct stat st; // 用于判断文件是否存在，以及文件的权限
    // 没有找到路径path对应的文件
    if (stat(path.c_str(), &st) == -1)
    {
        // 把客户端的内容读完
        // 客户端最后的内容是"\n"，所以只有"\n"就退出循环
        while (num_chars > 0 && buf != "\n")
            num_chars = This->GetLine(client, buf, This->max_buf_len);
        This->NotFound(client); // 返回404，未发现文件
    }
    else
        This->ServeFile(client, path); // 返回200，响应文件内容
    close(client);                     // 关闭客户端的连接
    This->client_set.erase(client);    // 删除活跃的客户端socket号
    printf("connection close....client: %d \n", client);
    return nullptr;
}
// 打印服务器的错误信息
void Server::FinisServer(const char *kErrorInfomation) const
{
    fputs((std::string("ServerError: ") + kErrorInfomation).c_str(), stderr);
    fputc('\n', stderr);
    exit(1);
}
// 解析一行http报文
size_t Server::GetLine(int sock, std::string &buf, size_t size) const
{
    buf.clear();
    // \n表示buf字符串结束
    size_t sum = 0;
    char c = '\0';
    ssize_t readed = 0;
    while (sum < size && c != '\n')
    {
        readed = recv(sock, &c, 1, 0);
        if (readed > 0)
        {
            // 跳过 \r\n两个字符
            if (c == '\r')
            {
                readed = recv(sock, &c, 1, MSG_PEEK);
                if (readed > 0 && c == '\n')
                    recv(sock, &c, 1, 0);
                else
                    c = '\n';
            }
            buf.push_back(c);
            sum++;
        }
        else
            c = '\n';
    }
    return sum;
}
// 请求方法未实现
void Server::Unimplemented(int client) const
{
    std::string buf;
    buf = "HTTP/1.0 501 Method Not Implemented\r\n";
    send(client, buf.c_str(), buf.size(), 0);
    buf = kServerName;
    send(client, buf.c_str(), buf.size(), 0);
    buf = "Content-Type: text/html\r\n";
    send(client, buf.c_str(), buf.size(), 0);
    buf = "\r\n";
    send(client, buf.c_str(), buf.size(), 0);
    buf = "<HTML><HEAD><TITLE>Method Not Implemented\r\n";
    send(client, buf.c_str(), buf.size(), 0);
    buf = "</TITLE></HEAD>\r\n";
    send(client, buf.c_str(), buf.size(), 0);
    buf = "<BODY><P>HTTP request method not supported.\r\n";
    send(client, buf.c_str(), buf.size(), 0);
    buf = "</BODY></HTML>\r\n";
    send(client, buf.c_str(), buf.size(), 0);
}
// 未发现文件
void Server::NotFound(int client) const
{
    std::string buf;
    buf = "HTTP/1.0 404 NOT FOUND\r\n";
    send(client, buf.c_str(), buf.size(), 0);
    buf = kServerName;
    send(client, buf.c_str(), buf.size(), 0);
    buf = "Content-Type: text/html\r\n";
    send(client, buf.c_str(), buf.size(), 0);
    buf = "\r\n";
    send(client, buf.c_str(), buf.size(), 0);
    buf = "<HTML><TITLE>Not Found</TITLE>\r\n";
    send(client, buf.c_str(), buf.size(), 0);
    buf = "<BODY><P>The server could not fulfill\r\n";
    send(client, buf.c_str(), buf.size(), 0);
    buf = "your request because the resource specified\r\n";
    send(client, buf.c_str(), buf.size(), 0);
    buf = "is unavailable or nonexistent.\r\n";
    send(client, buf.c_str(), buf.size(), 0);
    buf = "</BODY></HTML>\r\n";
    send(client, buf.c_str(), buf.size(), 0);
}
// 根据文件名返回给客户端请求的文件内容
void Server::ServeFile(int client, std::string &file_name)
{
    // 把客户端内容读完
    int num_chars = 1;
    std::string buf;
    while (num_chars > 0 && buf != "\n")
        num_chars = GetLine(client, buf, max_buf_len);
    // 根据文件路径打开文件
    std::fstream fs;
    fs.open(file_name, std::ios_base::in);
    if (fs.is_open() == false)
        NotFound(client);
    else
    {
        Headers(client);
        Cat(client, fs);
    }
    fs.close();
}
// 构造响应包首部
void Server::Headers(int client) const
{
    std::string buf;
    buf = "HTTP/1.0 200 OK\r\n";
    send(client, buf.c_str(), buf.size(), 0);
    buf = kServerName;
    send(client, buf.c_str(), buf.size(), 0);
    buf = "Content-Type: text/html\r\n";
    send(client, buf.c_str(), buf.size(), 0);
    buf = "\r\n";
    send(client, buf.c_str(), buf.size(), 0);
}
// 打开文件，并发送文件内容
void Server::Cat(int client, std::fstream &fs) const
{
    std::string buf;
    while (getline(fs, buf))
        send(client, buf.c_str(), buf.size(), 0);
}
// 单个线程处理客户端连接
void Server::SingleThread()
{
    server_sock = StartUpServer(&port); // 根据端口初始化服务器
    printf("http server_sock is %ld\n", server_sock);
    printf("http running on port %d\n", port);
    client_sock = accept(server_sock,
                         (sockaddr *)&client_name,
                         &client_name_len);
    client_set.insert(client_sock);
    printf("New connection...... ip:%s, port:%d\n",
           inet_ntoa(client_name.sin_addr),
           ntohs(client_name.sin_port));
    if (client_sock == -1)
    {
        perror("Failure to accept socket");
        exit(1);
    }
    ThreadArgs *thread_args = new ThreadArgs(client_sock, this);
    client_thread.push_back(thread_args);
    AcceptRequest((void *)thread_args);
    close(server_sock);
}
// 多线程处理客户端连接
void Server::MultiThread()
{
    server_sock = StartUpServer(&port);
    printf("http server_sock is %ld\n", server_sock);
    printf("http running on port %d\n", port);
    while (1)
    {
        // 服务器从listen监听队列中接受一个客户端连接，把客户端的socket地址填入client_name，并返回客户端的socket号
        client_sock = accept(server_sock,
                             (sockaddr *)&client_name,
                             &client_name_len);
        client_set.insert(client_sock); // 添加活跃的客户端socket号
        // inet_ntoa用网络字节序整数表示的IPv4地址转化为用点分十进制字符串表示的IPv4地址
        // ntohs把网络字节序的数字转为主机序的数字
        printf("New connection...... ip:%s, port:%d\n",
               inet_ntoa(client_name.sin_addr),
               ntohs(client_name.sin_port));
        if (client_sock == -1)
        {
            perror("Failure to accept socket");
            exit(1);
        }
        // 由于pthread_create要求传入静态方法AcceptRequest，而类中的静态方法调用不了非静态方法，所以我用结构体把指针对象一起传入，
        // 这样就实现了在静态方法中调用非静态方法
        // 又因为pthread_create不允许直接构造ThreadArgs，且为了保证thread_args的生命周期足够长，
        // 所以，我只有申请空间，并把所有的thread_args都放到一个容器中，最后释放空间
        ThreadArgs *thread_args = new ThreadArgs(client_sock, this);
        client_thread.push_back(thread_args);
        if (pthread_create(&new_thread, nullptr,
                           AcceptRequest,
                           (void *)thread_args) != 0)
        {
            perror("Failed to create thread");
            exit(1);
        }
    }
    close(server_sock); // 根据socket号，关闭服务器的socket文件
}
// 查询活跃的客户端数量
size_t Server::QueryClientNumber() const
{
    return client_set.size();
}
