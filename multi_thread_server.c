#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#define SERV_PORT 10000
#define SERV_IP "127.0.0.1"
#define MAX_LISTEN_LEN 128

struct RA
{
    int connfd;
    struct sockaddr_in cli_addr;
};


void *thread_func(void *arg)
{
    struct RA *tmpRA = (struct RA *)arg;

    char buf[BUFSIZ] = {0};
    while(1)
    {
        memset(buf,0,sizeof(buf));
        read(tmpRA->connfd,buf,sizeof(buf));
        printf("client ipaddress is %s\n",inet_ntop(AF_INET,&(tmpRA->cli_addr.sin_addr),NULL,0));
        printf("client send is %s\n",buf);
        write(tmpRA->connfd,buf,sizeof(buf));
    }


    close(tmpRA->connfd);
    free(tmpRA);
}

int main()
{
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;
    int clilen = sizeof(cli_addr);
    int listenfd,connfd;
    pthread_t thid;
    struct RA *threadRa = NULL;
    //初始化
    memset((void *)&serv_addr,0,sizeof(serv_addr));
    memset((void *)&cli_addr,0,sizeof(cli_addr));

    //serv_addr初始化
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    listenfd = socket(PF_INET,SOCK_STREAM,0);


    bind(listenfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
    listen(listenfd,MAX_LISTEN_LEN);

    while(1)
    {
        connfd = accept(listenfd,(struct sockaddr*)&cli_addr,&clilen);

        threadRa = (struct RA *)malloc(sizeof(struct RA));
        threadRa->connfd = connfd;
        memcpy(&threadRa->cli_addr,&cli_addr,clilen);

        //把监听返回的套接字交给线程去处理自己继续监听
        pthread_create(&thid,NULL,thread_func,(void *)threadRa);
        pthread_detach(thid);

    }

    return 0;
}
