#include "server.hpp"
#include "../Commons/Protocol.hpp"
#include "../Commons/Protocol.cpp"

std::map<std::string, std::deque<MessageHistory>> global_history;

// Fucion para obtener hora
std::string get_time() {
    std::time_t now = std::time(nullptr);
    std::tm* info = std::localtime(&now);
    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(2) << info->tm_hour << ":"
        << std::setw(2) << info->tm_min << ":"
        << std::setw(2) << info->tm_sec;
    return oss.str();
}

// Funcion para eliminar la presencia de un usuario
void remover(
    int client_fd,
    const std::vector<std::string>& channel_names,
    std::map<std::string, std::vector<int>>& channels) {
    for (const auto& channel_name : channel_names) {
        auto channel = channels.find(channel_name);
        if (channel == channels.end()) {
            continue;
        }

        auto& members = channel->second;
        members.erase(std::remove(members.begin(), members.end(), client_fd), members.end());
        if (members.empty()) {
            channels.erase(channel);
        }
    }
}

void process_request(size_t index, std::vector<pollfd>& fds, std::vector<ClientInfo>& clients, const std::string& request, std::map<std::string, std::vector<int>>& channels) {
    int client_fd = fds[index].fd;
    size_t first_space = request.find(" ");
    if(first_space == std::string::npos) {
        std::string resp = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Length: 16\r\nConnection: close\r\n\r\nMalformed Request";
        send(client_fd, resp.c_str(), resp.size(), 0);
        return;
    }

    std::string method = request.substr(0, first_space);
    size_t second_space = request.find(" ", first_space + 1);
    if (second_space == std::string::npos) {
        std::string resp = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Length: 16\r\nConnection: close\r\n\r\nMalformed Request";
        send(client_fd, resp.c_str(), resp.size(), 0);
        return;
    }

    std::string path = request.substr(first_space + 1, second_space - (first_space + 1));
    if (method != "GET") {
        std::string resp = "HTTP/1.1 405 Method Not Allowed\r\nAllow: GET\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Length: 18\r\nConnection: close\r\n\r\nMethod Not Allowed";
        send(client_fd, resp.c_str(), resp.size(), 0);
        return;
    }

    std::string response_buffer = "";
    bool found = false;

    if (path == "/usuarios") {
        found = true;
        for (const auto& client: clients) {
            if (client.type == ConnectType::CHAT and !client.nickname.empty() and client.verifiedName) {
                response_buffer += client.nickname + "\n";
            }
        }
    }
    else if (path == "/canales") {
        found = true;
        for (const auto& tup: channels) {
            response_buffer += tup.first + " " + std::to_string(tup.second.size()) + "\n";
        }
    }
    else if (path.rfind("/historial/", 0) == 0) {
        std::string target_channel = '#' + path.substr(11);
        if (channels.find(target_channel) == channels.end()) {
            std::string resp = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Length: 17\r\nConnection: close\r\n\r\nNo Such Channel";
            send(client_fd, resp.c_str(), resp.size(), 0);
            return;
        }

        found = true;
        auto it = global_history.find(target_channel);
        if (it != global_history.end()) {
            for (const auto& msg: it->second) {
                response_buffer += "[" + msg.time + "]" + msg.nick + ": " + msg.text + "\n";
            }
        }
    }

    if (!found) {
        std::string resp = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Length: 9\r\nConnection: close\r\n\r\nNot Found";
        send(client_fd, resp.c_str(), resp.size(), 0);
    }
    else {
        std::string headers = 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain; charset=utf-8\r\n"
            "Content-Length: " + std::to_string(response_buffer.size()) + "\r\n"
            "Connection: close\r\n"
            "\r\n";
        
        std::string answer = headers + response_buffer;
        send(client_fd, answer.c_str(), answer.size(), 0);
    }
}

