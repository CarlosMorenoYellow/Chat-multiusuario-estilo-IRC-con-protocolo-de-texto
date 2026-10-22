#include "server.hpp"
#include "../Commons/Protocol.hpp"
#include "../Commons/Protocol.cpp"

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
    clients.push_back({chat_space, ConnectType::CHAT, "", "", false, {}});
    fds.push_back({http_space, POLLIN, 0});
    clients.push_back({http_space, ConnectType::HTTP, "", "", false, {}});

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
                    clients.push_back({new_client, ConnectType::CHAT, "", "", false, {}});
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
                    clients.push_back({new_requester, ConnectType::HTTP, "", "", false, {}});
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
                    close(fds[i].fd);
                    fds.erase(fds.begin() + i);
                    clients.erase(clients.begin() + i);
                }
                else {
                    // Leamos lo que envio el cliente y guardemoslo en su buffer personal
                    clients[i].read_buffer.append(tmp_buffer, bytes);

                    // Si el stream se pasa del limite y es de un chat, veamos si al menos hay algo util adentro
                    if (clients[i].type == ConnectType::CHAT && clients[i].read_buffer.size() > protocol::MAXCLIENTLINE) {
                        size_t limit_check = clients[i].read_buffer.find("\r\n");

                        // Si no encontramos un indicador de fin de mensaje
                        if (limit_check == std::string::npos or limit_check > protocol::MAXCLIENTLINE) {
                            std::string oopsie_502 = "502 Linea demasiado larga\r\n";
                            send(fds[i].fd, oopsie_502.c_str(), oopsie_502.size(), 0);
                            clients[i].read_buffer.clear();
                            continue;
                        }
                    }
                    
                    // Lo mismo pero el stream vino de http
                    if (clients[i].type == ConnectType::HTTP and clients[i].read_buffer.size() > 8192) {
                        std::string http_oopsie_400 = "HTTP 400 Bad Request\n\rContent-Type: text/plain\n\rConnection: close\r\n\r\nMucho texto (mensaje muy largo)\n";
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
                            std::cout << "Commando recibido de " << fds[i].fd << ":" << pure_mesage << std::endl;
                        }
                        
                        // Entonces es de HTTP
                        else {
                            std::cout << "Request HTTP recibido de " << fds[i].fd << std::endl;

                            std::string dummy_ans = "HTTP 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 12\n\rConnection: close\r\n\r\nRecibi tu commando, no hare nada con el\n";
                            send(fds[i].fd, dummy_ans.c_str(), dummy_ans.size(), 0);
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