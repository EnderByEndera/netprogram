#include<stdio.h>

#include<string.h>
#include<sys/types.h>

#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>

#include<netdb.h>
#include<errno.h>
#include<stdlib.h>
#include<unistd.h>

int main(int argc,char **argv){
    int sock,accetp_sock,sock_len,numbytes,i;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    char buff[BUFSIZ];
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8000);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    bzero( & (server_addr.sin_zero), 8);
    sock_len = sizeof( struct sockaddr_in);
    printf("start to listen on address %d, and port %d\n", server_addr.sin_addr.s_addr, ntohs(server_addr.sin_port));
    sock = socket(AF_INET, SOCK_STREAM, 0);
    while( bind(sock, (struct sockaddr *)&server_addr, sock_len) == -1);
    printf("bind success\n");
    while(listen(sock,10)==-1);
    printf("Listening..\n");
    printf("Ready for Accept,Waiting...\n");
    accetp_sock = accept(sock, (struct sockaddr *) & client_addr, &sock_len);
    printf("Get the client\n");
    numbytes = send(accetp_sock, "Welecome to my server\n", 21, 0);
    while((numbytes = recv(accetp_sock, buff, BUFSIZ, 0) ) > 0){
        buff[numbytes] = '\0';
        printf("%s\n", buff);
        if(send(accetp_sock,buff, numbytes, 0) < 0){
            perror("write");
            return 1;
        }
    }
    close( accetp_sock);
    close( sock);
    return 0;
}   