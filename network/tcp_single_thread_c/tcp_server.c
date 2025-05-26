#include <stdio.h>      // 标准输入输出，用于 printf, perror
#include <stdlib.h>     // 标准库，用于 exit
#include <string.h>     // 字符串操作，用于 memset
#include <unistd.h>     // POSIX 系统调用，用于 close
#include <arpa/inet.h>  // 互联网地址转换，用于 inet_ntop
#include <sys/socket.h> // 套接字 API，用于 socket, bind, listen, accept, recv, send
#include <netinet/in.h> // 互联网地址结构，用于 sockaddr_in

#define PORT 8080           // 服务器监听的端口
#define BUFFER_SIZE 1024    // 接收和发送数据的缓冲区大小
#define BACKLOG 5           // listen 函数的连接队列最大长度

int main() {
    int server_fd;          // 服务器套接字文件描述符
    int client_socket;      // 客户端套接字文件描述符
    struct sockaddr_in address; // 服务器地址结构
    struct sockaddr_in client_address; // 客户端地址结构
    socklen_t addrlen = sizeof(address); // 地址结构长度
    socklen_t client_addrlen = sizeof(client_address); // 客户端地址结构长度
    char buffer[BUFFER_SIZE] = {0}; // 数据缓冲区
    char client_ip[INET_ADDRSTRLEN]; // 用于存储客户端IP地址的字符串

    // 1. 创建套接字
    // AF_INET: IPv4 协议族
    // SOCK_STREAM: TCP 流式套接字
    // 0: 默认协议 (TCP)
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    printf("Socket created successfully.\n");

    // 2. 设置套接字选项 (可选但推荐): 允许地址重用
    // 解决 "Address already in use" 错误，尤其是在服务器重启时
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Socket options set.\n");

    // 3. 准备服务器地址结构
    memset(&address, 0, sizeof(address)); // 清零结构体
    address.sin_family = AF_INET;         // IPv4
    address.sin_addr.s_addr = INADDR_ANY; // 监听所有可用的网络接口
    address.sin_port = htons(PORT);       // 将端口号从主机字节序转换为网络字节序

    // 4. 绑定套接字到指定地址和端口
    if (bind(server_fd, (struct sockaddr *)&address, addrlen) == -1) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Socket bound to port %d.\n", PORT);

    // 5. 监听传入连接
    // BACKLOG: 允许的最大待处理连接队列长度
    if (listen(server_fd, BACKLOG) == -1) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Server listening on port %d...\n", PORT);

    // 6. 接受连接循环 (单线程，一次处理一个客户端)
    while (1) {
        printf("\nWaiting for a new connection...\n");

        // 接受一个客户端连接
        // accept 函数会阻塞，直到有客户端连接
        if ((client_socket = accept(server_fd, (struct sockaddr *)&client_address, &client_addrlen)) == -1) {
            perror("accept failed");
            // 接受失败不退出，继续等待下一个连接
            continue;
        }

        // 打印客户端连接信息
        inet_ntop(AF_INET, &client_address.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("Connection accepted from %s:%d\n", client_ip, ntohs(client_address.sin_port));

        // 7. 处理客户端通信 (Echo 逻辑)
        ssize_t bytes_read;
        while ((bytes_read = recv(client_socket, buffer, BUFFER_SIZE - 1, 0)) > 0) {
            buffer[bytes_read] = '\0'; // 确保接收到的数据以 null 结尾
            printf("Received from client %s:%d: %s\n", client_ip, ntohs(client_address.sin_port), buffer);

            // 将接收到的数据原样发送回客户端
            if (send(client_socket, buffer, bytes_read, 0) == -1) {
                perror("send failed");
                break; // 发送失败，退出当前客户端处理循环
            }
            printf("Sent back to client %s:%d: %s\n", client_ip, ntohs(client_address.sin_port), buffer);
        }

        // 检查 recv 的返回值，判断客户端是断开连接还是发生错误
        if (bytes_read == 0) {
            printf("Client %s:%d disconnected.\n", client_ip, ntohs(client_address.sin_port));
        } else if (bytes_read == -1) {
            perror("recv failed");
        }

        // 8. 关闭客户端套接字
        close(client_socket);
        printf("Client socket %s:%d closed.\n", client_ip, ntohs(client_address.sin_port));
    }

    // 9. 关闭服务器套接字 (通常服务器会一直运行，所以这行代码可能永远不会执行)
    close(server_fd);
    printf("Server socket closed. Exiting.\n");

    return 0;
}
