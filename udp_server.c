#include <stdio.h>

#include <string.h>
#include <sys/types.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include <netdb.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct msgnode
{
    struct
    {
        int number;
        char *msg;
        int size;
    } data;
    struct msgnode *next;
} msgNode; // data structor for storing data from two clients

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
    addrNode *addrHead;
    msgNode *msgHead;
    int fd;
    char *buf;
};

void addMsgNode(msgNode *head, char *msg, size_t size);
int addAddrNode(addrNode *head, struct sockaddr_in client_address, char *buf);
void deleteAddrNode(addrNode *head, msgNode *msgHead, int fd);
void broadcast(addrNode *head, char *msg, int fd, size_t msgSize, struct sockaddr_in client_address);
void *heart_check(void *arg);
int client_server_structure(int argv, char **argc);
int self_organize(int argv, char **argc);
int heart_beep_filter(char *buf, addrNode *addrHead, struct sockaddr_in client_address, int socket_file_descriptor);
int eof_filter(char *buf, msgNode *msgHead, addrNode *addrHead, int socket_file_descriptor, struct sockaddr_in client_address);
int exit_filter(char *buf, addrNode *addrHead, struct sockaddr_in client_address);

// Building a Chat Room Server based on UDP connection
int main(int argv, char **argc)
{
    if (argv != 2)
    {
        printf("Usage: ./udp_server port\n");
        exit(0);
    }

    int choice;
    printf("INFO: Starting UDP chat room server.\n");
    while (1)
    {
        printf("INFO: Please select the mode, Enter \"1\" or \"2\" to continue:\n");
        printf("INFO: 1. C/S Structure 2. Self-Organize Structure (1/2)\n");
        scanf("%d", &choice);
        switch (choice)
        {
        case 1:
            return client_server_structure(argv, argc);
        case 2:
            return self_organize(argv, argc);
        default:
            break;
        }
    }
}

