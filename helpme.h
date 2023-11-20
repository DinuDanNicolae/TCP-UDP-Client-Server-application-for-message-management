#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

struct tcp_packet {
    char topic[100];
    int type;
    char id[100];
};

struct udp_packet {
    char topic[50];
    uint8_t type;
    char content[1500];
};

struct client {
    char id[20];
    int socket;
    char abonamente[50][51];
    int nr_abonamente;
    bool online;
};

struct protocol {
    int len;
    char buffer[1700];
};