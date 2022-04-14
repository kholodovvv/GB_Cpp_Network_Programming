#ifdef _WIN32
	#pragma comment (lib,"Ws2_32.lib")
	#include <winsock2.h>
	#include <windows.h>
	#include <ws2tcpip.h>
	#include <iostream>
	#include <stdio.h>
	#include <cstring>
	#include <string>
	#include <stdlib.h>
#else
    #include <cstdlib>
    #include <chrono>
    #include <iomanip>
    #include <iostream>
    #include <string>
    #include <cstring>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

#ifdef _WIN32
bool send_request(SOCKET& sock, const std::string& request)
#else
bool send_request(int& sock, const std::string& request)
#endif
{
    size_t bytes_count = 0;
    size_t req_pos = 0;
    auto const req_buffer = &(request.c_str()[0]);
    auto const req_length = request.length();

    while (true)
    {
        if ((bytes_count = send(sock, req_buffer + req_pos, req_length - req_pos, 0)) < 0)
        {
            if (EINTR == errno) continue;
        }
        else
        {
            if (!bytes_count) break;

            req_pos += bytes_count;

            if (req_pos >= req_length)
            {
                break;
            }
        }
    }

    return true;
}


const size_t buffer_size = 256;

int main(int argc, char const * const argv[])
{
#ifdef __linux__
	using namespace std::chrono_literals;
	int sock;
	int newsockfd;
#else
	SOCKET sock;
	SOCKET newsockfd;
#endif
   
/*    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " <port>" << std::endl;
        return EXIT_FAILURE;
    }

#ifdef _WIN32
    const int port { std::atoi(argv[1]) };
#else
    const int port { std::stoi(argv[1]) };
#endif
*/
    int port = 8888;
    std::cout << "Receiving messages on the port " << port << "...\n";

    struct sockaddr_storage client_addr = {0};
    socklen_t client_len = sizeof(client_addr);

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

#ifdef _WIN32
	// Initialize Winsock
	WSADATA wsaData;
	int status = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (status != NO_ERROR) {
		std::cerr << "FAILED TO INITIALIZE SOCKET!" << std::endl;
		return EXIT_FAILURE;
	}
	else {
		// Create a SOCKET object
		sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (sock == INVALID_SOCKET) {
			std::cerr << "FAILED TO CREATE SOCKET!" << std::endl;
			return EXIT_FAILURE;
		}
	}
#else
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (!sock)
	{
		std::cerr << "FAILED TO CREATE SOCKET!" << std::endl;
		return EXIT_FAILURE;
	}
#endif

    int broadcast = 1;
    setsockopt(sock, IPPROTO_TCP, SO_REUSEADDR, reinterpret_cast<const char*>(&broadcast), sizeof(broadcast));

    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0)
    {
        std::cerr << "BIND ERROR!" << std::endl;
		#ifdef _WIN32
			closesocket(sock);
		#else
			close(sock);
		#endif
        return EXIT_FAILURE;
    }

    char buffer[buffer_size] = {};

    listen(sock, 50);

    while (true)
    {
        newsockfd = accept(sock, (sockaddr*)&client_addr, &client_len);

        if(newsockfd < 0){
            throw std::runtime_error("ERROR ON ACCEPT!");
        }

        auto recv_bytes = recv(newsockfd, buffer, sizeof(buffer) - 1, 0);

        if(recv_bytes > 0){
            std::cout << recv_bytes << " was received..." << std::endl;
            buffer[recv_bytes] = '\0';
        }else{
            std::cerr << "ERROR RECEIVING THE MESSAGE!" << std::endl;
        }
 
        //Если клиент отправил команду для получения файла по указанному им пути [getfile путь_до_файла]
        size_t path_len;
        char fpath[60];

        if (int i = std::string(buffer).find("getfile") != std::string::npos) {
            path_len = recv_bytes - (i + 10);
            strcpy(fpath, &buffer[i + 8]);
            fpath[path_len] = '\0';

            FILE* file;
            file = fopen(fpath, "r");
            if (file != 0) {
                fgets(buffer, sizeof(buffer) - 1, file);
				fclose(file);
            }
            else {
                std::string(buffer) = "FILE NOT FOUND!\r\n";
                if(!send_request(newsockfd, buffer))
                    std::cerr << "ERROR SENDING THE MESSAGE!" << std::endl;
            }

            if (!send_request(newsockfd, buffer))
                std::cerr << "ERROR SENDING THE FILE!" << std::endl;
        }
        else {
            std::cout << buffer << std::endl;
        }

		continue;
    }

#ifdef _WIN32
	closesocket(newsockfd);
	closesocket(sock);
#else
	close(newsockfd);
	close(sock);
#endif
}
