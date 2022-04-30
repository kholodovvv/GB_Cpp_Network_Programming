#include <ctime>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <pthread.h>
#include <list>
#include <boost/system/error_code.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;
const int echo_port = 8888;

std::string make_daytime_string()
{
    using namespace std; // time_t, time и ctime;
    time_t now = time(0);
    return ctime(&now);
}


// Указатель shared_ptr и enable_shared_from_this нужны для того,
// чтобы сохранить объект tcp_connection до завершения выполнения операции.
class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
public:
    typedef std::shared_ptr<TcpConnection> pointer;

    static pointer create(boost::asio::io_context& io_context)
    {
        return pointer(new TcpConnection(io_context));
    }

    tcp::socket& socket()
    {
        return socket_;
    }

    // В методе start(), вызывается asio::async_write(), отправляющий данные клиенту.
    // Здесь используется asio::async_write(), вместо ip::tcp::socket::async_write_some(), чтобы весь блок данных был гарантированно отправлен.
    bool start()
    {
        // The data to be sent is stored in the class member message_ as we need to keep the data valid until the asynchronous operation is complete.
        message_ = make_daytime_string();
        auto sthrow  = shared_from_this();

        std::cout << "Sending data to the client tid = " << std::this_thread::get_id() << std::endl;

        // Здесь вместо boost::bind используется std::bind, чтобы уменьшить число зависимостей от Boost.
        // Он не работает с плейсхолдерами из Boost.
        // В комментариях указаны альтернативные плейсхолдеры.
        boost::asio::async_write(socket_, boost::asio::buffer(message_),
            // handle_write() выполнит обработку запроса клиента.
            [sthrow] (const boost::system::error_code& error, size_t bytes_transferred)
            {
                sthrow->handle_write(error, bytes_transferred);
            }
    );
        return true;
    }

private:
    TcpConnection(boost::asio::io_context& io_context)
        : socket_(io_context)
    {
    }

    void handle_write(const boost::system::error_code& /*error*/, size_t bytes_transferred)
    {
		std::cout << "Bytes transferred: " << bytes_transferred << std::endl;
    }

private:
    tcp::socket socket_;
    std::string message_;
};


class TcpServer
{
public:
    // В конструкторе инициализируется акцептор, начинается прослушивание TCP порта.
    TcpServer(boost::asio::io_context& io_context) :
        io_context_(io_context),
        acceptor_(io_context, tcp::endpoint(tcp::v4(), echo_port))
    {
        start_accept();
    }

private:
    // Метод start_accept() создаёт сокет и выполняет асинхронный `accept()`, при соединении.
    void start_accept()
    {
        TcpConnection::pointer new_connection = TcpConnection::create(io_context_);

        acceptor_.async_accept(new_connection->socket(),
            [this, new_connection] (const boost::system::error_code& error)
            {
                this->handle_accept(new_connection, error);
            }
        );

    }

    // Метод handle_accept() вызывается, когда асинхронный accept, инициированный в start_accept() завершается.
    // Она выполняет обработку запроса клиента и запуск нового акцептора.
    void handle_accept(TcpConnection::pointer new_connection, const boost::system::error_code& error)
    {
        if (!error)
        {
           //new_connection->start();
           //packaged_task используем для запуска задачи в отдельном потоке
          std::packaged_task<bool()> task([this, new_connection]{

              return new_connection->start();
          });

          std::thread t(std::move(task)); //создаём новый поток
          t.join(); // Вызов join блокирует вызывающий поток, до тех пор пока не выполниться задача

        }

        start_accept();
    }

private:
    boost::asio::io_context& io_context_;
    tcp::acceptor acceptor_;
    std::list<std::future<bool>> pending_tasks;
};


int main()
{
    try
    {
	    // io_context предоставляет службы ввода-вывода, которые будет использовать сервер, такие как сокеты.
        boost::asio::io_context io_context;
        TcpServer server(io_context);

        // Запуск асинхронных операций.
        io_context.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

    return EXIT_SUCCESS;
}

