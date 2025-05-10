#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <set>
#include <string>
#include <algorithm>
#include "json.hpp"

#pragma comment(lib, "Ws2_32.lib") 

using json = nlohmann::json;


struct Packet {
    char symbol[5]; 
    char buysellindicator;
    int32_t quantity;
    int32_t price;
    int32_t packetSequence;
};


bool initWinsock() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed:amelia " << result << std::endl;
        return false;
    }
    return true;
}


SOCKET connectToServer(const std::string& host, int port) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Error creating socket: " << WSAGetLastError() << std::endl;
        return INVALID_SOCKET;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(host.c_str()); 

    if (server_addr.sin_addr.s_addr == INADDR_NONE) {
        std::cerr << "Invalid address: " << host << std::endl;
        closesocket(sock);
        return INVALID_SOCKET;
    }

    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        return INVALID_SOCKET;
    }

    return sock;
}


bool sendRequest(SOCKET sock, uint8_t callType, uint8_t resendSeq = 0) {
    uint8_t payload[2] = {callType, resendSeq};
    if (send(sock, (const char*)payload, 2, 0) != 2) {
        std::cerr << "Failed to send request: " << WSAGetLastError() << std::endl;
        return false;
    }
    return true;
}


bool receivePacket(SOCKET sock, Packet& packet) {
    char buffer[17];
    int bytes_received = 0;

    
    while (bytes_received < 17) {
        int ret = recv(sock, buffer + bytes_received, 17 - bytes_received, 0);
        if (ret <= 0) {
            if (ret == 0) {
                std::cout << "Server closed connection." << std::endl;
            } else {
                std::cerr << "Receive error: " << WSAGetLastError() << std::endl;
            }
            return false;
        }
        bytes_received += ret;
    }

    
    strncpy(packet.symbol, buffer, 4);
    packet.symbol[4] = '\0'; 
    packet.buysellindicator = buffer[4];

   
    packet.quantity = ntohl(*(int32_t*)(buffer + 5));
    packet.price = ntohl(*(int32_t*)(buffer + 9));
    packet.packetSequence = ntohl(*(int32_t*)(buffer + 13));

    return true;
}


void writeToJson(const std::vector<Packet>& packets, const std::string& filename) {
    json j_array = json::array();
    for (const auto& p : packets) {
        json j_obj;
        j_obj["symbol"] = p.symbol;
        j_obj["buysellindicator"] = std::string(1, p.buysellindicator);
        j_obj["quantity"] = p.quantity;
        j_obj["price"] = p.price;
        j_obj["packetSequence"] = p.packetSequence;
        j_array.push_back(j_obj);
    }

    std::ofstream ofs(filename);
    if (!ofs.is_open()) {
        std::cerr << "Failed to open " << filename << " for writing." << std::endl;
        return;
    }
    ofs << j_array.dump(4) << std::endl;
    ofs.close();
    std::cout << "JSON output written to " << filename << std::endl;
}

int main() {
    
    if (!initWinsock()) {
        return 1;
    }

    const std::string host = "127.0.0.1";
    const int port = 3000;
    std::vector<Packet> packets;
    std::set<int> received_sequences;

    
    SOCKET sock = connectToServer(host, port);
    if (sock == INVALID_SOCKET) {
        WSACleanup();
        return 1;
    }

    if (!sendRequest(sock, 1)) {
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    
    Packet packet;
    while (receivePacket(sock, packet)) {
        packets.push_back(packet);
        received_sequences.insert(packet.packetSequence);
        std::cout << "Received packet: symbol=" << packet.symbol
                  << ", sequence=" << packet.packetSequence << std::endl;
    }

    closesocket(sock);

    
    const int max_sequence = 14; 
    std::vector<int> missing_sequences;
    for (int i = 1; i <= max_sequence; ++i) {
        if (received_sequences.find(i) == received_sequences.end()) {
            missing_sequences.push_back(i);
        }
    }

    
    for (int seq : missing_sequences) {
        std::cout << "Requesting missing packet: sequence=" << seq << std::endl;
        sock = connectToServer(host, port);
        if (sock == INVALID_SOCKET) {
            WSACleanup();
            return 1;
        }

        if (!sendRequest(sock, 2, static_cast<uint8_t>(seq))) {
            closesocket(sock);
            continue;
        }

        if (receivePacket(sock, packet)) {
            if (packet.packetSequence == seq) {
                packets.push_back(packet);
                std::cout << "Received missing packet: symbol=" << packet.symbol
                          << ", sequence=" << packet.packetSequence << std::endl;
            }
        }

        closesocket(sock); 
    }

    
    std::sort(packets.begin(), packets.end(),
              [](const Packet& a, const Packet& b) {
                  return a.packetSequence < b.packetSequence;
              });

    
    writeToJson(packets, "output.json");

    
    WSACleanup();
    return 0;
}