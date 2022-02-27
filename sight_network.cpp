#include "sight_network.h"
#include "sight.h"
#include "sight_log.h"

#include <cassert>
#include <cstdlib>
#include <sstream>
#include <string>
#include <uv.h>
#include <atomic>

static sight::SightNetServer netServer;

static std::atomic_int connectIdx = 1000;

namespace sight {

    // Base on: https://github.com/delaemon/libuv-tutorial/blob/eb9bdf73fa71fb19f5d539b7ab96bc2f3a87de80/tcp-echo-server.c

    void allocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
        buf->base = (char*)malloc(suggested_size);
        buf->len = suggested_size;
    }

    void OnDisconnect(uv_stream_t* clientStream) {
        logDebug("onDisconnect");

        uv_tcp_t* client = (uv_tcp_t*)clientStream;
        for (auto iter = netServer.clients.begin(); iter != netServer.clients.end(); iter++) {
            if (iter->client == nullptr || iter->client == client) {
                netServer.clients.erase(iter);
            }
        }
        
    }

    void OnMsg(short command, const char* buffer, int size) {
        SightNetBaseMsg baseMsg(buffer, size);

        
    }

    void onRead(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf) {
        if (nread < 0) {
            if (nread != UV_EOF) {
                OnDisconnect(client);
                uv_close((uv_handle_t*)client, nullptr);
            }
        } else {
            // buffer data
            auto netClient = netServer.findClient(client);
            assert(netClient != nullptr);
            logDebug("$0, $1",nread, buf->len);
            netClient->buffer->write(buf->base, nread);

        }

        if (buf && buf->base) {
            free(buf->base);
        }
    }

    void onNewConnection(uv_stream_t* server, int status) {
        if (status < 0) {
            fprintf(stderr, "New connection error %s\n", uv_strerror(status));
            return;
        }

        uv_tcp_t* client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
        uv_tcp_init(netServer.uvLoop, client);
        if (uv_accept(server, (uv_stream_t*)client) == 0) {
            netServer.clients.emplace_back(client, connectIdx++);
            uv_read_start((uv_stream_t*)client, allocBuffer, onRead);
        } else {
            uv_close((uv_handle_t*)client, NULL);
        }
    }

    int initNetworkServer(ushort port) {
        netServer.clients.reserve(8);

        netServer.uvLoop = (uv_loop_t*)malloc(sizeof(uv_loop_t));
        uv_loop_init(netServer.uvLoop);

        netServer.uvTcp = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
        auto tcpPointer = netServer.uvTcp;
        uv_tcp_init(netServer.uvLoop, tcpPointer);
        uv_tcp_nodelay(tcpPointer, 1);
        uv_tcp_keepalive(tcpPointer, 1, 0);

        struct sockaddr_in addr;
        uv_ip4_addr("0.0.0.0", port, &addr);
        uv_tcp_bind(tcpPointer, (const struct sockaddr*)&addr, 0);

        int r = uv_listen((uv_stream_t*)tcpPointer, 128, onNewConnection);
        if (r) {
            fprintf(stderr, "Listen error %s\n", uv_strerror(r));
            return CODE_FAIL;
        }

        return CODE_OK;
    }
    
    void runNetworkThread() {
        uv_run(netServer.uvLoop, UV_RUN_DEFAULT);
    }

    SightNetClient::SightNetClient(uv_tcp_t* client, int index)
        : client(client),
          index(index)
    {
        uv_fileno((const uv_handle_t*)client, &fd);
        buffer = new std::stringstream();
    }

    SightNetClient::~SightNetClient()
    {
        logDebug(index);
        if (buffer) {
            delete buffer;
            buffer = nullptr;
        }
    }

    void SightNetClient::checkMsg() {
        auto& bufferStream = *buffer;
        bufferStream.seekg(0, std::ios::end);
        int bufferLen = bufferStream.tellg();
        bufferStream.seekg(0, std::ios::beg);

        if (pkgLen <= 0) {
            // 2 byte length
            if (bufferLen < 2) {
                return;
            }

            if (!bufferStream.read(reinterpret_cast<char*>(&pkgLen), 2)) {
                //
                logDebug("read pkgLen failed");
                return;
            }

            bufferLen -= 2;
        }
        
        if (pkgLen > bufferLen) {
            // need more data
            return;
        }

        // 2 byte command
        short command = 0;
        if (!bufferStream.read(reinterpret_cast<char*>(&command), 2)) {
            //
            logDebug("read command failed");
            return;
        }

        bufferLen -= 2;
        char buffer[bufferLen];
        auto readSize = bufferStream.readsome(buffer, bufferLen);
        assert(readSize == bufferLen);
        OnMsg(command, buffer, readSize);
    }

    SightNetClient* SightNetServer::findClient(uv_tcp_t* client) {
        if (!client) {
            return nullptr;
        }

        for(auto& item: clients){
            if (item.client == client) {
                return &item;
            }
        }
        return nullptr;
    }

    SightNetClient* SightNetServer::findClient(uv_stream_t* client) {
        return findClient((uv_tcp_t*)client);
    }

    int SightNetBaseMsg::readInt() {
        int i;
        stream >> i;
        return i;
    }

    short SightNetBaseMsg::readShort() {
        short s;
        stream >> s;
        return s;
    }

    std::string SightNetBaseMsg::readString() {
        auto size = readShort();
        char buf[size];
        stream.readsome(buf, size);
        return std::string(buf, size);
    }

    SightNetBaseMsg::SightNetBaseMsg(const char* buffer, int size)
    {
        stream.clear();
        stream.write(buffer, size);
    }



}