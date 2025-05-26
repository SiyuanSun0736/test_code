#include <stdio.h>      // 标准输入输出，用于 printf, perror, fgets, fprintf
#include <stdlib.h>     // 标准库，用于 exit, atoi
#include <string.h>     // 字符串操作，用于 memset, strlen, strcmp, strcspn
#include <unistd.h>     // POSIX 系统调用，用于 close
#include <arpa/inet.h>  // 互联网地址转换，用于 inet_pton
#include <sys/socket.h> // 套接字 API，用于 socket, sendto, recvfrom
#include <netinet/in.h> // 互联网地址结构，用于 sockaddr_in

#define BUFFER_SIZE 1024      // 接收和发送数据的缓冲区大小

int main(int argc, char *argv[]) {
    int sockfd;                 // 套接字文件描述符
    char buffer[BUFFER_SIZE];   // 数据缓冲区
    char input_buffer[BUFFER_SIZE]; // 用户输入缓冲区
    struct sockaddr_in servaddr; // 服务器地址结构
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

    // 2. 创建 UDP 套接字
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    printf("UDP Socket created successfully.\n");

    // 3. 准备服务器地址结构
    memset(&servaddr, 0, sizeof(servaddr)); // 清零结构体

    servaddr.sin_family = AF_INET;         // IPv4
    servaddr.sin_port = htons(port_num);   // 将端口号从主机字节序转换为网络字节序

    // 将 IP 地址从文本形式转换为二进制形式
    if (inet_pton(AF_INET, server_ip, &servaddr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Server address prepared: %s:%d\n", server_ip, port_num);

    // 4. 交互式发送和接收数据
    printf("\nEnter messages to send (type 'exit' to quit):\n");
    socklen_t len;
    ssize_t n;

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

        // 发送数据报到服务器
        sendto(sockfd, (const char *)input_buffer, strlen(input_buffer),
               MSG_CONFIRM, (const struct sockaddr *)&servaddr, sizeof(servaddr));
        printf("Sent: %s\n", input_buffer);

        // 清空缓冲区，准备接收数据
        memset(buffer, 0, BUFFER_SIZE);
        len = sizeof(servaddr); // 必须在每次调用 recvfrom 之前设置其大小

        // 从服务器接收数据报
        // 注意：对于 UDP 客户端，如果它只与一个服务器通信，
        // 也可以在创建套接字后调用一次 connect()，
        // 之后就可以使用 send() 和 recv() 而不是 sendto() 和 recvfrom()。
        // 但这里为了演示 UDP 的 sendto/recvfrom 特性，我们继续使用它们。
        n = recvfrom(sockfd, (char *)buffer, BUFFER_SIZE - 1,
                     MSG_WAITALL, (struct sockaddr *)&servaddr, &len);
        if (n > 0) {
            buffer[n] = '\0'; // 确保接收到的数据以 null 结尾
            printf("Received from server: %s\n", buffer);
        } else if (n == 0) {
            // UDP 是无连接的，recvfrom 返回 0 通常表示对端发送了空数据报，
            // 而不是像 TCP 那样表示连接关闭。
            printf("Received empty datagram.\n");
        } else {
            perror("recvfrom failed");
            // 接收失败，但 UDP 不会断开连接，可以继续尝试发送
        }
    }

    // 5. 关闭套接字
    close(sockfd);
    printf("UDP Socket closed.\n");

    return 0;
}
