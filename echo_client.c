#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char **argv)
{
    int port;
    int sockfd, numbers;
    char buf[BUFSIZ];
    struct sockaddr_in sockaddress;

    if (argc != 3)
    {
        printf("usage: ./echo_client address port\n");
        return 0;
    }
    while ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        ;

    sockaddress.sin_family = AF_INET;
    sockaddress.sin_port = htons(atoi(argv[2]));
    sockaddress.sin_addr.s_addr = inet_addr(argv[1]);
    bzero(&(sockaddress.sin_zero), 8);
    while (connect(sockfd, (struct sockaddr *)&sockaddress, sizeof(struct sockaddr)) == -1)
        ;

    while (1)
    {
        scanf("%[^\n]%*c", buf);
        if (buf[0] == 'E' && buf[1] == 'O' && buf[2] == 'F') break;
        numbers = send(sockfd, buf, strlen(buf), 0);
        numbers = recv(sockfd, buf, BUFSIZ, 0);
        buf[numbers] = '\0';
        printf("%s\n", buf);
    }
    close(sockfd);
    return 0;
}