#ifndef SOCKET_HPP_LEMCELXN
#define SOCKET_HPP_LEMCELXN

#include <stdexcept>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip6.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <map>
#include <string>
#include <iostream>
#include <initializer_list>

#include "BitFlag.hpp"
/* #include "INData.hpp" */
#include "Utils.hpp"

#define MAX_CONN 10

namespace siigix {
    typedef __uint16_t port_t;
    typedef __uint32_t ip4_t;
    typedef in6_addr   ip6_t;

    typedef int ka_prop_t;
    struct KeepAliveConfig{
        ka_prop_t ka_idle  = 120;
        ka_prop_t ka_intvl = 3;
        ka_prop_t ka_cnt   = 5;
    };

    typedef struct sock_opt sock_opt_t;
    struct sock_opt {
        int level;
        int value;
        int opt;
        socklen_t optlen;

        sock_opt(int level, int opt, int value, int optlen) :
            level(level), opt(opt), value(value), optlen(optlen) {  }
        sock_opt() :
            sock_opt(0, 0, 0, 0) { }
    };

    typedef struct sock_opts sock_opts_t;
    struct sock_opts {
        int count;
        sock_opt_t *opts;

        sock_opt_t& operator[](int i) {
            if (i >= 0 && i < count) {
                return opts[i];
            }
            throw std::domain_error(buildErrorMessage("AHTUNG! Assign error. Assign to opts[] inside sock_opts not in range. passed value: ", i, ". Assignable range 0..", count-1));
        }

        sock_opts(int l_count) : count(l_count) {
            opts = new sock_opt_t [count];
        }
        sock_opts(const std::initializer_list<sock_opt_t>& opts_list) :
            sock_opts(opts_list.size())
        {
            int i = 0;
            for (auto& opt: opts_list) {
                /* opts[0] = opt; */
                memcpy(&opts[i], &opt, sizeof(opt));
                i++;
            }
        }

        ~sock_opts() {
            delete[] opts;
        }
    };

    static const int USE_OPT_FLAG = true;
    static const int NOT_USE_OPT_FLAG = false;

    /* return count of founded ips and ip+interfacename map */
    int getLocalIP(std::map<std::string, in_addr_t>& ret);
    bool operator==(struct sockaddr_in, struct sockaddr_in);
    bool operator==(struct sockaddr_in6 a, struct sockaddr_in6 b);

    static const int INVALID_SOCK = -1;

    /**********************************************************************
    *                              BaseSocket                               *
    **********************************************************************/

    class BaseSocket {
        public:
            //no copible
            BaseSocket(const BaseSocket& other) = delete;

            void Close();

            BaseSocket(BaseSocket&& move)            noexcept;
            BaseSocket& operator=(BaseSocket&& move) noexcept;
            void swap(BaseSocket& other)             noexcept;

            bool GetOpts(int level, int option, void *value, socklen_t *opt_len) const;
            bool SetOpts(int level, int option, const int *value, socklen_t opt_len);

            bool EnableKeepAlive();
            void SetKeepAlive(KeepAliveConfig ka) { _ka_conf = ka; }

            bool isValid() const { return ((_fd != INVALID_SOCK) ? true : false); };

            virtual ~BaseSocket();

            virtual std::string getIP() const   { return _ip; }
            virtual port_t      getPort() const { return _port; }

        protected:
            BaseSocket(int fd);

            int getSocketFD() const { return _fd; }

            void getSockName(struct sockaddr *addr, socklen_t *len) const;
            void getPeerName(struct sockaddr *addr, socklen_t *len) const;

            void setIP(void* addr, socklen_t len);
            void setPort(void* addr, socklen_t len);

        private:
            int         _fd;
            std::string _ip;
            port_t      _port;
            KeepAliveConfig _ka_conf;

    };

    /**********************************************************************
    *                             TransSocket                             *
    **********************************************************************/

    /* send/recive data to socket */
    class TransSocket : public BaseSocket {
        public:
            TransSocket(int fd);

            template<typename Fn>
            size_t recvMessage(char *data, size_t size, Fn scanForEnd = [](size_t) { return false; });
            void   sendMessage(const char* data, size_t size);
            void   sendMessageClose();
    };


    /**********************************************************************
    *                           ConnectSocket                            *
    **********************************************************************/

    /* Connect to remote host
     * Avaible to send/recive data */
    class ConnectSocket : public TransSocket {
        public:
            ConnectSocket(std::string ip, port_t port,
                    struct addrinfo hints = { .ai_family = AF_INET, .ai_socktype = SOCK_STREAM, .ai_protocol = 0 });

            std::string getIP() const   { return _ip; }
            port_t      getPort() const { return _port; }

        private:
            std::string _ip;
            port_t      _port;
    };

    /**********************************************************************
    *                            ListenSocket                            *
    **********************************************************************/

    /* listen for connection */
    class ListenSocket : public BaseSocket {
        public:
            ListenSocket(std::string host, port_t port, struct addrinfo hints, sock_opts_t opts, int max_conn = MAX_CONN);
            ListenSocket(std::string host, port_t port, struct addrinfo hints, int max_conn = MAX_CONN);

            TransSocket Accept(int flags = 0);
    };

    #include "Socket.inl"

} /* siigix */ 

#endif /* end of include guard: SOCKET_HPP_LEMCELXN */
