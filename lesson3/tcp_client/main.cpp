
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
	#include <vector>
	#include <thread>
#else
	#include <chrono>
	#include <iostream>
	#include <string>
    #include <cstring>
	#include <thread>
	#include <vector>
	#include <sys/socket.h>
    #include <sys/ioctl.h>
	#include <netinet/in.h>
    #include <netinet/tcp.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <unistd.h>

	using namespace std::chrono_literals;
#endif

const auto MAX_RECV_BUFFER_SIZE = 256;
std::string pathfile = "/home/sa/";

#ifdef _WIN32
bool send_request(SOCKET& sock, const std::string& request)
#else
bool send_request(int &sock, const std::string &request)
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

#ifdef _WIN32
std::pair<bool, std::string> RecvFile(SOCKET& sock)
#else
std::pair<bool, std::string> RecvFile(int& sock)
#endif
{
	std::string filename;
	std::pair<bool, std::string> return_result;
	
	while (true)
	{
		char buffer[MAX_RECV_BUFFER_SIZE];
		auto recv_bytes = recv(sock, buffer, sizeof(buffer), 0);

		std::cout
			<< recv_bytes
			<< " was received..."
			<< std::endl;

		if (recv_bytes > 0)
		{
			buffer[recv_bytes] = '\n';
			if (int i = std::string(buffer).find("FILE NOT FOUND!") != std::string::npos) {
				return_result.first = false;
				return_result.second = "FILE NOT FOUND!";

				return return_result;
			}

			srand(time(NULL));
			filename = pathfile + "in_" + std::to_string(rand() % (1000)) + ".txt";
			
			FILE* file;
			file = fopen(filename.c_str(), "wb");

			
			fwrite(buffer, 1, recv_bytes, file);
			fclose(file);

			return_result.first = true;
			return_result.second = filename;

			break;
		}
		else if (-1 == recv_bytes)
		{
			if (EINTR == errno) continue;
			return_result.first = false;
			return_result.second = "FILE ACCEPTANCE ERROR!";
			if (0 == errno) return return_result;
			 //std::cerr << errno << "FILE ACCEPTANCE ERROR!" << std::endl;
			//break;
			 
		}
		
		break;
	}

	return return_result;
}

int main()
{
    std::string str_user_answer;
    size_t int_user_answer;
    std::string addr_server, port_server;
#ifdef _WIN32
	SOCKET sock;
#else
	int sock;
#endif

    std::cout << "CONNECT TO THE SERVER BY (1 - HOSTNAME, 2 - IPv4, 3 - IPv6):" << std::endl;

    do{
        std::cout << "ENTER: ";
        std::cin >> str_user_answer;
        std::cout << std::endl;
		#ifdef _WIN32
			int_user_answer = atoi(str_user_answer.c_str());
		#else
			int_user_answer = stoi(str_user_answer);
		#endif
    }while(int_user_answer == 0 || int_user_answer > 3);

    do {
        std::cout << "ENTER THE SERVER PORT: ";
        std::cin >> port_server;
        std::cout << std::endl;
    }while(port_server.length() < 1);

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(std::stoi(port_server));

	struct sockaddr_in6 server_addr6;

    if(int_user_answer == 1)
    {
        do {
            std::cout << "ENTER THE HOSTNAME: ";
                std::cin >> addr_server;
            std::cout << std::endl;
        }while(addr_server.length() < 1);
		
        const struct hostent *remote_host { gethostbyname(addr_server.c_str()) };
		#ifdef _WIN32
			server_addr.sin_addr.s_addr = *reinterpret_cast<const uint32_t*>(remote_host->h_addr);
		#else
			server_addr.sin_addr.s_addr = *reinterpret_cast<const in_addr_t*>(remote_host->h_addr);
		#endif
    }
    else if(int_user_answer == 2)
    {
        do {
            std::cout << "ENTER THE IPv4: ";
                std::cin >> addr_server;
            std::cout << std::endl;
        }while(addr_server.length() < 7);

        server_addr.sin_addr.s_addr = inet_addr(addr_server.c_str());
    }
    else if(int_user_answer == 3)
    {
        do {
            std::cout << "ENTER THE IPv6: ";
                std::cin >> addr_server;
            std::cout << std::endl;
        }while(addr_server.length() < 7);

		server_addr6.sin6_family = AF_INET6;
		server_addr6.sin6_port = htons(std::stoi(port_server));

        inet_pton(AF_INET6, addr_server.c_str(), &(server_addr6.sin6_addr));

    }


	if (int_user_answer < 3)
	{
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

		if (connect(sock, reinterpret_cast<const sockaddr* const>(&server_addr), sizeof(server_addr)) != 0)
		{
			std::cerr << "CONNECTION ERROR!" << std::endl;
			#ifdef _WIN32
				closesocket(sock);
			#else
				close(sock);
			#endif
			return EXIT_FAILURE;
		}
	}
	else 
	{
		#ifdef _WIN32
		// Initialize Winsock
			WSADATA wsaData;
			int status = WSAStartup(MAKEWORD(2, 2), &wsaData);
			if (status != NO_ERROR) {
				std::cerr << "FAILED TO CREATE SOCKET!" << std::endl;
				return EXIT_FAILURE;
			}else {
		// Create a SOCKET object
				sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
				if (sock == INVALID_SOCKET) {
					std::cerr << "FAILED TO CREATE SOCKET!" << std::endl;
					return EXIT_FAILURE;
				}
			}
		#else
			sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
			if (!sock)
			{
				std::cerr << "FAILED TO CREATE SOCKET!" << std::endl;
				return EXIT_FAILURE;
			}
		#endif

			if (connect(sock, reinterpret_cast<const sockaddr* const>(&server_addr6), sizeof(server_addr6)) != 0)
			{
				std::cerr << "CONNECTION ERROR!" << std::endl;
				#ifdef _WIN32
					closesocket(sock);
				#else
					close(sock);
				#endif
				return EXIT_FAILURE;
			}

	}


    std::string request;
    std::vector<char> buffer;
    buffer.resize(MAX_RECV_BUFFER_SIZE);

    std::cout << "Connected to \"" << addr_server << "\"..." << std::endl;

    
 //Put the socket in non-blocking mode:
// If iMode = 0, blocking is enabled; 
// If iMode != 0, non-blocking mode is enabled.
	#ifdef _WIN32
		u_long iMode = 1;
		if (ioctlsocket(sock, FIONBIO, &iMode) != NO_ERROR)
	#else
		const int flag = 1;
		if (ioctl(sock, FIONBIO, const_cast<int*>(&flag)) < 0)
	#endif
		{
			std::cerr << "ERROR SWITCHING THE SOCKET TO NON-BLOCKING MODE!" << std::endl;
			#ifdef _WIN32
				closesocket(sock);
			#else
				close(sock);
			#endif
			return EXIT_FAILURE;
		}
	
    // Disable Naggles's algorithm.
#ifdef _WIN32
	if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&iMode), sizeof(iMode)) < 0)
