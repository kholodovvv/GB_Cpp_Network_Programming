#include <iostream>
#include <curl/curl.h>
#include <string>

CURL* curl;
static std::string buffer;

static int Response_Writer(char* data, size_t size, size_t nmember, std::string *buffer){
    int result = 0;

    if(buffer != NULL){

        buffer->append(data, size * nmember);
        result = size * nmember;
    }

    return result;
}

bool Client_Autorization_Request(/*CURL* curl,*/ std::string address){
    std::string login, password;
    CURLcode result;

    do{
        std::cout << "Enter your username and password:" << std::endl;
        std::cout << "login: ";
        std::cin >> login;
        std::cout << "password: ";
        std::cin >> password;
        std::cout << std::endl;
    }while(login.length() < 4 || password.length() < 4);

    address.append("?login=");
    address.append(login);
    address.append("&pass=");
    address.append(password);

    curl_easy_setopt(curl, CURLOPT_URL, address.c_str());
    struct curl_slist* headers{0};
    curl_slist_append(headers, "Accept: text/plain");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Response_Writer);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);

    result = curl_easy_perform(curl);
    if(result != CURLE_OK) return false;

    return true;
}

bool Send_Message(CURL* curl, std::string address){
    std::string message;

    do{
        std::cout << "Enter your message: ";
        std::cin >> message;
    }while(message.length() < 1);

    address.append(message.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, address.c_str());
    struct curl_slist* headers{0};
    curl_slist_append(headers, "Accept: text/plain");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode result = curl_easy_perform(curl);
    if(result != CURLE_OK) return false;

    return true;
}

int main(int argc, char** argv) {
    std::string address_server, answer_user;

    if(argc < 2){
        std::cout << "For the program to work, you must enter the server address and port!";
        EXIT_FAILURE;
    }

    //получаем handle curl (easy - простой режим)
    curl = curl_easy_init();

    address_server.append("http://");
    address_server.append(argv[1]);
    address_server.append(":");
    address_server.append(argv[2]);
    address_server.append("/");

    if(curl){

    //авторизация клиента
    if(Client_Autorization_Request(address_server)){
        if(buffer == "Ok"){
            //отправка сообщения серверу
            address_server.append("message?body=");
            std::cout << std::endl;

            Send_Message(curl, address_server) ? std::cout << "\r\n" << "The message was sent successfully!" << std::endl
                                                    : std::cerr << "Error sending the message!" << std::endl;
        }else{
            std::cerr << "\r\n" << "Authorization failed with an error!" << std::endl;
            EXIT_FAILURE;
        }

    }else{
        std::cerr << "\r\n" << "Error sending the request!" << std::endl;
        EXIT_FAILURE;
    }

        curl_easy_cleanup(curl);
    }

do{
    std::cout << "To exit, enter 'exit': ";
    std::cin >> answer_user;
    std::cout << std::endl;
}while(answer_user != "exit");

curl_global_cleanup();

    return 0;
}
