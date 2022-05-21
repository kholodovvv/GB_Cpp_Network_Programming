#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>
#include <httpserver.hpp>

class message_responce: public httpserver::http_resource{
public:
    const std::shared_ptr<httpserver::http_response> render(const httpserver::http_request& mess_req){
        std::cout << "Message from the client:" << std::endl;
        std::cout << mess_req.get_arg("body") << std::endl;
        return std::shared_ptr<httpserver::http_response>(new httpserver::string_response("GET: Ok", 200, "text/plain"));
    }
};

class auth_resource: public httpserver::http_resource {
public:

    std::vector<std::pair<std::string, std::string>> load_users(){
        std::ifstream in("./users");
        std::vector<std::pair<std::string, std::string>> vec;
        std::string line;
        std::pair<std::string, std::string> user;

        if(!in.is_open()) {
            std::cout << "Failed to load user information" << std::endl;
            return vec;
        }

        while(getline(in, line)){
            std::stringstream(line) >> user.first >> user.second;
            vec.push_back(user);
        }

        return vec;
    }

    const std::shared_ptr<httpserver::http_response> render(const httpserver::http_request& req) {
        std::vector<std::pair<std::string, std::string>> users = load_users();

        if(users.size() >= 0){
            for(auto it = users.begin(); it != users.end(); it++){
                if(it->first == req.get_arg("login") || it->second == req.get_arg("password")){
                    return std::shared_ptr<httpserver::string_response>(new httpserver::string_response("Ok", 200, "text/plain"));
                }
            }
            return std::shared_ptr<httpserver::http_response>(new httpserver::string_response("Not found", 404, "text/plain"));
        }
    }
};

const std::shared_ptr<httpserver::http_response> not_found(const httpserver::http_request& req) {
    return std::shared_ptr<httpserver::string_response>(new httpserver::string_response("Not found", 404, "text/plain"));
}


int main(int argc, char** argv) {

    if (argc < 1){
        std::cout << "Port number not specified!" << std::endl;
        EXIT_FAILURE;
    }

    httpserver::webserver ws = httpserver::create_webserver(atoi(argv[1]))
    .no_ssl()
    .not_found_resource(not_found);

    //Регистрация запроса для вывода сообщения на стороне сервера
    message_responce message_res;
    ws.register_resource("/message", &message_res);

    //Регистрация запроса на авторизацию
    auth_resource auth_res;
    ws.register_resource("/", &auth_res);

    ws.start(true);

    return 0;
}
