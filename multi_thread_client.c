
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>

#define SERV_IP "127.0.0.1"
#define SERV_PORT 10000
int main()
{
    struct sockaddr_in serv_addr;
    memset((void *)&serv_addr, 0, sizeof(struct sockaddr_in));

    char recvbuf[BUFSIZ] = {0};
    char buf[BUFSIZ] = {0};
    //初始化服务端的参数
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    int connfd = socket(AF_INET, SOCK_STREAM, 0);
    int n = connect(connfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (n == -1)
    {
        printf("client connect err");
        return -1;
    }
    memset(buf, 0, sizeof(buf));
    memset(recvbuf, 0, sizeof(recvbuf));
    while (1)
    {
        scanf("%s", buf);
        write(connfd, buf, strlen(buf));
        int readlen = read(connfd, recvbuf, sizeof(recvbuf));
        recvbuf[readlen] = '\0';
        printf("recvbuf is %s\n", recvbuf);
    }
    return 0;
}