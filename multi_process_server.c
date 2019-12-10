#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <strings.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>
#include <sys/wait.h>
#include <signal.h>

#define SERVER_PORT 6666

void wait_child(int singal) {
    while(waitpid(0, NULL, WNOHANG));
    return ;
}

int main()
{
    int listen_file_descriptor, connect_file_descriptor;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_len;
    pid_t pid;
    char buf[BUFSIZ];

    listen_file_descriptor = socket(PF_INET, SOCK_STREAM, 0); // 建立套接字，指定使用TCP
    printf("Socket Created\n");

    bzero(&server_address, sizeof(server_address)); // 重置存储IP地址信息的结构体
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); // 为结构体指定IP地址、端口和协议族

    bind(listen_file_descriptor, (struct sockaddr *)&server_address, sizeof(server_address)); // IP地址和套接字绑定
    printf("Bind Success\n");

    listen(listen_file_descriptor, 128); // 开始监听是否有客户端发起连接
    printf("Start listening\n"); 

    while (1)
    {
        // 接受客户端发起的连接，分配连接文件描述符
        printf("Start connecting\n");
        connect_file_descriptor = accept(listen_file_descriptor, (struct sockaddr *)&client_address, &client_address_len);
        printf("Connection success\n");

        pid = fork();
        if (pid < 0)
        {
            perror("Fork Error");
            exit(1);
        }
        else if (pid == 0)
        {
            close(listen_file_descriptor);
            break;
        }
        else
        {
            printf("Created one process, pid is %d\n", pid);
            close(connect_file_descriptor);
            signal(SIGCHLD, wait_child);
        }
    }

    if (pid == 0)
    {
        while (1)
        {
            int n;
            n = read(connect_file_descriptor, buf, sizeof(buf));
            if (n == 0)
            {
                close(connect_file_descriptor);
                return 0;
            }
            else if (connect_file_descriptor == -1)
            {
                perror("Read Error");
                exit(1);
            }
            else
            {
                if (buf[0] == '1')
                {
                    struct timeval tv;
                    gettimeofday(&tv, NULL); //获取1970-1-1到现在的时间结果保存到tv中
                    uint64_t sec = tv.tv_sec;
                    uint64_t min = tv.tv_sec / 60;
                    struct tm cur_tm; //保存转换后的时间结果
                    localtime_r((time_t *)&sec, &cur_tm);
                    snprintf(buf, BUFSIZ, "process: %d: %d-%02d-%02d %02d:%02d:%02d", getpid(), cur_tm.tm_year + 1900, cur_tm.tm_mon + 1, cur_tm.tm_mday, cur_tm.tm_hour, cur_tm.tm_min, cur_tm.tm_sec);
                    printf("Calling: %s\n", buf); //打印当前时间
                }
                else
                {
                    for (int i = 0; i < n; i++)
                    {
                        buf[i] = toupper(buf[i]);
                    }
                }
                write(connect_file_descriptor, buf, n);
            }
        }
    }

    return 0;
}