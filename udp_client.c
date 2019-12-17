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
#include <signal.h>
#include <sys/wait.h>
#include <pthread.h>

struct RA
{
    int *beep;
    int socket_file_descriptor;
    struct sockaddr_in server_address;
};

void wait_child(int signal);
void *heart_check(void *arg);
void *listen_thread(void *arg);
void *heart_send(void *arg);

int main(int argv, char **argc)
{
    struct sockaddr_in server_address;
    int socket_file_descriptor, n;
    char buf[BUFSIZ];
    int server_address_length, pid;
    char loginName[BUFSIZ];

    if (argv != 3)
    {
        printf("Usage: ./udp_client address port\n");
        exit(0);
    }

    socket_file_descriptor = socket(PF_INET, SOCK_DGRAM, 0);
    int *beep = (int *)malloc(sizeof(int));
    *beep = 0;

    bzero(&server_address, sizeof(server_address));
    inet_pton(AF_INET, argc[1], &server_address.sin_addr);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(atoi(argc[2]));
    server_address_length = sizeof(server_address);

    pthread_t heart_check_thid, listen_thid, heart_send_thid;
    struct RA *pthread_arg = (struct RA *)malloc(sizeof(struct RA));
    pthread_arg->beep = beep;
    pthread_arg->server_address = server_address;
    pthread_arg->socket_file_descriptor = socket_file_descriptor;

    while (1)
    {
        printf("Welcome, please input your name: ");
        fgets(loginName, BUFSIZ, stdin);
        if (strlen(loginName) <= 1)
        {
            printf("Name cannot be empty.\n");
        }
        else
            break;
    }

    loginName[strlen(loginName) - 1] = ':';

    char temp[BUFSIZ] = {0};
    strcat(temp, loginName);
    temp[strlen(loginName) - 1] = ':';
    strcat(temp, " went online\n");
    n = sendto(socket_file_descriptor, temp, strlen(temp), 0, (struct sockaddr *)&server_address, sizeof(server_address));

    int err = pthread_create(&heart_check_thid, NULL, heart_check, (void *)pthread_arg);
    if (err < 0)
    {
        perror("pthread_create error");
        exit(1);
    }
    err = pthread_detach(heart_check_thid);
    if (err < 0)
    {
        perror("pthread_detach error");
        exit(1);
    }

    err = pthread_create(&listen_thid, NULL, listen_thread, (void *)pthread_arg);
    if (err < 0)
    {
        perror("pthread_create error");
        exit(1);
    }
    err = pthread_detach(listen_thid);
    if (err < 0)
    {
        perror("pthread_detach error");
        exit(1);
    }

    err = pthread_create(&heart_send_thid, NULL, heart_send, (void *)pthread_arg);
    if (err < 0)
    {
        perror("pthread_create error");
        exit(1);
    }
    err = pthread_detach(heart_send_thid);
    if (err < 0)
    {
        perror("pthread_detach error");
        exit(1);
    }

    while (fgets(buf, BUFSIZ, stdin) != NULL)
    {
        if (strlen(buf) <= 1)
        {
            continue;
        }

        if (strcmp(buf, "EOF\n") == 0)
        {
            n = sendto(socket_file_descriptor, buf, strlen(buf), 0, (struct sockaddr *)&server_address, sizeof(server_address));
            if (n == -1)
            {
                perror("sendto error");
            }
            continue;
        }

        if (strcmp(buf, "exit\n") == 0)
        {
            n = sendto(socket_file_descriptor, buf, strlen(buf), 0, (struct sockaddr *)&server_address, sizeof(server_address));
            if (n == -1)
            {
                perror("sendto error");
            }
            break;
        }

        char temp[BUFSIZ] = {0};
        strcat(temp, loginName);
        strcat(temp, " ");
        strcat(temp, buf);
        strcpy(buf, temp);
        n = sendto(socket_file_descriptor, buf, strlen(buf), 0, (struct sockaddr *)&server_address, sizeof(server_address));
        if (n == -1)
        {
            perror("sendto error");
            exit(0);
        }
    }
    *beep = -1;
    close(socket_file_descriptor);
    return 0;
}

void *heart_check(void *arg)
{
    struct RA *tmpRA = (struct RA *)arg;
    int *beep = tmpRA->beep;

    while (1)
    {
        if (*beep == -1)
        {
            return beep;
        }
        (*beep)++;
        if (*beep == 16)
        {
            printf("WARNING: Server disconnected. Reconnecting...\n");
        }
        else if (*beep > 35)
        {
            printf("Error: Server disconnected. Waiting...\n");
            *beep = -1;
            sleep(29);
        }
        sleep(1);
    }
}

void *listen_thread(void *arg)
{
    struct RA *tmpRA = (struct RA *)arg;
    char buf[BUFSIZ];
    socklen_t server_address_length = sizeof(tmpRA->server_address);

    while (1)
    {
        int n = recvfrom(tmpRA->socket_file_descriptor, buf, BUFSIZ, 0, (struct sockaddr *)&(tmpRA->server_address), &server_address_length);
        if (n == -1)
        {
            perror("recvfrom error");
            exit(0);
        }
        buf[n] = '\0';

        if (strcmp(buf, "This is the Heart Beep") == 0)
        {
            *(tmpRA->beep) = 0;
            continue;
        }

        printf("%s", buf);
        if (strcmp(buf, "Server shutdown\n") == 0)
        {
            break;
        }
    }
    *tmpRA->beep = -1;
    close(tmpRA->socket_file_descriptor);
    return 0;
}

void *heart_send(void *arg)
{
    struct RA *tmpRA = (struct RA *)arg;
    char buf[BUFSIZ];
    while (1)
    {
        strcpy(buf, "This is the Heart Beep");
        int n = sendto(tmpRA->socket_file_descriptor, buf, strlen(buf), 0, (struct sockaddr *)&(tmpRA->server_address), sizeof(tmpRA->server_address));
        if (n == -1)
        {
            perror("heart beat sendto error");
        }
        sleep(15);
    }

    return 0;
}