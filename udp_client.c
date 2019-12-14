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

int main(int argv, char **argc)
{
    struct sockaddr_in server_address;
    int socket_file_descriptor, n;
    char buf[BUFSIZ];
    int server_address_length, pid;
    if (argv != 3)
    {
        printf("Usage: ./udp_client address port\n");
        exit(0);
    }

    socket_file_descriptor = socket(PF_INET, SOCK_DGRAM, 0);

    bzero(&server_address, sizeof(server_address));
    inet_pton(AF_INET, argc[1], &server_address.sin_addr);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(atoi(argc[2]));
    server_address_length = sizeof(server_address);
    n = sendto(socket_file_descriptor, buf, strlen(buf), 0, (struct sockaddr *) &server_address, sizeof(server_address));

    pid = fork();
    if (pid < 0)
    {
        perror("Fork error");
        exit(1);
    }
    else if (pid == 0)
    {
        while (fgets(buf, BUFSIZ, stdin) != NULL)
        {
            n = sendto(socket_file_descriptor, buf, strlen(buf), 0, (struct sockaddr *)&server_address, sizeof(server_address));
            if (n == -1)
            {
                perror("sendto error");
                exit(0);
            }
        }
    }
    else if (pid > 0)
    {
        while (1)
        {
            n = recvfrom(socket_file_descriptor, buf, BUFSIZ, 0, (struct sockaddr *)&server_address, &server_address_length);
            if (n == -1)
            {
                perror("recvfrom error");
                exit(0);
            }
            buf[n] = '\0';

            printf("Received: %s", buf);
        }
    }

    return 0;
}