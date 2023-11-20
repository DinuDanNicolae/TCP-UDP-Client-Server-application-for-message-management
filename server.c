#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdbool.h>
#include "helpme.h"

struct sockaddr_in tcp_addr;
struct sockaddr_in udp_addr;
struct sockaddr_in client_addr;

int main(int argc, char *argv[]) { 

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    int MAX_CLIENTS = 10;

    struct client connected_clients[50];
    memset(connected_clients, 0, sizeof(connected_clients));

    memset (&tcp_addr, 0, sizeof(tcp_addr));
    memset (&udp_addr, 0, sizeof(udp_addr));

    int udp_sockfd; // UDP socket
    if ((udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("udp socket creation failed"); 
        //exit(EXIT_FAILURE); 
    } 

    int tcp_sockfd; //TCP socket
    if ((tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("tcp socket creation failed");
        //exit(EXIT_FAILURE);
    }

    int ret, enable = 1;
    if (setsockopt(tcp_sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("TCP setsockopt(SO_REUSEADDR) failed");
        //exit(EXIT_FAILURE);
    }

    if (setsockopt(udp_sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("UDP setsockopt(SO_REUSEADDR) failed");
        //exit(EXIT_FAILURE);
    }
    
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_addr.s_addr = INADDR_ANY;
    tcp_addr.sin_port = htons(port);

    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = INADDR_ANY;
    udp_addr.sin_port = htons(port);

    if (bind(tcp_sockfd, (struct sockaddr *) &tcp_addr, sizeof(tcp_addr)) < 0) {
        //perror("tcp bind failed");
        //exit(EXIT_FAILURE);
    }

    if (bind(udp_sockfd, (struct sockaddr *) &udp_addr, sizeof(udp_addr)) < 0) {
        //perror("udp bind failed");
        //exit(EXIT_FAILURE);
    }

    ret = listen(tcp_sockfd, MAX_CLIENTS);

    if (ret < 0) {
        perror("listen failed");
        //exit(EXIT_FAILURE);
    }

    struct pollfd poll_fds[50];

    poll_fds[0].fd = tcp_sockfd;
    poll_fds[0].events = POLLIN;

    poll_fds[1].fd = udp_sockfd;
    poll_fds[1].events = POLLIN;

    poll_fds[2].fd = STDIN_FILENO;
    poll_fds[2].events = POLLIN;

    while (1) {

        ret = poll(poll_fds, MAX_CLIENTS, -1);

        if (ret < 0) {
            perror("poll failed");
            //exit(EXIT_FAILURE);
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {

            if (poll_fds[i].revents & POLLIN) {

                if (poll_fds[i].fd == tcp_sockfd) {

                    struct tcp_packet tcp_packet;
                    socklen_t cli_len = sizeof(client_addr);

                    int tcp_client_sockfd = accept(tcp_sockfd, (struct sockaddr *) &client_addr, &cli_len);

                    if (tcp_client_sockfd < 0) {
                        perror("tcp accept failed");
                        //exit(EXIT_FAILURE);
                    }

                    ret = setsockopt(tcp_client_sockfd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int));

                    if (ret < 0) {
                        perror("tcp setsockopt(TCP_NODELAY) failed");
                        //exit(EXIT_FAILURE);
                    }

                    ret = recv(tcp_client_sockfd, &tcp_packet, sizeof(tcp_packet), 0);

                    if (ret < 0) {
                        perror("tcp recv failed");
                        //exit(EXIT_FAILURE);
                    }

                    int ok = 0;

                    // verificam daca clientul este deja conectat

                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (strcmp(connected_clients[j].id, tcp_packet.id) == 0) {
                            ok = 1;
                            if (connected_clients[j].online == true) {
                                printf("Client %s already connected.\n", tcp_packet.id);
                                close(tcp_client_sockfd);
                                break;
                            } else {
                                connected_clients[j].online = true;
                                connected_clients[j].socket = tcp_client_sockfd;
                                poll_fds[j].fd = tcp_client_sockfd;
                                poll_fds[j].events = POLLIN;
                                printf("New client %s connected from %s:%hu.\n", tcp_packet.id, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                                break;
                            }
                        } 
                    }

                    if (ok == 0) {
                        strcpy(connected_clients[MAX_CLIENTS].id, tcp_packet.id);
                        connected_clients[MAX_CLIENTS].online = true;
                        connected_clients[MAX_CLIENTS].socket = tcp_client_sockfd;
                        poll_fds[MAX_CLIENTS].fd = tcp_client_sockfd;
                        poll_fds[MAX_CLIENTS].events = POLLIN;
                        MAX_CLIENTS++;

                        printf("New client %s connected from %s:%hu.\n", tcp_packet.id, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                    }

                } else if (poll_fds[i].fd == udp_sockfd) {

                    struct udp_packet udp_packet;
                    socklen_t cli_len = sizeof(client_addr);

                    memset(&udp_packet, 0, sizeof(udp_packet));

                    int ret = recvfrom(udp_sockfd, &udp_packet, sizeof(udp_packet), 0, (struct sockaddr *) &client_addr, &cli_len);

                    if (ret < 0) {
                        perror("udp recvfrom failed");
                        //exit(EXIT_FAILURE);
                    }

                    // trimitem pachetul clientilor abonati

                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        for (int k = 0; k < connected_clients[j].nr_abonamente; k++) {
                            if (strcmp(connected_clients[j].abonamente[k], udp_packet.topic) == 0) {

                                char* ip_str = inet_ntoa(client_addr.sin_addr);
                                char port_str[6];
                                memset(port_str, 0, sizeof(port_str));
                                sprintf(port_str, "%d", ntohs(client_addr.sin_port));

                                if (udp_packet.type == 0) { // INT
                                    char buffer[1563];
                                    memset(buffer, 0, sizeof(buffer));
                                    strcpy(buffer, ip_str);
                                    strcat(buffer, ":");
                                    strcat(buffer, port_str);
                                    strcat(buffer, " - ");
                                    strcat(buffer, udp_packet.topic);
                                    strcat(buffer, " - ");
                                    strcat(buffer, "INT");
                                    strcat(buffer, " - ");
                                    unsigned char sign_bit;
                                    memcpy(&sign_bit, udp_packet.content, 1);
                                    int converted_number, nr;
                                    memcpy(&converted_number, udp_packet.content + 1, 4);
                                    if (sign_bit == 1) {
                                        nr = -ntohl(converted_number);
                                    } else {
                                        nr = ntohl(converted_number);
                                    }
                                    char number_str[33];
                                    sprintf(number_str, "%d", nr);
                                    strcat(buffer, number_str);
                                    // strcat(buffer, "\n");
                                    struct protocol packet;
                                    memset(packet.buffer, 0, sizeof(packet.buffer));
                                    packet.len = strlen(buffer);
                                    strcpy(packet.buffer, buffer);
                                    int n = send(connected_clients[j].socket, &packet, sizeof(struct protocol), 0);

                                    if (n < 0) {
                                        perror("send failed");
                                        // exit(EXIT_FAILURE);
                                    }

                                }
                                if (udp_packet.type == 3) { // STRING
                                    char buffer[1563];
                                    memset(buffer, 0, sizeof(buffer));
                                    strcpy(buffer, ip_str);
                                    strcat(buffer, ":");
                                    strcat(buffer, port_str);
                                    strcat(buffer, " - ");
                                    strcat(buffer, udp_packet.topic);
                                    strcat(buffer, " - ");
                                    strcat(buffer, "STRING");
                                    strcat(buffer, " - ");
                                    strcat(buffer, udp_packet.content);
                                    struct protocol packet;
                                    memset(packet.buffer, 0, sizeof(packet.buffer));
                                    packet.len = strlen(buffer);
                                    strcpy(packet.buffer, buffer);
                                    int n = send(connected_clients[j].socket,&packet, sizeof(struct protocol), 0);

                                    if (n < 0) {
                                        perror("send failed");
                                        // exit(EXIT_FAILURE);
                                    }

                                }

                                if (udp_packet.type == 1) { // SHORT_REAL
                                    char buffer[1563];
                                    memset(buffer, 0, sizeof(buffer));
                                    strcpy(buffer, ip_str);
                                    strcat(buffer, ":");
                                    strcat(buffer, port_str);
                                    strcat(buffer, " - ");
                                    strcat(buffer, udp_packet.topic);
                                    strcat(buffer, " - ");
                                    strcat(buffer, "SHORT_REAL");
                                    strcat(buffer, " - ");
                                    float converted_number = ntohs(*(uint16_t*)(udp_packet.content));
                                    char number_str[20];
                                    sprintf(number_str, "%.2f", converted_number / 100);
                                    strcat(buffer, number_str);
                                    struct protocol packet;
                                    memset(packet.buffer, 0, sizeof(packet.buffer));
                                    packet.len = strlen(buffer);
                                    strcpy(packet.buffer, buffer);
                                    int n = send(connected_clients[j].socket, &packet, sizeof(struct protocol), 0);


                                    if (n < 0) {
                                        perror("send failed");
                                        // exit(EXIT_FAILURE);
                                    }
                                }

                                if (udp_packet.type == 2) { // float
                                    char buffer[1566];
                                    memset(buffer, 0, sizeof(buffer));
                                    strcpy(buffer, ip_str);
                                    strcat(buffer, ":");
                                    strcat(buffer, port_str);
                                    strcat(buffer, " - ");
                                    strcat(buffer, udp_packet.topic);
                                    strcat(buffer, " - ");
                                    strcat(buffer, "FLOAT");
                                    strcat(buffer, " - ");
                                    unsigned char sign_bit;
                                    memcpy(&sign_bit, udp_packet.content, 1);
                                    float converted_number = ntohl(*(uint32_t*)(udp_packet.content + 1));

                                    float divisor = 1.0;
                                    int power = udp_packet.content[5];
                                    for (int j = 0; j < power; j++) {
                                        divisor *= 10.0;
                                    }
                                    converted_number /= divisor;

                                    if (sign_bit == 1) {
                                        converted_number = -converted_number;
                                    }

                                    char number_str[20];
                                    sprintf(number_str, "%.*lf", power ,converted_number);
                                    strcat(buffer, number_str);
                                    struct protocol packet;
                                    memset(packet.buffer, 0, sizeof(packet.buffer));
                                    packet.len = strlen(buffer);
                                    strcpy(packet.buffer, buffer);
                                    int n = send(connected_clients[j].socket, &packet, sizeof(struct protocol), 0);


                                    if (n < 0) {
                                        perror("send failed");
                                        // exit(EXIT_FAILURE);
                                    }
                                }
                            }
                        }
                    }

                } else if (poll_fds[i].fd == STDIN_FILENO) {

                    // verificam comanda exit

                    char buf[50];
                    memset(buf, 0, sizeof(buf));
                    fgets(buf, sizeof(buf), stdin);

                    if (strncmp(buf, "exit", 4) == 0) {
                        // printf("Server stopped.\n");
                        exit(EXIT_SUCCESS);
                    }

                } else {

                    struct tcp_packet tcp_packet;

                    // primim pachet de la clientii tcp
                    // verificam daca este de tip subscribe/unsubscribe sau deconectare

                    int n = recv(poll_fds[i].fd, &tcp_packet, sizeof(tcp_packet), 0);

                    if (n < 0) {
                        perror("tcp recv failed");
                        //exit(EXIT_FAILURE);
                    }

                    if (n == 0) {
                        printf("Client %s disconnected.\n", tcp_packet.id);
                        close(poll_fds[i].fd);
                        connected_clients[i].online = false;
                        poll_fds[i].fd = -1;
                    }

                    if (n > 0) {
                        if (tcp_packet.type == 1) {

                            // adaugam la abonamentele clientului
                            
                            strcpy(connected_clients[i].abonamente[connected_clients[i].nr_abonamente], tcp_packet.topic);
                            connected_clients[i].abonamente[connected_clients[i].nr_abonamente][strlen(tcp_packet.topic) + 1] = '\0';
                            connected_clients[i].nr_abonamente++;

                        }
                        if (tcp_packet.type == 2) {
                            
                            struct client client;
                            strcpy(client.id, tcp_packet.id);
                            client.socket = poll_fds[i].fd;

                            // scoatem din abonamentele clientului

                            for (int j = 0; j < connected_clients[i].nr_abonamente; j++) {
                                if (strcmp (tcp_packet.topic, connected_clients[i].abonamente[j]) == 0) {
                                    for (int k = j; k < connected_clients[i].nr_abonamente - 1; k++) {
                                        strcpy(connected_clients[i].abonamente[k], connected_clients[i].abonamente[k + 1]);
                                    }
                                    connected_clients[i].nr_abonamente--;
                                }
                            } 
                        }
                    }
                } 
            }
        }
    }

    close(tcp_sockfd);
    close(udp_sockfd);

    return 0;

}