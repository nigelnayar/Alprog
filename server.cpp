#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <map>
#include <fstream>
#include <ctime>
#include <sstream>
#include "include/nlohmann/json.hpp"
#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

std::mutex data_mutex;
std::map<std::string, float> latest_readings;
std::vector<std::string> critical_events;

void save_to_binary(const std::string& filename, const std::map<std::string, float>& data) {
    std::ofstream out(filename, std::ios::binary);
    for (const auto& [id, level] : data) {
        size_t len = id.size();
        out.write(reinterpret_cast<const char*>(&len), sizeof(len));
        out.write(id.c_str(), len);
        out.write(reinterpret_cast<const char*>(&level), sizeof(level));
    }
}

void export_to_json(const std::string& filename, const std::vector<std::string>& events) {
    nlohmann::json j(events);
    std::ofstream out(filename);
    out << j.dump(4);
}

void handle_client(SOCKET client_sock) {
    char buffer[256];
    int bytes_received = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        std::string msg(buffer);
        std::istringstream iss(msg);
        std::string sensor_id;
        float level;
        time_t now = time(nullptr);

        if (iss >> sensor_id >> level) {
            std::lock_guard<std::mutex> lock(data_mutex);
            latest_readings[sensor_id] = level;
            std::cout << "[" << std::ctime(&now) << "] Sensor " << sensor_id << ": " << level << std::endl;

            if (level < 10.0f || level > 90.0f) {
                std::ostringstream event;
                event << "{" << std::endl <<"date: " << std::ctime(&now) << ", " << std::endl << "sensor_id:" << sensor_id << ", " << std::endl << "level: " << level << ", " << std::endl << "}" << std::endl;
                critical_events.push_back(event.str());
            }
        }
    }
#ifdef _WIN32
    closesocket(client_sock);
#else
    close(client_sock);
#endif
}

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    SOCKET server_sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(12345);

    bind(server_sock, (sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_sock, 5);

    std::cout << "Server is listening on port 12345...\n";

    while (true) {
        SOCKET client_sock = accept(server_sock, nullptr, nullptr);
        std::thread(handle_client, client_sock).detach();

        std::lock_guard<std::mutex> lock(data_mutex);
        save_to_binary("backup.bin", latest_readings);
        export_to_json("critical.json", critical_events);
    }

#ifdef _WIN32
    closesocket(server_sock);
    WSACleanup();
#else
    close(server_sock);
#endif
    return 0;
}