int client_server_structure(int argv, char **argc)
{
    printf("INFO: Start C/S structure mode\n");
    int size = 220 * 1024; // used for setsocketopt() func to set buffer size
    struct sockaddr_in server_address, client_address;
    int socket_file_descriptor; // used for UDP connection
    socklen_t client_address_length;
    int sockfd;
    char buf[BUFSIZ];
    char str[INET_ADDRSTRLEN];
    int i, n;
    pthread_t thid_heart_check, thid_heart_listen;

    msgNode *msgHead = (msgNode *)malloc(sizeof(msgNode));
    addrNode *addrHead = (addrNode *)malloc(sizeof(addrNode));

    msgHead->next = NULL;
    addrHead->next = NULL;
    msgHead->data.number = 0;
    addrHead->data.number = 0;

    while ((socket_file_descriptor = socket(PF_INET, SOCK_DGRAM, 0)) == -1)
        ;

    bzero(&server_address, sizeof(server_address));
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(atoi(argc[1]));

    bind(socket_file_descriptor, (struct sockaddr *)&server_address, sizeof(server_address));
    setsockopt(socket_file_descriptor, SOL_SOCKET, SO_ATTACH_BPF, &size, sizeof(size));

    printf("INFO: Bind success, accepting UDP connections...\n");

    struct RA *thread_attr = (struct RA *)malloc(sizeof(struct RA));
    thread_attr->addrHead = addrHead;
    thread_attr->msgHead = msgHead;
    thread_attr->fd = socket_file_descriptor;
    thread_attr->buf = buf;
    int err = pthread_create(&thid_heart_check, NULL, heart_check, (void *)thread_attr);
    if (err != 0)
    {
        perror("pthread_create failure");
        exit(1);
    }
    err = pthread_detach(thid_heart_check);
    if (err != 0)
    {
        perror("pthread_detach failure");
        exit(1);
    }

    while (1)
    {
        client_address_length = sizeof(client_address);
        n = recvfrom(socket_file_descriptor, buf, BUFSIZ, 0, (struct sockaddr *)&client_address, &client_address_length);
        if (n == -1)
        {
            perror("recvfrom error");
        }
        buf[n] = '\0';

        int result = heart_beep_filter(buf, addrHead, client_address, socket_file_descriptor);
        if (result == 1)
            continue;

        result = eof_filter(buf, msgHead, addrHead, socket_file_descriptor, client_address);
        if (result == 1)
            break;

        result = exit_filter(buf, addrHead, client_address);
        if (result == 1)
            continue;

        printf("INFO: Received from IP: %s, PORT: %d\n", inet_ntop(AF_INET, &client_address.sin_addr, str, sizeof(str)), ntohs(client_address.sin_port));
        printf("INFO: Receive Message: %s", buf);
        i = addAddrNode(addrHead, client_address, buf);
        if (i == 0)
        {
            printf("INFO: New IP address added, IP: %s, PORT: %d\n", inet_ntop(AF_INET, &client_address.sin_addr, str, sizeof(str)), ntohs(client_address.sin_port));
            char temp[BUFSIZ] = "Connected to Server\n--------------Recent Message--------------\n";
            n = sendto(socket_file_descriptor, temp, strlen(temp), 0, (struct sockaddr *)&client_address, client_address_length);
            if (n == -1)
            {
                perror("sendto error");
            }
            msgNode *ptr = msgHead;
            while (ptr->next != NULL)
            {
                ptr = ptr->next;
                n = sendto(socket_file_descriptor, ptr->data.msg, ptr->data.size, 0, (struct sockaddr *)&client_address, sizeof(client_address));
                if (n == -1)
                {
                    perror("sendto error");
                }
            }
            strcpy(temp, "------------------------------------------\n");
            n = sendto(socket_file_descriptor, temp, strlen(temp), 0, (struct sockaddr *)&client_address, client_address_length);
            if (n == -1)
            {
                perror("sendto error");
            }
        }
        addMsgNode(msgHead, buf, n);
        broadcast(addrHead, buf, socket_file_descriptor, n, client_address);
    }

    return 0;
}

int self_organize(int argv, char **argc)
{
    printf("INFO: Start Self Organization mode\n");
    int size = 220 * 1024; // used for setsocketopt() func to set buffer size
    struct sockaddr_in server_address, client_address;
    int socket_file_descriptor; // used for UDP connection
    socklen_t client_address_length;
    int sockfd;
    char buf[BUFSIZ];
    char str[INET_ADDRSTRLEN];
    int i, n;
    pthread_t thid_heart_check, thid_heart_listen;

    msgNode *msgHead = (msgNode *)malloc(sizeof(msgNode));
    addrNode *addrHead = (addrNode *)malloc(sizeof(addrNode));

    msgHead->next = NULL;
    addrHead->next = NULL;

    while ((socket_file_descriptor = socket(PF_INET, SOCK_DGRAM, 0)) == -1)
        ;

    bzero(&server_address, sizeof(server_address));
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(atoi(argc[1]));

    bind(socket_file_descriptor, (struct sockaddr *)&server_address, sizeof(server_address));
    setsockopt(socket_file_descriptor, SOL_SOCKET, SO_ATTACH_BPF, &size, sizeof(size));

    struct RA *thread_attr = (struct RA *)malloc(sizeof(struct RA));
    thread_attr->addrHead = addrHead;
    thread_attr->msgHead = msgHead;
    thread_attr->fd = socket_file_descriptor;
    thread_attr->buf = buf;
    int err = pthread_create(&thid_heart_check, NULL, heart_check, (void *)thread_attr);
    if (err != 0)
    {
        perror("pthread_create failure");
        exit(1);
    }
    err = pthread_detach(thid_heart_check);
    if (err != 0)
    {
        perror("pthread_detach failure");
        exit(1);
    }

    printf("INFO: Bind success, accepting UDP connections...\n");

    while (1)
    {
        client_address_length = sizeof(client_address);
        n = recvfrom(socket_file_descriptor, buf, BUFSIZ, 0, (struct sockaddr *)&client_address, &client_address_length);
        if (n == -1)
        {
            perror("recvfrom error");
        }
        buf[n] = '\0';

        int result = heart_beep_filter(buf, addrHead, client_address, socket_file_descriptor);
        if (result == 1)
            continue;

        result = eof_filter(buf, msgHead, addrHead, socket_file_descriptor, client_address);
        if (result == 1)
            break;

        result = exit_filter(buf, addrHead, client_address);
        if (result == 1)
            continue;

        printf("INFO: Receive Messge: %s", buf);

        i = addAddrNode(addrHead, client_address, buf);
        if (i == 0)
        {
            printf("INFO: New IP address added, IP: %s, PORT: %d\n", inet_ntop(AF_INET, &client_address.sin_addr, str, sizeof(str)), ntohs(client_address.sin_port));
            addrNode *addrptr = addrHead;
            while (addrptr->next != NULL)
            {
                addrptr = addrptr->next;
                inet_ntop(AF_INET, &(addrptr->data.client_address.sin_addr), str, strlen(str));
                snprintf(buf, BUFSIZ, "^: address: %s, port: %d\n", str, ntohs(addrptr->data.client_address.sin_port));
                broadcast(addrHead, buf, socket_file_descriptor, strlen(buf), client_address);
            }
        }
    }

    return 0;
}

