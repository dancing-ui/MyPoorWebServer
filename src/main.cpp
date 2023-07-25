#include "server.h"

int main()
{
    setbuf(stdout, NULL); // 让printf立即输出，便于看到调试信息
    Server server; // 声明服务器类
    server.SingleThread();
    server.MultiThread();
    return 0;
}
