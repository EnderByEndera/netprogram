#include <stdio.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <time.h>
#include <sys/errno.h>

#define MAXLINE 8192
#define SERVER_PORT 8000
#define OPEN_MAX 5000

int main()
{
    int i, listen_file_descriptor, connection_file_descriptor, socket_file_descriptor;
    int n, num = 0;
    ssize_t nready, epoll_file_descriptor, res;
    char buf[MAXLINE], str[INET_ADDRSTRLEN];
    socklen_t clilen;

    struct sockaddr_in client_address, server_address;
    struct epoll_event evt, evts[OPEN_MAX];

    listen_file_descriptor = socket(PF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listen_file_descriptor, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // 实现端口复用

    bzero(&server_address, sizeof(server_address));
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);

    while (bind(listen_file_descriptor, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
        ;

    listen(listen_file_descriptor, 20);

    epoll_file_descriptor = epoll_create(OPEN_MAX);
    if (epoll_file_descriptor == -1)
    {
        perror("epoll_create error");
    }
    evt.events = EPOLLIN;
    evt.data.fd = listen_file_descriptor;

    res = epoll_ctl(epoll_file_descriptor, EPOLL_CTL_ADD, listen_file_descriptor, &evt); // 将lfd及其对应的结构体设置到树上，epoll_file_descriptor可找到该树
    if (res == -1)
    {
        perror("epoll_ctrl error");
    }

    while (1)
    {
        /* epoll为server阻塞监听事件，ep为struct epoll_event类型数组，OPEN_MAX为数组容量，-1表示永久阻塞 */
        nready = epoll_wait(epoll_file_descriptor, evts, OPEN_MAX, -1);
        if (nready == -1)
        {
            perror("epoll_wait error");
        }
        for (i = 0; i < nready; i++)
        {
            if (!evts[i].events & EPOLLIN)
            { // 如果不是读事件，继续循环
                continue;
            }
            if (evts[i].data.fd == listen_file_descriptor)
            {
                clilen = sizeof(client_address);
                connection_file_descriptor = accept(listen_file_descriptor, (struct sockaddr *)&client_address, &clilen);
                printf("Received from %s at PORT %d\n", inet_ntop(AF_INET, &client_address.sin_addr, str, sizeof(str)), ntohs(client_address.sin_port));
                printf("cfd %d---client %d\n", connection_file_descriptor, ++num);

                evt.events = EPOLLIN;
                evt.data.fd = connection_file_descriptor;
                res = epoll_ctl(epoll_file_descriptor, EPOLL_CTL_ADD, connection_file_descriptor, &evt);
                if (res == -1)
                {
                    perror("epoll_ctl error");
                }
            }
            else
            {
                socket_file_descriptor = evts[i].data.fd;
                n = read(socket_file_descriptor, buf, MAXLINE);
                if (n == 0)
                {
                    res = epoll_ctl(epoll_file_descriptor, EPOLL_CTL_DEL, socket_file_descriptor, NULL);
                    if (res == -1)
                    {
                        perror("epoll_ctl delete error");
                    }
                    close(socket_file_descriptor);
                    printf("client[%d] closed connection\n", socket_file_descriptor);
                }
                else if (n < 0)
                {
                    perror("read n < 0 error: ");
                    res = epoll_ctl(epoll_file_descriptor, EPOLL_CTL_DEL, socket_file_descriptor, NULL);
                    if (res == -1)
                    {
                        perror("epoll_ctl delete error");
                    }
                    close(socket_file_descriptor);
                    printf("client[%d] closed connection\n", socket_file_descriptor);
                }
                else
                {
                    if (buf[0] == '1')
                    {
                        struct timeval timeval;
                        gettimeofday(&timeval, NULL);
                        uint64_t sec = timeval.tv_sec;
                        uint64_t min = timeval.tv_sec / 60;
                        struct tm cur_tm;
                        localtime_r((time_t *)&sec, &cur_tm);
                        snprintf(buf, BUFSIZ, "process: %d: %d-%02d-%02d %02d:%02d:%02d", getpid(), cur_tm.tm_year + 1900, cur_tm.tm_mon + 1, cur_tm.tm_mday, cur_tm.tm_hour, cur_tm.tm_min, cur_tm.tm_sec);
                        printf("Calling: %s\n", buf);
                        write(socket_file_descriptor, buf, BUFSIZ);
                    }
                    else
                    {
                        printf("Received: %s\n", buf);
                        for (i = 0; i < n; i++)
                        {
                            buf[i] = toupper(buf[i]);
                        }
                        write(socket_file_descriptor, buf, n);
                    }
                }
            }
        }
    }

    return 0;
}