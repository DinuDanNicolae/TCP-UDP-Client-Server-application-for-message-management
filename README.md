# TCP-UDP-Client-Server-application-for-message-management

Dinu Dan 
323CC
PCOM Topic 2

The implementation of this topic consists of 3 files: subscriber.c, server.c, and the helpme.h header.

The helpme.h file contains the data structures necessary for the implementation of the topic:
struct tcp_packet: contains the fields necessary for a TCP packet received by the server from a TCP client

struct udp_packet: contains the fields necessary for a UDP packet received by the server from a UDP client

struct client: stores the client's ID, socket, number of subscriptions, the client's subscriptions, and their online/offline status.

struct protocol: stores the length of the packet and the buffer in which the packet is stored.

The server.c file contains the actual implementation of the server, which receives packets from both TCP and UDP clients and processes them according to their type.

For multiplexing, I used polls. Thus, two sockets are created on the same port, one TCP and one UDP, which are added to the poll array.
Depending on the event received from the poll, it is checked whether it is a TCP client, UDP client, or STDIN event, and the received packet is processed accordingly.
In the case of an event on the TCP socket, it is checked whether it is a new client or an existing client.
For a new client, it is added to the client array, and its fields in the structure are completed.

In the case of an event on the UDP socket, the packet is received.
Subsequently, we iterate through all clients, and for each client, it is checked whether they are subscribed to the topic found in the packet received from the UDP client.
If so, the packet is sent to the respective TCP client, modifying the content according to the packet's data type.

The functionality of subscribe/unsubscribe and client disconnection is implemented in the form of a passive TCP event.
Thus, a packet is received from a TCP client, and it is checked whether the receiving function returned 0.
In this case, the client sent an "exit" type packet, so it needs to be disconnected.
Otherwise, it is checked what kind of packet was received (either subscribe or unsubscribe).
For subscribe, the topic is copied into a character matrix, and the number of subscriptions is incremented.
For unsubscribe, the topic is searched for in the character matrix and deleted, shifting over it, and the number of subscriptions is decremented.

In the case of an event on STDIN, the command is read from the keyboard, and it is checked if it is "exit".
Then, the server is closed with exit(0);

The subscriber.c file contains the implementation of the TCP client.
For multiplexing, I used polls again. Thus, a TCP socket is created and connected to the server.
Initially, a packet containing the client's ID is sent. Subsequently, events are awaited either from STDIN or from the server.
In the case of events from STDIN, it is checked if it is "exit", and the client is closed with close(sockfd), and a packet is sent to the server to announce the disconnection.
In the case of a subscribe/unsubscribe, a packet is sent to the server with the respective topic to inform the server of the subscription.

Events from the server are of two types: data or disconnection.
In the case of a data event, the packet in which the subscribed topic is found is received.
Subsequently, its content is displayed in the form of <UDP_CLIENT_IP>:<UDP_CLIENT_PORT> - <TOPIC> - <DATA_TYPE> - <MESSAGE_VALUE>
In the case of a disconnection event from the server, the client will also be disconnected.