void addMsgNode(msgNode *head, char *msg, size_t size)
{
    msgNode *ptr = head;
    while (ptr->next != NULL)
    {
        ptr = ptr->next;
    }
    msgNode *new = (msgNode *)malloc(sizeof(msgNode));
    new->next = NULL;
    new->data.msg = (char *)malloc(sizeof(char) * size);
    for (size_t i = 0; i < size; i++)
    {
        new->data.msg[i] = msg[i];
    }
    new->data.size = size;
    new->data.number = ptr->data.number + 1;
    ptr->next = new;
}

int addAddrNode(addrNode *head, struct sockaddr_in client_address, char *buf)
{
    addrNode *ptr = head;
    char name[32] = {0};
    int i;

    for (i = 0; buf[i] != ':' && i < 32; i++)
    {
        name[i] = buf[i];
    }

    while (ptr->next != NULL)
    {
        ptr = ptr->next;
        if (strcmp(ptr->data.name, name) == 0)
        {
            ptr->data.ttl = 20;
            if (ptr->data.client_address.sin_addr.s_addr != client_address.sin_addr.s_addr || ptr->data.client_address.sin_port != client_address.sin_port)
            {
                ptr->data.client_address = client_address;
                return 0;
            }
            else
            {
                return 1;
            }
        }
    }
    addrNode *new = (addrNode *)malloc(sizeof(addrNode));

    new->next = NULL;
    new->data.client_address = client_address;
    new->data.number = ptr->data.number + 1;
    new->data.ttl = 20;
    for (i = 0; buf[i] != ':' && i < 32; i++)
    {
        new->data.name[i] = buf[i];
    }
    new->data.name[i] = '\0';

    ptr->next = new;
    return 0;
}

