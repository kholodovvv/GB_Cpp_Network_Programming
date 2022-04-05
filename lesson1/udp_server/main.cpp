#ifdef _WIN32
#pragma comment (lib,"Ws2_32.lib")
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <iostream>
#include <stdio.h>
#include <cstring>
#include <stdlib.h>
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


int main(int argc, char const *argv[])
{

    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " <port>" << std::endl;
        return EXIT_FAILURE;
    }


    #ifdef _WIN32
    
    WSADATA wsaData; //Создание структуры типа WSADATA, в которую автоматически, в момент создания, загружаются данные о версии сокетов

    int error = WSAStartup(MAKEWORD(2, 0), &wsaData); //Вызов функции создания сокетов WSAStartup, 1-м параметром передаётся запрашиваемая версия сокетов

    if (error == SOCKET_ERROR)
    {
        std::cerr << "FAILED TO CREATE SOCKET!" << std::endl;
        return EXIT_FAILURE;
    }

    const int port{ std::atoi(argv[1]) };

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); //Создаём сокет

        if(sock == INVALID_SOCKET) {
            std::cerr << "FAILED TO CREATE SOCKET!" << std::endl;
            return EXIT_FAILURE;
        }else {
            std::cout << "Starting echo server on the port " << port << "...\n";
        }
    #else
    const int port{ std::stoi(argv[1]) };
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); //Создаём сокет

        if(sock == -1) {
            std::cerr << "FAILED TO CREATE SOCKET!" << std::endl;
            return EXIT_FAILURE;
        }else {
            std::cout << "Starting echo server on the port " << port << "...\n";
        }
    #endif

    #ifdef _WIN32
        sockaddr_in addr;
        addr.sin_family = PF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.S_un.S_addr = INADDR_ANY;
    #else
    sockaddr_in addr =
            {
                    .sin_family = PF_INET,
                    .sin_port = htons(port),
                    .sin_addr = {.s_addr = INADDR_ANY}
            };
    #endif

    if (bind(sock, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) != 0)
    {
        std::cerr << "BIND ERROR!" << std::endl;
        // Socket will be closed in the Socket destructor.
        #ifdef _WIN32
            closesocket(sock);
        #else
            close(sock);
        #endif
        return EXIT_FAILURE;
    }

    char buffer[256];
    char hostBuffer[NI_MAXHOST] = "No Name";
    bool exitCommand = false;

    // socket address used to store client address
    struct sockaddr_in client_address = {0};
    socklen_t client_address_len = sizeof(sockaddr_in);
    int recv_len = 0;

    std::cout << "Running echo server...\n" << std::endl;

    while (exitCommand != true)
    {
        // Read content into buffer from an incoming client.
        recv_len = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                            reinterpret_cast<sockaddr *>(&client_address),
                            &client_address_len);

        //Реализация обратного резолва
        if(getnameinfo(reinterpret_cast<const sockaddr*>(&client_address), client_address_len,
                    hostBuffer, sizeof(hostBuffer),nullptr, 0, NI_NAMEREQD)) {
            std::cout << "Hostname not available..." << std::endl;
        }

        if (recv_len > 0)
        {
            buffer[recv_len] = '\0';

            std::cout
                    << "Client with address "
                    << inet_ntoa(client_address.sin_addr)
                    << ":" << client_address.sin_port
                    << " Client name: "
                    <<  hostBuffer
                    << " sent datagram "
                    << "[length = "
                    << recv_len
                    << "]:\n'''\n"
                    << buffer
                    << "\n'''"
                    << std::endl;

            // Send same content back to the client ("echo").
            sendto(sock, buffer, recv_len, 0, reinterpret_cast<const sockaddr *>(&client_address),
                   client_address_len);
        }

        std::cout << std::endl;

        //Если клиент отправил команду "exit"
        if(std::strcmp(buffer, "exit") == 0){
            exitCommand = true;
            #ifdef _WIN32
                closesocket(sock);
            #else
                close(sock);
            #endif
        }

    }

    return EXIT_SUCCESS;
}
