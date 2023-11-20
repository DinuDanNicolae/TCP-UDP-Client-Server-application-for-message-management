#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <poll.h>
#include <netinet/tcp.h>
#include "helpme.h"

struct sockaddr_in tcp_addr;

int main(int argc, char *argv[]) {

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    if (argc != 4) {
        printf("\n Usage: %s <id Client> <ip> <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[3]);
    int client_sockfd;

    client_sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (client_sockfd < 0) {
        perror("socket creation failed");
        //exit(EXIT_FAILURE);
    }

    int ret, enable = 1;
    ret = setsockopt(client_sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&enable, sizeof(int));

    if (ret < 0) {
        perror("setsockopt(TCP_NODELAY) failed");
        //exit(EXIT_FAILURE);
    }

    memset(&tcp_addr, 0, sizeof(tcp_addr));
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_port = htons(port);
    tcp_addr.sin_addr.s_addr = inet_addr(argv[2]);

    ret = connect(client_sockfd, (struct sockaddr *)&tcp_addr, sizeof(tcp_addr));

    if (ret < 0) {
        perror("connect failed");
        //exit(EXIT_FAILURE);
    }

    struct tcp_packet packet;

    strcpy(packet.id, argv[1]);
    packet.type = 0;
    strcpy(packet.topic, "");

    // trimitem serverului id-ul clientului de conectare

    ret = send(client_sockfd, &packet, sizeof(packet), 0);

    if (ret < 0) {
        perror("send failed");
        //exit(EXIT_FAILURE);
    }

    char buf[100];
    memset(buf, 0, sizeof(buf));

    struct pollfd fds[2];

    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    fds[1].fd = client_sockfd;
    fds[1].events = POLLIN;

    int rc;

    while (1) {
        
        rc = poll(fds, 2, -1);

        if (rc < 0) {
            perror("poll failed");
            //exit(EXIT_FAILURE);
        }

        if (fds[0].revents & POLLIN) {

            memset(buf, 0, sizeof(buf));
            fgets(buf, sizeof(buf), stdin);

            // verificam comenzile de exit, subscribe si unsubscribe

            if(strncmp(buf, "exit", 4) == 0) {
                
                packet.type = 0;
                strcpy(packet.topic, "");
                strcpy(packet.id, argv[1]);

                rc = send(client_sockfd, &packet, sizeof(packet), 0);

                if (rc < 0) {
                    perror("send failed");
                    //exit(EXIT_FAILURE);
                }

                // shutdown(client_sockfd, SHUT_RDWR);
                close(client_sockfd);
                break;
            }

            if(strncmp(buf, "subscribe", 9) == 0) {

                char *token = strtok(buf, " ");
                token = strtok(NULL, " ");
        
                token[strlen(token)] = '\0';
                
                strcpy(packet.topic, token);
                packet.type = 1;
                strcpy(packet.id, argv[1]);

                rc = send(client_sockfd, &packet, sizeof(packet), 0);

                if (rc < 0) {
                    perror("send failed");
                    //exit(EXIT_FAILURE);
                }

                printf("Subscribed to topic.\n");


            }

            if(strncmp(buf, "unsubscribe", 11) == 0) {

                char *token = strtok(buf, " ");
                token = strtok(NULL, " ");
        
                token[strlen(token) - 1] = '\0';
                
                strcpy(packet.topic, token);
                packet.type = 2;
                strcpy(packet.id, argv[1]);

                rc = send(client_sockfd, &packet, sizeof(packet), 0);

                if (rc < 0) {
                    perror("send failed");
                    //exit(EXIT_FAILURE);
                }

                printf("Unsubscribed from topic.\n");

            }

        } else if (fds[1].revents & POLLIN) {

            struct protocol packet;
            memset(packet.buffer, 0, sizeof(packet.buffer));

            // primim mesajele de la server si le afisam

            rc = recv(client_sockfd, &packet, sizeof(struct protocol), 0);

            if (rc <= 0) {
                break;
            }

            if (rc > 0) {
                printf("%s\n", packet.buffer);
                // printf("\n");
            }
        }
    }

    close(client_sockfd);
    return 0;
} 