#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <iostream>
#include <stdio.h>
#include <cstring>
#include <stdlib.h>
#include <unistd.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <cstring>
#endif

int main() {

    std::string user_answer;
    bool exitFlag = true;
    int server_port;
    char buffer[255];

    while(exitFlag) {

        do {
            std::cout << "ENTER PORT SERVER" << std::endl;
            std::cin >> user_answer;
            server_port = std::stoi(user_answer);
        } while (server_port <= 0);


        int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        #ifdef _WIN32
            if(sock == INVALID_SOCKET) {
                std::cerr << "FAILED TO CREATE SOCKET!" << std::endl;
                return EXIT_FAILURE;
            }
        #else
            if (sock == -1) {
                std::cerr << "FAILED TO CREATE SOCKET!" << std::endl;
                return EXIT_FAILURE;
            }
        #endif

        #ifdef _WIN32
            sockaddr_in addr;
            addr.sin_family = PF_INET;
            addr.sin_port = htons(server_port);
            addr.sin_addr.S_un.S_addr = INADDR_ANY;
        #else
            sockaddr_in addr =
                {
                    .sin_family = PF_INET,
                    .sin_port = htons(server_port),
                    .sin_addr = {.s_addr = INADDR_ANY}
                };
        #endif


        std::cout << "ENTER MESSAGE" << std::endl;
        std::cin >> buffer;

        sendto(sock, buffer, sizeof(buffer), MSG_CONFIRM,
               reinterpret_cast<const sockaddr *>(&addr), sizeof(addr));

        std::cout << std::endl;
        std::cout << "THE MESSAGE HAS BEEN SENT..." << std::endl;
        std::cout << std::endl;

        socklen_t server_address_len = sizeof(sockaddr_in);

        ssize_t recv_len = 0;
        recv_len = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                            reinterpret_cast<sockaddr *>(&addr), &server_address_len);

        if (recv_len > 0) {
            buffer[recv_len] = '\0';
            std::cout << "SERVER MESSAGE: " << buffer << std::endl;
        }

        std::cout << "SEND ANOTHER MESSAGE (Y/N)?" << std::endl;
        std::cin >> user_answer;

        if(user_answer == "N" || user_answer == "n"){
            exitFlag = false;

            #ifdef _WIN32
                closesocket(sock);
            #else
                close(sock);
            #endif
        }

    }

    return EXIT_SUCCESS;
}