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
#include <sys/time.h>

typedef struct addrnode
{
    struct
    {
        int number;
        struct sockaddr_in client_address;
        char name[32];
        int ttl;
    } data;
    struct addrnode *next;
} addrNode;

struct RA
{
    int *beep;
    int socket_file_descriptor;
    struct sockaddr_in server_address;
    addrNode *head;
};

void wait_child(int signal);
void *heart_check(void *arg);
void *listen_thread(void *arg);
void *heart_send(void *arg);
void broadcast(addrNode *head, char *msg, int fd, size_t msgSize, struct sockaddr_in client_address);
int address_filter(char *buf, struct RA *tmpRA);

int main(int argv, char **argc)
{
    struct sockaddr_in server_address;
    int socket_file_descriptor, n;
    char buf[BUFSIZ];
    int server_address_length;
    char loginName[32];
    addrNode *head = (addrNode *)malloc(sizeof(addrNode));
    head->next = NULL;
    head->data.number = 0;

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

    struct timeval tv;
    uint64_t sec;

    pthread_t heart_check_thid, listen_thid, heart_send_thid;
    struct RA *pthread_arg = (struct RA *)malloc(sizeof(struct RA));
    pthread_arg->beep = beep;
    pthread_arg->server_address = server_address;
    pthread_arg->socket_file_descriptor = socket_file_descriptor;
    pthread_arg->head = head;

    while (1)
    {
        printf("Welcome, please input your name: ");
        fgets(loginName, BUFSIZ, stdin);
        if (strlen(loginName) <= 1)
        {
            printf("Name cannot be empty.\n");
        }
        else if (strlen(loginName) > 16)
        {
            printf("Length of the name needs to be less than 16,\n");
        }
        else
            break;
    }

    loginName[strlen(loginName) - 1] = ':';

    char temp[BUFSIZ] = {0};
    strcat(temp, loginName);
    temp[strlen(loginName) - 1] = ':';
    strcat(temp, " went online    ");

    gettimeofday(&tv, NULL);
    sec = tv.tv_sec;
    struct tm cur_tm;
    localtime_r((time_t *)&sec, &cur_tm);
    snprintf(buf, BUFSIZ, "%d-%02d-%02d %02d:%02d:%02d\n", cur_tm.tm_year + 1900, cur_tm.tm_mon + 1, cur_tm.tm_mday, cur_tm.tm_hour, cur_tm.tm_min, cur_tm.tm_sec);

    strcat(temp, buf);
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
        temp[strlen(temp) - 1] = ' ';

        struct timeval tv;
        gettimeofday(&tv, NULL);
        uint64_t sec = tv.tv_sec;
        struct tm cur_tm;
        localtime_r((time_t *)&sec, &cur_tm);
        snprintf(buf, BUFSIZ, "   %d-%02d-%02d %02d:%02d:%02d\n", cur_tm.tm_year + 1900, cur_tm.tm_mon + 1, cur_tm.tm_mday, cur_tm.tm_hour, cur_tm.tm_min, cur_tm.tm_sec);

        strcat(temp, buf);
        strcpy(buf, temp);
        broadcast(head, buf, socket_file_descriptor, BUFSIZ, server_address);
        n = sendto(socket_file_descriptor, buf, strlen(buf), 0, (struct sockaddr *)&server_address, server_address_length);

        if (n == -1)
        {
            perror("sendto error");
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
    int time = 2;

    while (1)
    {
        if (*beep == -1)
        {
            printf("Error: Server disconnected. Exiting...\n");
            exit(0);
        }
        (*beep)++;
        if (*beep == 16)
        {
            printf("WARNING: Server disconnected. Reconnecting...\n");
            time = 1;
        }
        else if (*beep >= 35)
        {
            printf("WARNING: Reconnection failed. Waiting 30 seconds for server to respond...\n");
            *beep = -1;
            sleep(30 - time);
        }
        sleep(time);
    }
}

void *listen_thread(void *arg)
{
    struct RA *tmpRA = (struct RA *)arg;
    char buf[BUFSIZ];

    while (1)
    {
        int n = recv(tmpRA->socket_file_descriptor, buf, BUFSIZ, 0);
        if (n == -1)
        {
            perror("recv error");
            exit(0);
        }
        if (n >= BUFSIZ)
            buf[BUFSIZ - 1] = '\0';
        else 
            buf[n] = '\0';

        int result = address_filter(buf, tmpRA);
        if (result == 1)
        {
            continue;
        }

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
    int time = 15;

    while (1)
    {
        strcpy(buf, "This is the Heart Beep");
        int n = sendto(tmpRA->socket_file_descriptor, buf, strlen(buf), 0, (struct sockaddr *)&(tmpRA->server_address), sizeof(tmpRA->server_address));
        // printf("INFO: Sent Heart Beep Pack.\n");
        if (n == -1)
        {
            perror("heart beat sendto error");
            exit(0);
        }
        if (*(tmpRA->beep) > 8)
            time = 6;
        if (*(tmpRA->beep) == -1)
            time = 3;
        sleep(time);
    }

    return 0;
}

int address_filter(char *buf, struct RA *tmpRA)
{
    if (buf[0] == '^' && buf[1] == '.')
    {
        int i;
        for (i = 12; buf[i] != ',' && i < 28; i++)
            ;
        char address[i - 12 + 1];
        char port[5];
        for (i = 12; buf[i] != ',' && i < 28; i++)
        {
            address[i - 12] = buf[i];
        }
        address[i - 12] = '\0';
        addrNode *new = (addrNode *)malloc(sizeof(addrNode));
        new->next = NULL;
        int init;
        i += 8;
        for (init = i; buf[i] != '\n' && i - init < 5; i++)
        {
            port[i - init] = buf[i];
        }
        port[i - init] = '\0';

        int err = inet_pton(AF_INET, address, &new->data.client_address.sin_addr);
        if (err == 0)
        {
            perror("Convert IP address failed");
        }
        new->data.client_address.sin_port = htons(atoi(port));
        new->data.client_address.sin_family = AF_INET;

        addrNode *ptr = tmpRA->head;
        while (ptr->next != NULL)
        {
            ptr = ptr->next;
            if (ptr->data.client_address.sin_addr.s_addr == new->data.client_address.sin_addr.s_addr && ptr->data.client_address.sin_port == new->data.client_address.sin_port)
            {
                return 1;
            }
        }

        new->data.number = ptr->data.number + 1;
        ptr->next = new;
        return 1;
    }
    return 0;
}

void broadcast(addrNode *head, char *msg, int fd, size_t msgSize, struct sockaddr_in client_address)
{
    addrNode *ptr = head;
    while (ptr->next != NULL)
    {
        ptr = ptr->next;
        int numbytes = sendto(fd, msg, msgSize, 0, (struct sockaddr *)&ptr->data.client_address, sizeof(ptr->data.client_address));
        if (numbytes == -1)
        {
            perror("sendto error");
        }
    }
}