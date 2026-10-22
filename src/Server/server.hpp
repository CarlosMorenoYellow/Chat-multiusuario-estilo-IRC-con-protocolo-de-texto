#ifndef SERVER_HPP
#define SERVE_HPP

//librerias
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <unistd.h>
#include <cstring>

//constantes utiles
const int MAX_CHAT_SIZE = 512;

enum class ConnectType {
    CHAT,
    HTTP
};

//contenedor de estado de cliente
struct ClientInfo {
    int fd;
    ConnectType type;
    std::string read_buffer;
    std::string nickname;
    bool verifiedName = false;
    std::vector<std::string> channels;
};

//espacio para declarar funciones
int socket_plocket(int port);
#endif