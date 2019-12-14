#include <stdio.h>

#include <string.h>
#include <sys/types.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <netdb.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    int sock, accetp_sock, sock_len, numbytes, i;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    char buff[BUFSIZ];
    if (argc - 1 != 2)
    {
        printf("Usage: ./echo_server address port\n");
        exit(0);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    server_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(server_addr.sin_zero), 8);
    sock_len = sizeof(struct sockaddr_in);
    sock = socket(AF_INET, SOCK_STREAM, 0);
    while (bind(sock, (struct sockaddr *)&server_addr, sock_len) == -1)
        ;
    while (listen(sock, 10) == -1)
        ;
    while (1)
    {
        accetp_sock = accept(sock, (struct sockaddr *)&client_addr, &sock_len);
        while (1)
        {
            numbytes = recv(accetp_sock, buff, BUFSIZ, 0);
            if (numbytes <= 0)
            {
                close(accetp_sock);
                break;
            }
            buff[numbytes] = '\0';
            printf("%s\n", buff);
            if (send(accetp_sock, buff, numbytes, 0) < 0)
            {
                perror("write");
                return 1;
            }
        }
    }
    close(sock);
    return 0;
}