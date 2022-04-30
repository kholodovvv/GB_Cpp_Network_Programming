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
	#include <filesystem>
	#include <assert.h>
	#include <fstream>
	#include <array>
	#include <optional>
	#include <list>
	#include <future>
#else
	#include <algorithm>
	#include <cassert>
	#include <filesystem>
	#include <fstream>
	#include <future>
	#include <iostream>
	#include <list>
	#include <optional>
	#include <stdexcept>
	#include <string>
	#include <thread>
	#include <vector>
	#include <netdb.h>
	#include <arpa/inet.h>
    #include <unistd.h>
	#include <cerrno>

    #include <boost/system/error_code.hpp>
    #include <boost/asio.hpp>
#endif


#if !defined(_WIN32)
extern "C"
{
#   include <signal.h>
}
#else
#   include <cwctype>
#endif


#if !defined(MAX_PATH)
#   define MAX_PATH (256)
#endif

using boost::asio::ip::tcp;
const auto clients_count = 10;
const auto buffer_size = 4096;
using namespace std::literals;
namespace fs = std::filesystem;

#if defined(_WIN32)
const wchar_t separ = fs::path::preferred_separator;
#else
const wchar_t separ = *reinterpret_cast<const wchar_t*>(&fs::path::preferred_separator);
#endif

std::unique_ptr<addrinfo, decltype(&freeaddrinfo)>
#ifdef _WIN32
get_serv_info(const char* port, const char* hostname)
#else
get_serv_info(const char* port)
#endif
{
	struct addrinfo hints;

	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo *s_i;
    int ai_status;

#ifdef _WIN32
	memset(&hints, 0, sizeof(hints)); //Edition
	if ((ai_status = getaddrinfo(hostname, port, &hints, &s_i)) != 0)
#else
	if ((ai_status = getaddrinfo(nullptr, port, &hints, &s_i)) != 0)
#endif
    {
        std::cerr << "getaddrinfo error " << gai_strerror(ai_status) << std::endl;
        return std::unique_ptr<addrinfo, decltype(&freeaddrinfo)>(nullptr, freeaddrinfo);
    }

    return std::unique_ptr<addrinfo, decltype(&freeaddrinfo)>(s_i, freeaddrinfo);
}


void set_reuse_addr(int &sock)
{
    const int flag = 1;

    // Allow reuse of port.
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&flag), sizeof(flag)) < 0)
    {
        throw std::logic_error("Set SO_REUSEADDR error");
    }
}


int accept_client(int &server_sock)
{
    struct sockaddr_storage client_addr;
    socklen_t client_addr_length = sizeof(client_addr);
    std::array<char, INET_ADDRSTRLEN> addr;

    int client_sock(accept(server_sock, reinterpret_cast<sockaddr*>(&client_addr), &client_addr_length));

    if (!client_sock)
    {
        throw std::logic_error("Accepting client");
    }

    assert(sizeof(sockaddr_in) == client_addr_length);

    std::cout <<
        "Client from " << inet_ntop(AF_INET, &(reinterpret_cast<const sockaddr_in * const>(&client_addr)->sin_addr), &addr[0], addr.size())
        << "..."
        << std::endl;
    return client_sock;
}


class Transceiver
{
public:
    Transceiver(int &&client_sock) : client_sock_(std::move(client_sock)) {}
    Transceiver(const Transceiver&) = delete;
    Transceiver() = delete;

public:
    const int &ts_socket() const { return client_sock_; }

public:
    bool send_buffer(const std::vector<char> &buffer)
    {
        size_t transmit_bytes_count = 0;
        const auto size = buffer.size();

        while (transmit_bytes_count != size)
        {
            auto result = send(client_sock_, &(buffer.data()[0]) + transmit_bytes_count, size - transmit_bytes_count, 0);
            if (-1 == result)
            {
                if (need_to_repeat()) continue;
                return false;
            }

            transmit_bytes_count += result;
        }

        return true;
    }

