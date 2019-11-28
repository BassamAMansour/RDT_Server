#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <vector>
#include <set>
#include "PacketUtils.h"
#include "Packets.h"

#define PORT "2000"
#define RECV_BUFFER_SIZE 10000

using namespace std;

void setupAddressInfo(addrinfo **returnedAddressInfo);

int bindToSocket(addrinfo **addressInfo);

void serveClients(int mainSocketFd);

void *get_in_addr(struct sockaddr *socketAddress);

void processAck(int socketFd, sockaddr_storage clientAddress, Packets::ack_packet ackPacket);

void serveNewClient(int socketFd, sockaddr_storage clientAddress, Packets::packet requestPacket);

void printReceivedFrom(sockaddr_storage &clientAddress);

void printPacket(Packets::packet packet);

void sendPacket(int socketFd, sockaddr_storage clientAddress, Packets::packet packet);

bool dropPacket();

void receivePacket(int mainSocketFd, sockaddr_storage &clientAddress, Packets::packet &receivedPacket);

set<uint32_t> acksReceived;

float lossProbability = 1.0;
int seedValue;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "usage: seed value and PLP\n");
        exit(1);
    }

    lossProbability = atof(argv[2]);
    seedValue = atoi(argv[1]);

    struct addrinfo *addressInfo;

    setupAddressInfo(&addressInfo);
    int socketFd = bindToSocket(&addressInfo);

    serveClients(socketFd);

    close(socketFd);

    return 0;
}

void serveClients(int mainSocketFd) {
    struct sockaddr_storage clientAddress{};
    Packets::packet receivedPacket{};

    while (true) {
        receivePacket(mainSocketFd, clientAddress, receivedPacket);

        if (receivedPacket.len > Packets::ACK_PACKET_SIZE) { // New client request

            serveNewClient(mainSocketFd, clientAddress, receivedPacket);
        } else if (receivedPacket.len == Packets::ACK_PACKET_SIZE) { //Handle unhandled Ack
            printf("Unhandled ack received.");

            struct Packets::ack_packet ackPacket{};

            memcpy(&ackPacket, &receivedPacket, Packets::ACK_PACKET_SIZE);

            processAck(mainSocketFd, clientAddress, ackPacket);
        } else {
            perror("serveClient: unrecognized type of received packet.");
            break;
        }
    }
}

void receivePacket(int mainSocketFd, sockaddr_storage &clientAddress, Packets::packet &receivedPacket) {
    char buffer[RECV_BUFFER_SIZE + 1];
    int receivedBytes;

    printf("server: waiting to recvfrom...\n");

    socklen_t clientAddressLen = sizeof clientAddress;
    if ((receivedBytes = recvfrom(mainSocketFd, buffer, RECV_BUFFER_SIZE, 0, (struct sockaddr *) &clientAddress,
                                  &clientAddressLen)) == -1) {
        perror("recvfrom");
        exit(1);
    }

    memcpy(&receivedPacket, buffer, receivedBytes);

    //printReceivedFrom(clientAddress);
    printPacket(receivedPacket);
}

void serveNewClient(int socketFd, sockaddr_storage clientAddress, Packets::packet requestPacket) {
    string fileName(requestPacket.data, requestPacket.len - Packets::PACKET_HEADER_SIZE);

    vector<Packets::packet> filePackets = PacketUtils::loadFileIntoPackets(fileName);

    //TODO: Handle it to congestion control

    string packetsCount = to_string(filePackets.size());

    struct Packets::packet fileInfoPacket{0, static_cast<uint16_t>(Packets::PACKET_HEADER_SIZE + packetsCount.size()),
                                          filePackets[0].seqno - 1, ' '};
    copy(packetsCount.begin(), packetsCount.end(), fileInfoPacket.data);

    sendPacket(socketFd, clientAddress, fileInfoPacket);

    if (acksReceived.find(fileInfoPacket.seqno) != acksReceived.end()) { printf("File Info Ack not received."); }

    acksReceived.erase(fileInfoPacket.seqno);

    for (auto packet:filePackets) {
        if (!dropPacket()) {
            sendPacket(socketFd, clientAddress, packet);
        } else {
            printf("Packet with seq#: %d dropped intentionally.\n", packet.seqno);
        }
    }
}

void sendPacket(int socketFd, sockaddr_storage clientAddress, Packets::packet packet) {
    char buffer[packet.len];
    memcpy(buffer, (const unsigned char *) &packet, packet.len);

    int sentBytes = 0;
    if ((sentBytes = sendto(socketFd, buffer, packet.len, 0,
                            (struct sockaddr *) &clientAddress, sizeof(clientAddress))) == -1) {
        perror("client: sendto");
        exit(1);
    }

    if (sentBytes < packet.len) {
        perror("sendPacket: sent bytes are less than the actual length.");
    }

    printf("Send Packet: seq#-> %d\n", packet.seqno);
}

void processAck(int socketFd, sockaddr_storage clientAddress, Packets::ack_packet ackPacket) {
    printf("Recv Ack: with seq#-> %d\n", ackPacket.seqno);
    acksReceived.insert(ackPacket.seqno);
//TODO:Implement
}

void setupAddressInfo(addrinfo **returnedAddressInfo) {
    struct addrinfo hints{};

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    int status = getaddrinfo(nullptr, PORT, &hints, returnedAddressInfo);

    if (status != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }
}

int bindToSocket(addrinfo **addressInfo) {
    int socketFd = -1;
    struct addrinfo *addressIterator;

    // loop through all the results and bind to the first we can
    for (addressIterator = *addressInfo; addressIterator != nullptr; addressIterator = addressIterator->ai_next) {

        if ((socketFd = socket(addressIterator->ai_family, addressIterator->ai_socktype,
                               addressIterator->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        // Free up socket if in use
        int yes = 1;
        if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(socketFd, addressIterator->ai_addr, addressIterator->ai_addrlen) == -1) {
            close(socketFd);
            perror("server: bind");
            continue;
        }

        break;
    }

    if (addressIterator == nullptr || socketFd == -1) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    freeaddrinfo(*addressInfo);

    return socketFd;
}

bool dropPacket() {
    //TODO: use dropProbability with seedValue
    return false;
}


void printReceivedFrom(sockaddr_storage &clientAddress) {
    char s[INET6_ADDRSTRLEN];
    printf("server: got packet from %s\n",
           inet_ntop(clientAddress.ss_family, get_in_addr((struct sockaddr *) &clientAddress), s, sizeof s));
}

void printPacket(Packets::packet packet) {
    printf("Packet: checkSum-> %d , length-> %d , seq#-> %d\n", packet.cksum, packet.len, packet.seqno);

    if (packet.len > Packets::ACK_PACKET_SIZE) {
        printf("Packet: data-> %s\n", packet.data);
    }
}

void *get_in_addr(struct sockaddr *socketAddress) {
    if (socketAddress->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) socketAddress)->sin_addr);
    }

    return &(((struct sockaddr_in6 *) socketAddress)->sin6_addr);
}