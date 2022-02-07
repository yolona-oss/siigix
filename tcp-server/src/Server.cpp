#include <Server.hpp>

#include <string.h>

namespace siigix
{
namespace TCP {
    Server::Server(port_t port, std::string ip, int threads) :
        Server(default_data_hndl_fn, default_conn_hndl_fn, default_conn_hndl_fn,
                port, ip,
                threads)
    { }

    Server::Server(data_hndl_fn_t d_hndl_fn,
            connection_hndl_fn_t conn_hndl_fn,
            connection_hndl_fn_t disconn_hdnl_fn,
            port_t port, std::string ip, int threads) :
        _socket(port),
        _data_hndl_fn(d_hndl_fn),
        _conn_hndl_fn(conn_hndl_fn),
        _disconn_hndl_fn(disconn_hdnl_fn),
        _thread_pool(threads)
    { }

    Server::~Server()
    {
        if (_status == ServerStatus::running) {
            stop();
        }
    }

    int
    Server::start()
    {
        if (_status == ServerStatus::running) {
            stop();
        }

        /* int reuseaddr = 1; */

        /* _socket.SetOpts(SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)); */

        std::function<void()> hal = [this]() { handleAcceptLoop(); };
        std::function<void()> wdl = [this]() { waitDataLoop(); };
        _thread_pool.addJob(hal);
        _thread_pool.addJob(wdl);

        return _status;
    }

    void
    Server::stop()
    {
        _thread_pool.dropJobs();
        _status = ServerStatus::shutdowned;
        _socket.Close();
        _clients.clear();
    }

    void Server::joinLoop() {_thread_pool.join();}

    bool
    Server::connectClient(std::string ip, const port_t port, connection_hndl_fn_t connect_hndl) {
        try {
            std::unique_ptr<Client> client(new Client(std::move(ConnectSocket(ip, port))));
            connect_hndl(*client);
            _client_mutex.lock();
            _clients.emplace_back(std::move(client));
            _client_mutex.unlock();
        } catch (...) {
            std::exception_ptr p = std::current_exception();
            std::cerr <<(p ? p.__cxa_exception_type()->name() : "null") << std::endl;
            return false;
        }

        return true;
    }

    bool
    Server::disconnectClient(std::string ip, port_t port) {
        bool disconn = false;
        for (std::unique_ptr<Client>& client : _clients)
            if (client->getIP() == ip && client->getPort() == port) {
                client->disconnect();
                disconn = true;
            }
        return disconn;
    }

    void
    Server::disconnectAll() {
        for(std::unique_ptr<Client>& client : _clients) {
            /* client->disconnect(); */
        }
    }

    bool
    Server::sendToClient(std::string ip, port_t port, const IOBuff& data) {
        bool data_is_sended = false;
        for (std::unique_ptr<Client>& client : _clients)
            if (client->getIP() == ip && client->getPort() == port) {
                client->sendMessage(data);
                data_is_sended = true;
        }
        return data_is_sended;
    }

    void
    Server::sendToAll(const IOBuff& data) {
        for(std::unique_ptr<Client>& client : _clients)
            client->_protocol.sendMessage(client->_ip, data);
    }

    void
    Server::handleAcceptLoop() {
        if (_status == ServerStatus::running) {
            /* TODO AHTUNG!!! not clear memeory on error!!!!!!!!!!!!!! */
            std::unique_ptr<Client> client( new Client( std::move(_socket.Accept()) ) );
            _conn_hndl_fn(*client);
            _client_mutex.lock();
            _clients.emplace_back(std::move(client));
            _client_mutex.unlock();
        } else {
                //very big error
        }

        //loop
        if (_status == ServerStatus::running) {
            _thread_pool.addJob([this](){handleAcceptLoop();});
        }
    }

    void
    Server::waitDataLoop()
    {
        std::lock_guard lock(_client_mutex);
        IOBuff data;

        /* for all clients */
        for (auto it = _clients.begin(), end = _clients.end(); it != end; ++it)
        {
            auto& client = *it;

            if (client) /* client reciving data */
            {
                if (client->recvMessage(data)) {
                    _thread_pool.addJob([this, _data = std::move(data), &client]() {
                        client->_access_mutex.lock();
                        _data_hndl_fn(_data, *client);
                        client->_access_mutex.unlock();
                    });
                }
            }
            else if(client->_status == ClientStatus::disconnected) /* on client disconnected */
            {
                _thread_pool.addJob([this, &client, it] () {
                    client->_access_mutex.lock();
                    Client* pointer = client.release();
                    client = nullptr;
                    pointer->_access_mutex.unlock();
                    _disconn_hndl_fn(*pointer);
                    _clients.erase(it);
                    delete pointer;
               });
            }
        }

        //loop
        if (_status == ServerStatus::running) {
            _thread_pool.addJob([this](){waitDataLoop();});
        }
    }

} /* TCP */ 
} /* siigix */ 