    bool send_file(fs::path const& file_path)
    {
        std::vector<char> buffer(buffer_size);
        std::ifstream file_stream(file_path, std::ifstream::binary);

        if (!file_stream) return false;

        std::cout << "Sending file " << file_path << "..." << std::endl;

		file_stream.seekg(0, std::ios::end);
		uintmax_t fsize = file_stream.tellg();

        //Если размер буфера меньше размера файла, то передаём файл по частям
        if (fsize > buffer_size) {

            file_stream.seekg(0, std::ios::beg);

            while (!file_stream.eof()) {

                  file_stream.read(&buffer[0], buffer.size());

                  if (!send_buffer(buffer)) {
                      return false;
				  }
				  else {
					  std::cout << "Sent " << file_stream.gcount() << " byte" << std::endl;
					  buffer.clear();
					  buffer.resize(buffer_size);
				  }
             }
			file_stream.close();
        }
        else {

            while (file_stream)
            {
                file_stream.read(&buffer[0], buffer.size());
                if (!send_buffer(buffer)) return false;
            }

			file_stream.close();

        }

        return true;
    }

    std::string get_request()
    {
        std::array<char, MAX_PATH + 1> buffer;
        size_t recv_bytes = 0;
        const auto size = buffer.size() - 1;

        std::cout << "Reading user request..." << std::endl;
        while (true)
        {
            auto result = recv(client_sock_, &buffer[recv_bytes], size - recv_bytes, 0);

            if (!result) break;

            if (-1 == result)
            {
                if (need_to_repeat()) continue;
                throw std::logic_error("Socket reading error");
            }

            auto fragment_begin = buffer.begin() + recv_bytes;
            auto ret_iter = std::find_if(fragment_begin, fragment_begin + result,
                                         [](char sym) { return '\n' == sym || '\r' == sym;  });
            if (ret_iter != buffer.end())
            {
                *ret_iter = '\0';
                recv_bytes += std::distance(fragment_begin, ret_iter);
                break;
            }
            recv_bytes += result;
            if (size == recv_bytes) break;
        }

        buffer[recv_bytes] = '\0';

        auto result = std::string(buffer.begin(), buffer.begin() + recv_bytes);
        std::cout << "Request = \"" << result << "\"" << std::endl;

        return result;
    }

private:
    static bool need_to_repeat()
    {
        switch (errno)
        {
            case EINTR:
            case EAGAIN:
            // case EWOULDBLOCK: // EWOULDBLOCK == EINTR.
                return true;
        }

        return false;
    };

private:
    int client_sock_;
};


class Client
{
public:
    Client(int &&sock) :
        tsr_(std::move(sock))
    {
        std::cout
            << "Client ["<< static_cast<int>(tsr_.ts_socket()) << "] "
            << "was created..."
            << std::endl;
    }

    std::optional<fs::path> recv_file_path()
    {
        auto request_data = tsr_.get_request();
        if (!request_data.size()) return std::nullopt;

        auto cur_path = fs::current_path().wstring();
        auto file_path = fs::weakly_canonical(request_data).wstring();

#if defined(_WIN32)
        std::transform(cur_path.begin(), cur_path.end(), cur_path.begin(),
            [](wchar_t c) { return std::towlower(c); }
        );
        std::transform(file_path.begin(), file_path.end(), file_path.begin(),
            [](wchar_t c) { return std::towlower(c); }
        );
#endif
        if (file_path.find(cur_path) == 0)
        {
            file_path = file_path.substr(cur_path.length());
        }

        return fs::weakly_canonical(cur_path + separ + file_path);
    }

    bool send_file(const fs::path &file_path)
    {
        if (!(fs::exists(file_path) && fs::is_regular_file(file_path))) return false;

        return tsr_.send_file(file_path);
    }

