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

typedef struct node
{
    struct
    {
        int number;
        char *msg;
        int checksum;
    } data;
    struct node *next;
} linkedNode; // data structor for storing data from two clients

typedef struct list
{
    linkedNode *head;
} linkedList;

//CRUD operation for linkedList
void addNode(linkedNode* head, int number, int checksum, char* msg)
{
    linkedNode* ptr = head;
    while(ptr->next != NULL)
    {
        ptr = ptr->next;
    } 
    linkedNode* new = (linkedNode*)malloc(sizeof(linkedNode));
    new->data.checksum = checksum;
    new->data.msg = msg;
    new->data.number = number;
    new->next = NULL;
    ptr->next = new;
}

void deleteNode(linkedNode* head, int number)
{
    linkedNode* ptr = head;
    linkedNode* nex = ptr->next;
    while(nex->data.number != number)
    {
        ptr = ptr->next;
        nex = nex->next;
    }
    ptr->next = nex->next;
    nex->next = NULL;
    free(nex->data.msg);
    free(nex);
}

char* checkNode(linkedNode* head, int number)
{
    linkedNode* ptr = head->next;
    while(ptr->data.number != number)
    {
        ptr = ptr->next;
    }
    return ptr->data.msg;
}

void updateNode(linkedNode* head, int number, char* msg)
{
    linkedNode* ptr = head->next;
    while(ptr->data.number != number)
    {
        ptr = ptr->next;
    }
    ptr->data.msg = msg;
}

// Building a Chat Room Server based on UDP connection
int main(int argv, char **argc)
{
    int size = 220 * 1024; // used for setsocketopt() func to set buffer size
    struct sockaddr_in server_address, client_address, client_addresses[2];
    int socket_file_descriptor; // used for UDP connection
    socklen_t client_address_length;
    int sockfd;
    char buf[BUFSIZ];
    char str[INET_ADDRSTRLEN];
    int i, n, count = 0;
    linkedList list;

    if (argv != 2)
    {
        printf("Usage: ./udp_server port\n");
        exit(0);
    }

    while ((socket_file_descriptor = socket(PF_INET, SOCK_DGRAM, 0)) == -1)
        ;

    bzero(&server_address, sizeof(server_address));
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(atoi(argc[1]));

    bind(socket_file_descriptor, (struct sockaddr *)&server_address, sizeof(server_address));

    printf("Accepting connections...\n");
    for (int i = 0; i < 2; i++)
    {
        bzero(&client_addresses[i], sizeof(client_addresses[i]));
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
        printf("received from %s at PORT %d \n", inet_ntop(AF_INET, &client_address.sin_addr, str, sizeof(str)), ntohs(client_address.sin_port));

        if (buf[0] == 'E' && buf[1] == 'O' && buf[2] == 'F')
        {
            printf("received EOF, ready to stop server\n");
            close(socket_file_descriptor);
            break;
        }
        printf("received: %s", buf);
        if (count != 2)
        {
            client_addresses[count].sin_family = client_address.sin_family;
            client_addresses[count].sin_addr.s_addr = client_address.sin_addr.s_addr;
            client_addresses[count].sin_port = client_address.sin_port;
            count++;
        }
        else
        {
            int judge = 0;
            for (int i = 0; i < 2; i++)
            {
                if (client_addresses[i].sin_addr.s_addr == client_address.sin_addr.s_addr && client_addresses[i].sin_port == client_address.sin_port)
                {
                    judge = 1;
                    break;
                }
            }
            if (judge == 0) // If no match in client_addresses array
            {
                client_addresses[1].sin_family = client_addresses[0].sin_family;
                client_addresses[1].sin_addr.s_addr = client_addresses[0].sin_addr.s_addr;
                client_addresses[1].sin_port = client_addresses[0].sin_port;
                client_addresses[0].sin_family = client_address.sin_family;
                client_addresses[0].sin_addr.s_addr = client_address.sin_addr.s_addr;
                client_addresses[0].sin_port = client_address.sin_port;
            } // Use LRU method to change the storage of addresses
        }
        if (count == 2)
        {
            for (int i = 0; i < 2; i++)
            {
                if (client_addresses[i].sin_addr.s_addr != client_address.sin_addr.s_addr ||
                    client_addresses[i].sin_port != client_address.sin_port)
                {
                    n = sendto(socket_file_descriptor, buf, n, 0, (struct sockaddr *)&client_addresses[i], client_address_length);
                    if (n == -1)
                    {
                        perror("sendto error");
                    }
                }
            }
        }
    }

    return 0;
}