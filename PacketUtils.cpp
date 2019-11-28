//
// Created by bassam on 27/11/2019.
//

#include <cstring>
#include "PacketUtils.h"


vector<Packets::packet> PacketUtils::loadFileIntoPackets(const string &fileName) {
    return loadContentIntoPackets(getFileContents(fileName));
}

vector<Packets::packet> PacketUtils::loadContentIntoPackets(string fileContent) {
    vector<Packets::packet> packets;
    packets.reserve((fileContent.size() / Packets::MAX_DATA_SIZE) + 1);

    char fileContentArray[fileContent.size()];
    copy(fileContent.begin(), fileContent.end(), fileContentArray);

    int bytesLeft = fileContent.size();
    int iterator = 0;
    uint32_t sequenceNumber = 1;

    while (bytesLeft > 0) {

        int bytesToSend = min((int) Packets::MAX_DATA_SIZE, bytesLeft);

        struct Packets::packet packet{0, static_cast<uint16_t>(bytesToSend + Packets::PACKET_HEADER_SIZE),
                                      sequenceNumber, ' '};

        memcpy(packet.data, fileContentArray + iterator, bytesToSend);

        packets.push_back(packet);

        bytesLeft -= bytesToSend;
        iterator += bytesToSend;
        sequenceNumber++;
    }

    return packets;
}

string PacketUtils::getFileContents(const string &fileName) {

    string fileContents;

    ifstream inFile(string("./").append(fileName), ios::binary);

    if (inFile.fail()) {
        perror("Error while opening the file.");
        return "";
    }

    string line;
    while (inFile) {
        getline(inFile, line);
        fileContents.append(line);
    }

    inFile.close();

    return fileContents;
}