    bool process()
    {
        auto file_to_send = recv_file_path();
        bool result = false;

        if (std::nullopt != file_to_send)
        {
            std::cout << "Trying to send " << *file_to_send << "..." << std::endl;
            if (send_file(*file_to_send))
            {
                std::cout << "File was sent." << std::endl;
            }
            else
            {
                std::cerr << "File sending error!" << std::endl;
            }
            result = true;
        }

        return result;
    }

private:
    Transceiver tsr_;
    fs::path file_path_;
};


int main(int argc, const char * const argv[])
{
    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " <port>" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Usage: " << argv[0] << " <port>" << std::endl;

#if !defined(_WIN32)
    signal(SIGPIPE, SIG_IGN);
#endif

	int server_sock;
	int client_sock;

    try
    {

	#ifdef _WIN32
		// Initialize Winsock
		WSADATA wsaData;
		int status = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (status != NO_ERROR) {
			std::cerr << "FAILED TO INITIALIZE SOCKET!" << std::endl;
			return EXIT_FAILURE;
		}

        std::cout << "ENTER HOSTNAME OR IP TO THE SERVER: ";
        std::cin >> hostname[];
		const char hostname[] = "192.168.0.36";

		auto servinfo = get_serv_info(argv[0], hostname);
    #else

        auto servinfo = get_serv_info(argv[1]);
	#endif
		
        if (!servinfo)
        {
            std::cerr << "Can't get servinfo!" << std::endl;
            exit(EXIT_FAILURE);
        }
//---------Create Server Socket
        //auto server_sock = {servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol};

		#ifdef _WIN32
			// Create a SOCKET object
				server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (server_sock == -1) {
				std::cerr << "FAILED TO CREATE SOCKET!" << std::endl;
				return EXIT_FAILURE;
			}
		#else
			server_sock = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
			if (!server_sock)
			{
				throw std::logic_error("Socket creation error");
			}
		#endif

//---------------
        set_reuse_addr(server_sock);

        if (bind(server_sock, servinfo->ai_addr, servinfo->ai_addrlen) < 0)
        {
		#ifdef _WIN32
			closesocket(server_sock);
        #else

			close(server_sock);
		#endif
            throw std::logic_error("Bind error");
        }

        std::list<std::future<bool>> pending_tasks;

        std::cout
            << "Listening on port " << argv[1] << "...\n"
            << "Server path: " << fs::current_path()
            << std::endl;

        if (listen(server_sock, clients_count) < 0)
        {
            throw std::logic_error("Listen error");
        }

        std::cout << "Listen was run..." << std::endl;

        while (true)
        {
            client_sock = accept_client(server_sock);

            if (!client_sock)
            {
                throw std::logic_error("Client socket error");
            }

            pending_tasks.push_back(std::async(std::launch::async, [&](int &&sock)
            {
                Client client(std::move(sock));
                std::cout << "Client tid = " << std::this_thread::get_id() << std::endl;
                auto result = client.process();
                std::cout
                    << "Client with tid = " << std::this_thread::get_id()
                    << " exiting..."
                    << std::endl;

                return result;
            }, std::move(client_sock)));

            std::cout << "Cleaning tasks..." << std::endl;
            for (auto task = pending_tasks.begin(); task != pending_tasks.end();)
            {
                if (std::future_status::ready == task->wait_for(1ms))
                {
                    auto fu = task++;
                    std::cout
                        << "Request completed with a result = " << fu->get() << "...\n"
                        << "Removing from list." << std::endl;
                    pending_tasks.erase(fu);
                }
                else ++task;
            }
        }

		#ifdef _WIN32
			closesocket(client_sock);
		#else
			close(client_sock);
		#endif
    }
    catch (const std::logic_error &e)
    {
        std::cerr
            << e.what()
            << " [" << "MESSAGE ERROR" << "]!"
            << std::endl;
        return EXIT_FAILURE;
    }

	#ifdef _WIN32
		closesocket(server_sock);
	#else
		close(server_sock);
	#endif
	
    return EXIT_SUCCESS;
}

