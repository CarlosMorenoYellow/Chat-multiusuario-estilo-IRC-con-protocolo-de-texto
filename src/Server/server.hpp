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
#include <deque>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

//constantes utiles

enum class ConnectType {
    CHAT,
    HTTP
};

//contenedor para historial de mensajes
struct MessageHistory {
    std::string time;
    std::string nick;
    std::string text;
};

//contenedor de estado de cliente
struct ClientInfo {
    int fd;
    ConnectType type;
    std::string read_buffer;
    std::string nickname;
    bool verifiedName = false;
    std::vector<std::string> channels;
    bool baddie_bad = false;
    std::vector<std::string> session_messages;
    std::vector<bool> deletion;
};

//espacio para declarar funciones
int socket_plocket(int port);
#endif