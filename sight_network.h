//
// Created by Aincvy(aincvy@gmail.com) on 2022/01/20.

#pragma once

#include <uv.h>
#include <vector>
#include <sstream>

namespace sight {

    constexpr int NET_CMD_CHECK_VERSION = 1000;

    constexpr int NET_CMD_DEBUG_START = 2000;
    constexpr int NET_CMD_DEBUG_END = 2001;


    
    struct SightNetBaseMsg {

        std::stringstream stream;

        int readInt();
        short readShort();
        std::string readString();

        SightNetBaseMsg(const char* buffer, int size);
    };
    
    struct SightNetClient {
        uv_tcp_t* client = nullptr;
        int index = -1;     // -1 means not init.
        uv_os_fd_t fd;

        std::stringstream bufferStream;
        short pkgLen = 0;

        SightNetClient(uv_tcp_t* client, int index);
        ~SightNetClient();

        /**
         * @brief Check is there has a msg?
         * If has, then handle it
         */
        void checkMsg();

    };

    struct SightNetServer{
        uv_tcp_t* uvTcp;
        uv_loop_t* uvLoop = nullptr;
        std::vector<SightNetClient> clients;

        /**
         * @brief 
         * 
         * @param client 
         * @return SightNetClient* tmp result.
         */
        SightNetClient* findClient(uv_tcp_t* client);
        SightNetClient* findClient(uv_stream_t* client);
    };



    int initNetworkServer(ushort port);

    /**
     * @brief will block thread.
     * 
     */
    void runNetworkThread();

}