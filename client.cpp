#include <iostream>
#include <string>
#include <random>  // For random number generation
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

float generate_random_level(float min = 0.0f, float max = 100.0f) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(min, max);
    return dis(gen);
}
float generate_random_time(float min = 1000.0f, float max = 3000.0f) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(min, max);
    return dis(gen);
}

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    std::string sensor_id;
        std::cout << "Sensor ID (type 'exit' to quit): ";
        std::cin >> sensor_id;

    while (true) {
        Sleep(generate_random_time());
        float level = generate_random_level();
        std::cout << "Generated Water Level: " << level << std::endl;

        SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) {
            std::cerr << "Failed to create socket.\n";
            continue;
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(12345);
        server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) == 0) {
            std::string msg = sensor_id + " " + std::to_string(level);
            send(sock, msg.c_str(), msg.size(), 0);
        } else {
            std::cerr << "Failed to connect to server.\n";
        }

#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
    }

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
