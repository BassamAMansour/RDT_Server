//
// Created by bassam on 27/11/2019.
//

#ifndef RDT_SERVER_PACKETUTILS_H
#define RDT_SERVER_PACKETUTILS_H


#include <vector>
#include <string>
#include <fstream>
#include "Packets.h"

using namespace std;

class PacketUtils {

public:
    static vector<Packets::packet> loadFileIntoPackets(const string &fileName);

    static string getFileContents(const string& fileName);

    static vector<Packets::packet> loadContentIntoPackets(string fileContent);
};


#endif //RDT_SERVER_PACKETUTILS_H
