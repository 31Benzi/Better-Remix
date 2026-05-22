#ifndef EASYWSCLIENT_HPP_20120819_MIOFVASDTNUASZDQPLFD
#define EASYWSCLIENT_HPP_20120819_MIOFVASDTNUASZDQPLFD
#ifdef _WIN32
#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS // _CRT_SECURE_NO_WARNINGS for sscanf errors in MSVC2013 Express
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <fcntl.h>
#pragma comment(lib, "ws2_32")
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#ifndef _SSIZE_T_DEFINED
typedef int ssize_t;
#define _SSIZE_T_DEFINED
#endif
#ifndef _SOCKET_T_DEFINED
typedef SOCKET socket_t;
#define _SOCKET_T_DEFINED
#endif
#ifndef snprintf
#define snprintf _snprintf_s
#endif
#if _MSC_VER >= 1600
// vs2010 or later
#include <stdint.h>
#else
typedef __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#endif
#define socketerrno WSAGetLastError()
#define SOCKET_EAGAIN_EINPROGRESS WSAEINPROGRESS
#define SOCKET_EWOULDBLOCK WSAEWOULDBLOCK
#include <random>
#else
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#ifndef _SOCKET_T_DEFINED
typedef int socket_t;
#define _SOCKET_T_DEFINED
#endif
#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif
#ifndef SOCKET_ERROR
#define SOCKET_ERROR (-1)
#endif
#define closesocket(s) ::close(s)
#include <errno.h>
#define socketerrno errno
#define SOCKET_EAGAIN_EINPROGRESS EAGAIN
#define SOCKET_EWOULDBLOCK EWOULDBLOCK
#endif

// This code comes from:
// https://github.com/dhbaird/easywsclient
//
// To get the latest version:
// wget https://raw.github.com/dhbaird/easywsclient/master/easywsclient.hpp
// wget https://raw.github.com/dhbaird/easywsclient/master/easywsclient.cpp

#include <string>
#include <vector>

namespace easywsclient
{

    struct Callback_Imp
    {
        virtual void operator()(const std::string& message) = 0;
    };
    struct BytesCallback_Imp
    {
        virtual void operator()(const std::vector<uint8_t>& message) = 0;
    };

    class WebSocket
    {
    public:
        typedef enum readyStateValues
        {
            CLOSING,
            CLOSED,
            CONNECTING,
            OPEN
        } readyStateValues;
        struct wsheader_type
        {
            unsigned header_size;
            bool fin;
            bool mask;
            enum opcode_type
            {
                CONTINUATION = 0x0,
                TEXT_FRAME = 0x1,
                BINARY_FRAME = 0x2,
                CLOSE = 8,
                PING = 9,
                PONG = 0xa,
            } opcode;
            int N0;
            uint64_t N;
            uint8_t masking_key[4];
        };

        std::vector<uint8_t> rxbuf;
        std::vector<uint8_t> txbuf;
        std::vector<uint8_t> receivedData;

        socket_t sockfd;
        readyStateValues readyState;
        bool useMask;
        bool isRxBad;

        typedef WebSocket* pointer;

        // Factories:
        static pointer create_dummy();
        static pointer from_url(const std::string& url, const std::string& origin = std::string());
        static pointer from_url_no_mask(const std::string& url, const std::string& origin = std::string());

        // Interfaces:
        ~WebSocket() { };
        WebSocket(socket_t sockfd, bool useMask)
            : sockfd(sockfd)
            , readyState(OPEN)
            , useMask(useMask)
            , isRxBad(false) { };
        void poll(int timeout = 0); // timeout in milliseconds
        void send(const std::string& message);
        void sendBinary(const std::string& message);
        void sendBinary(const std::vector<uint8_t>& message);
        void sendPing();
        void close();
        readyStateValues getReadyState() const;

        template <class Callable>
        void dispatch(Callable callable)
        // For callbacks that accept a string argument.
        { // N.B. this is compatible with both C++11 lambdas, functors and C function pointers
            struct _Callback : public Callback_Imp
            {
                Callable& callable;
                _Callback(Callable& callable)
                    : callable(callable)
                {
                }
                void operator()(const std::string& message)
                {
                    callable(message);
                }
            };
            _Callback callback(callable);
            _dispatch(callback);
        }

        template <class Callable>
        void dispatchBinary(Callable callable)
        // For callbacks that accept a std::vector<uint8_t> argument.
        { // N.B. this is compatible with both C++11 lambdas, functors and C function pointers
            struct _Callback : public BytesCallback_Imp
            {
                Callable& callable;
                _Callback(Callable& callable)
                    : callable(callable)
                {
                }
                void operator()(const std::vector<uint8_t>& message)
                {
                    callable(message);
                }
            };
            _Callback callback(callable);
            _dispatchBinary(callback);
        }

    protected:
        void _dispatch(Callback_Imp& callable);
        void _dispatchBinary(BytesCallback_Imp& callable);
        template <class Iterator>
        void sendData(wsheader_type::opcode_type type, uint64_t message_size, Iterator message_begin, Iterator message_end);
    };

} // namespace easywsclient

#endif /* EASYWSCLIENT_HPP_20120819_MIOFVASDTNUASZDQPLFD */