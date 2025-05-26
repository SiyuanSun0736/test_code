#include <stdio.h>      // 标准输入输出，用于 printf, perror, fgets, fprintf
#include <stdlib.h>     // 标准库，用于 exit, atoi
#include <string.h>     // 字符串操作，用于 memset, strlen, strcmp, strcspn
#include <unistd.h>     // POSIX 系统调用，用于 close
#include <arpa/inet.h>  // 互联网地址转换，用于 inet_pton
#include <sys/socket.h> // 套接字 API，用于 socket, connect, send, recv
#include <netinet/in.h> // 互联网地址结构，用于 sockaddr_in

#define BUFFER_SIZE 1024      // 接收和发送数据的缓冲区大小

int main(int argc, char *argv[]) {
    int sock = 0;               // 客户端套接字文件描述符
    struct sockaddr_in serv_addr; // 服务器地址结构
    char buffer[BUFFER_SIZE] = {0}; // 数据缓冲区
    char input_buffer[BUFFER_SIZE]; // 用户输入缓冲区
    const char *server_ip;      // 服务器 IP 地址字符串
    int port_num;               // 服务器端口号

    // 1. 检查命令行参数数量
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <Server_IP_Address> <Port_Number>\n", argv[0]);
        fprintf(stderr, "Example: %s 127.0.0.1 8080\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // 从命令行参数获取 IP 地址和端口
    server_ip = argv[1];
    port_num = atoi(argv[2]); // 将字符串端口号转换为整数

    // 简单的端口号有效性检查
    if (port_num <= 0 || port_num > 65535) {
        fprintf(stderr, "Invalid port number: %s. Port must be between 1 and 65535.\n", argv[2]);
        exit(EXIT_FAILURE);
    }

    // 2. 创建套接字
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }
    printf("Socket created successfully.\n");

    // 3. 准备服务器地址结构
    memset(&serv_addr, 0, sizeof(serv_addr)); // 清零结构体

    serv_addr.sin_family = AF_INET;         // IPv4
    serv_addr.sin_port = htons(port_num);   // 将端口号从主机字节序转换为网络字节序

    // 将 IP 地址从文本形式转换为二进制形式
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(sock);
        exit(EXIT_FAILURE);
    }
    printf("Server address prepared: %s:%d\n", server_ip, port_num);

    // 4. 连接到服务器
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        close(sock);
        exit(EXIT_FAILURE);
    }
    printf("Connected to server %s:%d\n", server_ip, port_num);

    // 5. 交互式发送和接收数据
    printf("\nEnter messages to send (type 'exit' to quit):\n");
    while (1) {
        printf("> ");
        // 从标准输入读取一行
        if (fgets(input_buffer, BUFFER_SIZE, stdin) == NULL) {
            printf("Error reading input or EOF.\n");
            break;
        }

        // 移除 fgets 读取的末尾换行符
        input_buffer[strcspn(input_buffer, "\n")] = 0;

        // 检查是否输入 'exit'
        if (strcmp(input_buffer, "exit") == 0) {
            printf("Exiting client.\n");
            break;
        }

        // 发送数据到服务器
        if (send(sock, input_buffer, strlen(input_buffer), 0) == -1) {
            perror("send failed");
            break;
        }
        printf("Sent: %s\n", input_buffer);

        // 清空缓冲区，准备接收数据
        memset(buffer, 0, BUFFER_SIZE);

        // 从服务器接收数据
        ssize_t bytes_read = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0'; // 确保接收到的数据以 null 结尾
            printf("Received from server: %s\n", buffer);
        } else if (bytes_read == 0) {
            printf("Server disconnected.\n");
            break; // 服务器断开连接
        } else {
            perror("recv failed");
            break; // 接收失败
        }
    }

    // 6. 关闭套接字
    close(sock);
    printf("Socket closed.\n");

    return 0;
}