void deleteAddrNode(addrNode *addrHead, msgNode *msgHead, int fd)
{
    addrNode *ptr = addrHead;
    addrNode *ptr2 = addrHead->next;
    char str[INET_ADDRSTRLEN];

    while (ptr->next != NULL)
    {
        inet_ntop(AF_INET, &ptr2->data.client_address.sin_addr, str, sizeof(str));
        ptr2->data.ttl--;
        if (ptr2->data.ttl > 0)
        {
            printf("INFO: time to live: %d of IP: %s at PORT %d\n", ptr2->data.ttl, str, ntohs(ptr2->data.client_address.sin_port));
        }
        else if (ptr2->data.ttl <= 0 && ptr2->data.ttl > -15)
        {
            printf("WARNING: Client address %s at PORT %d is not responded, waiting to be responeded, time to live: %d\n", str, ntohs(ptr2->data.client_address.sin_port), ptr2->data.ttl);
        }
        else
        {
            printf("ERROR: Client address %s at PORT %d is not responded, judged as offline.\n", str, ntohs(ptr2->data.client_address.sin_port));

            char buf[BUFSIZ];
            snprintf(buf, BUFSIZ, "%s: went offline.\n", ptr2->data.name);
            addMsgNode(msgHead, buf, strlen(buf));
            broadcast(addrHead, buf, fd, BUFSIZ, ptr2->data.client_address);
            ptr->next = ptr2->next;
            free(ptr2);
            ptr2 = ptr->next;
            continue;
        }
        ptr = ptr->next;
        ptr2 = ptr2->next;
    }
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

void *heart_check(void *arg)
{
    printf("INFO: Heart Check Thread opened, start to check heart beat of every address\n");
    struct RA *tmpRA = (struct RA *)arg;

    while (1)
    {
        deleteAddrNode(tmpRA->addrHead, tmpRA->msgHead, tmpRA->fd);
        sleep(1);
    }
}

int heart_beep_filter(char *buf, addrNode *addrHead, struct sockaddr_in client_address, int socket_file_descriptor)
{
    // Heart Beat Check Function
    socklen_t client_address_length = sizeof(client_address);
    if (strcmp(buf, "This is the Heart Beep") == 0)
    {
        addrNode *ptr = addrHead;
        while (ptr->next != NULL)
        {
            ptr = ptr->next;
            if (ptr->data.client_address.sin_addr.s_addr == client_address.sin_addr.s_addr && ptr->data.client_address.sin_port == client_address.sin_port)
            {
                ptr->data.ttl = 20;
                break;
            }
        }
        int n = sendto(socket_file_descriptor, buf, strlen(buf), 0, (struct sockaddr *)&client_address, client_address_length);
        if (n == -1)
        {
            perror("sendto error");
        }
        return 1;
    }
    return 0;
}

int eof_filter(char *buf, msgNode *msgHead, addrNode *addrHead, int socket_file_descriptor, struct sockaddr_in client_address)
{
    socklen_t client_address_length = sizeof(client_address);
    // EOF End Server Function
    if (strcmp(buf, "EOF\n") == 0)
    {
        printf("WARNING: EOF message received, ready to stop server\n");
        msgNode *msgptr = msgHead;
        addrNode *addrptr = addrHead;

        while (msgHead->next != NULL)
        {
            msgptr = msgptr->next;
            free(msgHead->data.msg);
            free(msgHead);
            msgHead = msgptr;
        }
        free(msgHead->data.msg);
        free(msgHead);
        char temp[BUFSIZ] = "Server shutdown\n";
        broadcast(addrHead, temp, socket_file_descriptor, BUFSIZ, client_address);
        while (addrHead->next != NULL)
        {
            addrptr = addrptr->next;
            free(addrHead);
            addrHead = addrptr;
        }
        free(msgHead->data.msg);
        free(msgHead);
        int n = sendto(socket_file_descriptor, temp, strlen(temp), 0, (struct sockaddr *)&client_address, client_address_length);
        if (n == -1)
        {
            perror("sendto error");
        }
        close(socket_file_descriptor);
        return 1;
    }
    return 0;
}

int exit_filter(char *buf, addrNode *addrHead, struct sockaddr_in client_address)
{
    if (strcmp(buf, "exit\n") == 0)
    {
        addrNode *ptr = addrHead;
        while (ptr->next != NULL)
        {
            ptr = ptr->next;
            if (ptr->data.client_address.sin_addr.s_addr == client_address.sin_addr.s_addr && ptr->data.client_address.sin_port == client_address.sin_port)
            {
                ptr->data.ttl = -100;
                break;
            }
        }
        return 1;
    }
    return 0;
}