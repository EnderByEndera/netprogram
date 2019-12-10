#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <ctype.h>

#define SERVER_PORT 8000

int main() {
    printf("Started\n");
    int i, j, n, maxi;

    int nready, client[FD_SETSIZE]; // 自定义数组client，防止遍历1024个文件描述符 FD_SETSIZE 默认为1024
    int maxfd, listenfd, connfd, sockfd;
    char buf[BUFSIZ], str[INET_ADDRSTRLEN]; // #define INET_ADDRSTRLEN 16

    struct sockaddr_in client_address, server_address;
    socklen_t client_address_length;
    fd_set read_set, all_set; // rset读事件文件描述符集合 allset用来暂存

    listenfd = socket(PF_INET, SOCK_STREAM, 0);

    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(SERVER_PORT);
    printf("Server address initialized\n");

    if (bind(listenfd, (struct sockaddr*) &server_address, sizeof(server_address)) == -1) {
        printf("Bind failed\n");
        exit(1);
    }
    printf("Bind success\n");
    listen(listenfd, 20);
    printf("Start listening\n");

    maxfd = listenfd; // 起初listenfd即为最大文件描述符

    maxi = -1; // 将来用作client[]的下标，初始值指向n个元素之前下标位置
    for (i = 0; i < FD_SETSIZE; i++) {
        client[i] = -1;
    }
    FD_ZERO(&all_set);
    FD_SET(listenfd, &all_set);
    printf("Initialization finished\n");

    while (1) {
        read_set = all_set;
        nready = select(maxfd + 1, &read_set, NULL, NULL, NULL);
        printf("Select finished\n");
        if (nready < 0) {
            perror("select error");
            exit(0);
        }
        if (FD_ISSET(listenfd, &read_set)) {
            client_address_length = sizeof(client_address);
            connfd = accept(listenfd, (struct sockaddr *) &client_address, &client_address_length);
            printf("Received from IP: %s, Port: %d \n", inet_ntop(AF_INET, &client_address.sin_addr, str, sizeof(str)), ntohs(client_address.sin_port));
            for (i = 0; i < FD_SETSIZE; i++) {
                if (client[i] < 0) {
                    client[i] = connfd;
                    break;
                }
            }
            printf("Added to the bitmap\n");
            if (i == FD_SETSIZE) {
                perror("Reached limitation of the connection number\n");
                exit(1);
            }
            
            FD_SET(connfd, &all_set);
            
            if (connfd > maxfd) {
                maxfd = connfd;
            }

            if (i > maxi) {
                maxi = i;
            } // Ensuring that the maxi is the largest number of fd;

            if (--nready == 0) {
                continue;
            }
        }

        for (i = 0; i < maxfd; i++) {
            if ((sockfd = client[i]) < 0) {
                continue;
            }
            printf("Find the right sockfd\n");
            if (FD_ISSET(sockfd, &read_set)) {
                printf("Start to read from client\n");
                n = read(sockfd, buf, sizeof(buf));
                if (n == 0) {
                    printf("Client start to end the communication\n"); // EOF
                    close(sockfd);
                    FD_CLR(sockfd, &read_set);
                    client[i] = -1;
                } else if (n > 0) { // Something is written by client
                    for (j = 0; j < n; j++) {
                        buf[j] = toupper(buf[j]);
                    }
                    // sleep(5);
                    write(sockfd, buf, sizeof(buf));
                }
                if (--nready == 0) {
                    break;
                }
            }
        }
    }

    close(listenfd);
    return 0;
}