#include <stdio.h>      // 标准输入输出，用于 printf, perror, fgets
#include <stdlib.h>     // 标准库，用于 exit
#include <string.h>     // 字符串操作，用于 memset, strlen, strcmp, strcspn
#include <unistd.h>     // POSIX 系统调用，用于 close
#include <arpa/inet.h>  // 互联网地址转换，用于 inet_pton
#include <sys/socket.h> // 套接字 API，用于 socket, sendto, recvfrom
#include <netinet/in.h> // 互联网地址结构，用于 sockaddr_in

// 硬编码服务器的 IP 地址和端口号
#define SERVER_IP "127.0.0.1" // 服务器的 IP 地址 (这里是本机回环地址)
#define PORT 8080             // 服务器监听的端口 (与服务器端保持一致)
#define BUFFER_SIZE 1024      // 接收和发送数据的缓冲区大小

int main() {
    int sockfd;                 // 套接字文件描述符
    char buffer[BUFFER_SIZE];   // 数据缓冲区
    char input_buffer[BUFFER_SIZE]; // 用户输入缓冲区
    struct sockaddr_in servaddr; // 服务器地址结构

    // 1. 创建 UDP 套接字
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    printf("UDP Socket created successfully.\n");

    // 2. 准备服务器地址结构
    memset(&servaddr, 0, sizeof(servaddr)); // 清零结构体

    servaddr.sin_family = AF_INET;         // IPv4
    servaddr.sin_port = htons(PORT);       // 将端口号从主机字节序转换为网络字节序

    // 将 IP 地址从文本形式转换为二进制形式
    if (inet_pton(AF_INET, SERVER_IP, &servaddr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Server address prepared: %s:%d\n", SERVER_IP, PORT);

    // 3. 交互式发送和接收数据
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
        n = recvfrom(sockfd, (char *)buffer, BUFFER_SIZE - 1,
                     MSG_WAITALL, (struct sockaddr *)&servaddr, &len);
        if (n > 0) {
            buffer[n] = '\0'; // 确保接收到的数据以 null 结尾
            printf("Received from server: %s\n", buffer);
        } else if (n == 0) {
            printf("Received empty datagram.\n");
        } else {
            perror("recvfrom failed");
        }
    }

    // 4. 关闭套接字
    close(sockfd);
    printf("UDP Socket closed.\n");

    return 0;
}