#else
    if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char *>(&flag), sizeof(flag)) < 0)
#endif
    {
        std::cerr << "ERROR DISABLING THE MODE NAGGLES'S" << std::endl;
		#ifdef _WIN32
			closesocket(sock);
		#else
			close(sock);
		#endif
        return EXIT_FAILURE;
    }

    std::cout << "Waiting for the user input..." << std::endl;

	while (true)
	{
		std::cout << "> " << std::flush;
		if (!std::getline(std::cin, request)) break;

		std::cout
			<< "Sending request: \"" << request << "\"..."
			<< std::endl;
		request = "getfile ./home/sa/test"; //!!!
		request += "\r\n";

		if (!send_request(sock, request))
		{
			std::cerr << "ERROR SENDING A REQUEST TO THE SERVER!" << std::endl;
			#ifdef _WIN32
				closesocket(sock);
			#else
				close(sock);
			#endif
			return EXIT_FAILURE;
		}


		if (int res = request.find("getfile") != std::string::npos) {
			
			#ifdef _WIN32
				std::this_thread::sleep_for(std::chrono::seconds(5));
			#else
				std::this_thread::sleep_for(5s);
			#endif
				std::pair<bool, std::string> result = RecvFile(sock);
				if (result.first) {
					std::cout << "THE FILE IS WRITTEN WITH THE NAME '" << result.second << "'" << std::endl;
				}
				else {
					std::cout << result.second << "'" << std::endl;
				}
		}
		else 
		{
			std::cout
				<< "Request was sent, reading response..."
				<< std::endl;
			#ifdef _WIN32
				std::this_thread::sleep_for(std::chrono::seconds(5));
			#else
				std::this_thread::sleep_for(5s);
			#endif

			while (true)
			{
				auto recv_bytes = recv(sock, buffer.data(), buffer.size() - 1, 0);

				std::cout
					<< recv_bytes
					<< " was received..."
					<< std::endl;

				if (recv_bytes > 0)
				{
					buffer[recv_bytes] = '\0';
					std::cout << "------------\n" << std::string(buffer.begin(), std::next(buffer.begin(), recv_bytes)) << std::endl;
					continue;
				}
				else if (-1 == recv_bytes)
				{
					if (EINTR == errno) continue;
					if (0 == errno) break;
					// std::cerr << errno << ": " << sock_wrap.get_last_error_string() << std::endl;
					break;
				}

				break;
			}

		}
	}
	#ifdef _WIN32
		closesocket(sock);
	#else
		close(sock);
	#endif

    return EXIT_SUCCESS;
}