// Funcion para leer y ejecutar comandos
void process_command(size_t index, std::vector<pollfd>& fds, std::vector<ClientInfo>& clients, const std::string message, std::map<std::string, std::vector<int>>& channels){
    std::string cmd = message;
    std::string args = "";
    size_t gap = message.find(" ");

    if (gap != std::string::npos) {
        cmd = message.substr(0, gap);
        args = message.substr(gap + 1);
    }

    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

    int client_fd = fds[index].fd;
    ClientInfo& current_client = clients[index];

    if (!current_client.verifiedName and cmd != "NICK" and cmd != "QUIT") {
        std::string oopsie_530 = "530 No te has gesitrado con NICK\r\n";
        send(client_fd, oopsie_530.c_str(), oopsie_530.size(), 0);
        return;
    }

    if (cmd == "NICK") {
        if (args.empty()) {
            std::string oopsie_501 = "501 Sintaxis invalida, debes escribir un nombre\r\n";
            send(client_fd, oopsie_501.c_str(), oopsie_501.size(), 0);
            return;
        }

        if (!(protocol::validarApodo(args))) {
            std::string oopsie_501 = "501 Sintaxis invalida, el apodo no cumple con las restricciones\r\n";
            send(client_fd, oopsie_501.c_str(), oopsie_501.size(), 0);
            return;
        }

        for (size_t i = 0; i < clients.size(); ++i) {
            if(i != index and clients[i].nickname == args) {
                std::string oopsie_433 = "433 Ese nombre esta ocupado en este momento\r\n";
                send(client_fd, oopsie_433.c_str(), oopsie_433.size(), 0);
                return;
            }
        }

        current_client.nickname = args;
        current_client.verifiedName = true;

        std::string goodie_201 = "201 Bienvenido(a), " + args + "\r\n";
        send(client_fd, goodie_201.c_str(), goodie_201.size(), 0);
        std::cout << client_fd << " se ha logueado con el apodo " << args << std::endl;
    }
    else if (cmd == "QUIT") {
        std::string goodie_299 = "299 Bye bye!\r\n";
        send(client_fd, goodie_299.c_str(), goodie_299.size(), 0);
        std::cout << client_fd << " se esta largando con QUIT" << std::endl;

        for (const auto& channel: current_client.channels) {
            auto& members = channels[channel];
            std::string quit_message = "302 " + channel + " " + current_client.nickname + " cerro sesion\r\n";
            for(int member_fd: members) {
                if (member_fd != client_fd) {
                    send(member_fd, quit_message.c_str(), quit_message.size(), 0);
                }
            }
        }

        remover(client_fd, current_client.channels, channels);
        close(client_fd);
        fds.erase(fds.begin() + index);
        clients.erase(clients.begin() + index);
    }
    else if (cmd == "JOIN") {
        if (args.empty() or !protocol::validarCanal(args)) {
            std::string oopsie_501 = "501 Sntaxis de canal invalida\r\n";
            send(client_fd, oopsie_501.c_str(), oopsie_501.size(), 0);
            return;
        }

        auto& members = channels[args];

        if (std::find(members.begin(), members.end(), client_fd) == members.end()) {
            std::string join_message = "302 " + args + " " + current_client.nickname + " se ha unido!\r\n";
            for (int member_fd: members) {
                send(member_fd, join_message.c_str(), join_message.size(), 0);
            }

            members.push_back(client_fd);
            current_client.channels.push_back(args);
        }

        std::string goodie_210 = "210 Te uniste a " + args + "\r\n";
        send(client_fd, goodie_210.c_str(), goodie_210.size(), 0);
        std::cout << current_client.nickname << " se a unido al canal" << args << std::endl;
    }
    else if (cmd == "PART") {
        if (args.empty()) {
            std::string oopsie_501 = "501 No se especifico canal\r\n";
            send(client_fd, oopsie_501.c_str(), oopsie_501.size(), 0);
            return;
        }
        if (channels.find(args) == channels.end()) {
            std::string oopsie_551 = "551 Ese canal no existe\r\n";
            send(client_fd, oopsie_551.c_str(), oopsie_551.size(), 0);
            return;
        }
        auto& members = channels[args];
        auto it = std::find(members.begin(), members.end(), client_fd);

        if (it == members.end()) {
            std::string oopsie_442 = "442 No eres miembro del canal\r\n";
            send(client_fd, oopsie_442.c_str(), oopsie_442.size(), 0);
            return;
        }

        std::string exit_message = "302 " + args + " " + current_client.nickname + " ha salido del chat\r\n";
        for (int member_fd: members) {
            if (member_fd != client_fd) {
                send(member_fd, exit_message.c_str(), exit_message.size(), 0);
            }
        }

        members.erase(it);
        
        auto cl_it = std::find(current_client.channels.begin(), current_client.channels.end(), args);
        if (cl_it != current_client.channels.end()) {
            current_client.channels.erase(cl_it);
        }

        if (members.empty()) {
            channels.erase(args);
        }

        std::string goodie_211 = "211 Salida del canal exitosa\r\n";
        send(client_fd, goodie_211.c_str(), goodie_211.size(), 0);
    }
    else if (cmd == "MSG") { 
        size_t space = args.find(" ");
        if (args.empty() or space == std::string::npos) {
            std::string oopsie_501 = "501 Sintaxis invalida. Debes colocar el canal que deseas mensajear y el mensaje\r\n";
            send(client_fd, oopsie_501.c_str(), oopsie_501.size(), 0);
            return;
        }

        std::string target_channel = args.substr(0, space);
        std::string message = args.substr(space + 1);

        if (channels.find(target_channel) == channels.end()) {
            std::string oopsie_551 = "551 Ese canal no existe\r\n";
            send(client_fd, oopsie_551.c_str(), oopsie_551.size(), 0);
            return;
        }

        const auto& members = channels[target_channel];
        if(std::find(members.begin(), members.end(), client_fd) == members.end()) {
            std::string oopsie_442 = "442 No eres miembro de este canal\r\n";
            send(client_fd, oopsie_442.c_str(), oopsie_442.size(), 0);
            return;
        }

        std::string goodie_250 = "250 OK!\r\n";
        send(client_fd, goodie_250.c_str(), goodie_250.size(), 0);

        MessageHistory new_reg{
            get_time(),
            current_client.nickname,
            message
        };
        auto& queue = global_history[target_channel];
        queue.push_back(new_reg);
        if (queue.size() > 50) {
            queue.pop_front();
        }

        std::string worldwidenoise = "300 " + target_channel + " " + current_client.nickname + ": " + message + "\r\n";
        for (int member_fd: members) {
            if (member_fd != client_fd) {
                send(member_fd, worldwidenoise.c_str(), worldwidenoise.size(), 0);
            }
        }
    }
    else if (cmd == "WHO") {
        if(args.empty() or !protocol::validarCanal(args)) {
            std::string oopsie_501 = "501 Sintaxis invalida. Debe haber un canal valido como argumento.\r\n";
            send(client_fd, oopsie_501.c_str(), oopsie_501.size(), 0);
            return;
        }

        if (channels.find(args) == channels.end()) {
            std::string oopsie_551 = "551 Ese canal no existe\r\n";
            send(client_fd, oopsie_551.c_str(), oopsie_551.size(), 0);
            return;
        }

        const auto& member_fds = channels[args];
        std::string response_buffer = "";
        for (int m_fd: member_fds) {
            std::string user_nick = "dummy";
            for (const auto& client: clients) {
                if (client.fd == m_fd) {
                    user_nick = client.nickname;
                    break;
                }
            }

            response_buffer += "213-" + user_nick + "\r\n";
        }

        response_buffer += "213 " + std::to_string(member_fds.size()) + " miembro(s)\r\n";
        send(client_fd, response_buffer.c_str(), response_buffer.size(), 0);
    }

    else if (cmd == "CHANS") {
        std::string response_buffer = "";
        size_t total_channels = channels.size();
        for (const auto& tup: channels) {
            std::string chan_name = tup.first;
            size_t total_members = tup.second.size();
            response_buffer += "212- " + chan_name + " " + std::to_string(total_members) + "\r\n";
        }

        response_buffer += "212 " + std::to_string(total_channels) + " canales\r\n";
        send(client_fd, response_buffer.c_str(), response_buffer.size(), 0);
    }
    
    else {
        std::string oopsie_500 = "500 Comando desconocido\r\n";
        send(client_fd, oopsie_500.c_str(), oopsie_500.size(), 0);
    }
}

// Funcion para abrir sockets
int socket_plocket(int port){
    // Preparemos recursos para abrir socket
    int socket_space = socket(AF_INET, SOCK_STREAM, 0); 
    if (socket_space < 0){
        std::cerr << "Error al obtener recursos para puerto" << port << std::endl;
        return -1;
    }

    // Anclemos el socket al puerto forzado, haciendo que nos permita usarlo incluso si esta en downtime
    int opt = 1; // 1 por convenciones de C
    if (setsockopt(socket_space, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
        std::cerr << "Error al anclar puerto" << port << std::endl;
        close(socket_space);
        return -1;
    }

    // Configuremos el socket
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Veamos si levanta
    if (bind(socket_space, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Socket no levanto en el puerto " << port << std::endl;
        close(socket_space);
        return -1; 
    }

    if (listen(socket_space, 300) < 0) {
        std::cerr << "Error al escuchar en el puerto" << port << std::endl;
        close(socket_space);
        return -1;
    }

    std::cout << "Tenemos socket en el puerto " << port << std::endl;
    return socket_space;
}

int main() {
    int chat_port = 5000;
    int http_port = 8080;
    int chat_space = socket_plocket(chat_port);
    int http_space = socket_plocket(http_port);

    if (chat_space < 0 or http_space < 0){
        std::cout << "Cerrando, adios..." << std::endl;
        return 1;
    }

    // Logramos crear nuestros sockets, guardemoslos
    std::vector<pollfd> fds;
    std::vector<ClientInfo> clients;
    fds.push_back({chat_space, POLLIN, 0});
    clients.push_back({chat_space, ConnectType::CHAT, "", "", false, {}, false});
    fds.push_back({http_space, POLLIN, 0});
    clients.push_back({http_space, ConnectType::HTTP, "", "", false, {}, false});

    // Tambien creemos los canales
    std::map<std::string, std::vector<int>> channels;

    std::cout << "Sockets listos, esperando conecciones..." << std::endl;

    while (true){

        // Vemos que ha ocurrido con los fds
        int activity = poll(fds.data(), fds.size(), -1);

        if (activity < 0) {
            std::cerr << "Ocurrio un error con el poll, saliendo..." << std::endl;
            break;
        }

        // Iteremos los eventos recividos
        for (size_t i = fds.size(); i-- > 0; ) {

            // Si no ha ocurrido nada en esta posicion
            if (!(fds[i].revents & POLLIN)){
                continue;
            }
            
            // Si ocurrio algo donde escuchamos el chat (cliente nuevo)
            if (fds[i].fd == chat_space) {
                int new_client = accept(chat_space, nullptr, nullptr);
                if (new_client >= 0) {
                    fds.push_back({new_client, POLLIN, 0});
                    clients.push_back({new_client, ConnectType::CHAT, "", "", false, {}, false});
                    std::cout << "Nuevo cliente añadido por chat " << new_client << std::endl;
                    std::string greet = "200 chat de prueba listo\r\n";
                    send(new_client, greet.c_str(), greet.size(), 0);
                }
            }

            // Si ocurrio algo donde escuchamos por http (request de status)
            else if (fds[i].fd == http_space) {
                int new_requester = accept(http_space, nullptr, nullptr);
                if (new_requester >= 0) {
                    fds.push_back({new_requester, POLLIN, 0});
                    clients.push_back({new_requester, ConnectType::HTTP, "", "", false, {}, false});
                    std::cout << "Nueva peticion por http con fd " << new_requester << std::endl; 
                }
            }

            // Tenemos datos de algun cliente
            else {
                char tmp_buffer[1024];
                ssize_t bytes = recv(fds[i].fd, tmp_buffer, sizeof(tmp_buffer)-1, 0);

                // Si encontramos un error o no hay informacion, perdimos el cliente
                if (bytes <= 0) {
                    std::cout << "Cliente con FD " << fds[i].fd << " desconectado" << std::endl;

                    for (const auto& channel: clients[i].channels) {
                        auto& members = channels[channel];
                        std::string disccon_message = "302 " + channel + " " + clients[i].nickname + " ha perdido conexion\r\n";
                        for (int member_fd: members) {
                            if (member_fd != fds[i].fd) {
                                send(member_fd, disccon_message.c_str(), disccon_message.size(), 0);
                            }
                        }
                    }

                    remover(fds[i].fd, clients[i].channels, channels);
                    close(fds[i].fd);
                    fds.erase(fds.begin() + i);
                    clients.erase(clients.begin() + i);
                }
                else {
                    // Revisemos si hubo un exeso de datos
                    if (clients[i].baddie_bad) {
                        std::string trash(tmp_buffer, bytes);
                        size_t recovery_point = trash.find("\r\n");

                        if (recovery_point != std::string::npos){
                            clients[i].baddie_bad = false;
                            clients[i].read_buffer = trash.substr(recovery_point + 2);
                        }
                        else {
                            clients[i].read_buffer.clear();                            
                        }
                        continue;
                    }

                    // Leamos lo que envio el cliente y guardemoslo en su buffer personal
                    clients[i].read_buffer.append(tmp_buffer, bytes);

                    // Si el stream se pasa del limite y es de un chat, veamos si al menos hay algo util adentro
                    if (clients[i].type == ConnectType::CHAT && clients[i].read_buffer.size() > protocol::MAXCLIENTLINE) {
                        size_t limit_check = clients[i].read_buffer.find("\r\n");

                        // Si no encontramos un indicador de fin de mensaje
                        if (limit_check == std::string::npos or limit_check > protocol::MAXCLIENTLINE) {
                            std::string oopsie_502 = "502 Linea demasiado larga\r\n";
                            send(fds[i].fd, oopsie_502.c_str(), oopsie_502.size(), 0);
                            clients[i].baddie_bad = true;
                            clients[i].read_buffer.clear();
                            continue;
                        }
                    }
                    
                    // Lo mismo pero el stream vino de http
                    if (clients[i].type == ConnectType::HTTP and clients[i].read_buffer.size() > 8192) {
                        std::string http_oopsie_400 = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nMucho texto (mensaje muy largo)\n";
                        send(fds[i].fd, http_oopsie_400.c_str(), http_oopsie_400.size(), 0);
                        close(fds[i].fd);
                        fds.erase(fds.begin() + i);
                        clients.erase(clients.begin() + i);
                        continue;
                    }

                    // Hay algo util, veamos
                    size_t pos;
                    std::string ender = (clients[i].type == ConnectType::CHAT) ? "\r\n" : "\r\n\r\n";
                    while ((pos = clients[i].read_buffer.find(ender)) != std::string::npos) {
                        std::string pure_mesage = clients[i].read_buffer.substr(0, pos);
                        clients[i].read_buffer.erase(0, pos + ender.length());

                        // Es un mensaje de CHAT?
                        if (clients[i].type == ConnectType::CHAT) {
                            int client_fd = fds[i].fd;
                            process_command(i, fds, clients, pure_mesage, channels);
                            if (i >= fds.size() || fds[i].fd != client_fd) {
                                break;
                            }
                            std::cout << "Mensaje recibido de " << client_fd << ":" << pure_mesage << std::endl;
                        }
                        
                        // Entonces es de HTTP
                        else {
                            std::cout << "Request HTTP recibido de " << fds[i].fd << std::endl;
                            process_request(i, fds, clients, pure_mesage, channels);
                            close(fds[i].fd);
                            fds.erase(fds.begin() + i);
                            clients.erase(clients.begin() + i);
                            break;
                        }
                    }
                }
            }
        }
    }

    close(chat_space);
    close(http_space);
    return 0;
}