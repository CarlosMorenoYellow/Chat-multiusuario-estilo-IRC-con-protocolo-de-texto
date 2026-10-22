#include "server.hpp"

// Funcion para abrir sockets
int socket_plocket(int port){
    // Preparemos recursos para abrir socket
    int socket_space = socket(AF_INET, SOCK_STREAM, 0); 
    if (socket_space < 0){
        std::cerr << "Error al obtener recursos para puerto" << port << std::endl;
        return -1;
    }

    // Anclemos el socket al puerto forzado
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

    if (chat_space < 0 or http_space < 1){
        std::cout << "Cerrando, adios..." << std::endl;
        return 1;
    }

    // Logramos rear nuestros sockets, guardemoslos
    std::vector<pollfd> fds;
    fds.push_back({chat_space, POLLIN, 0});
    fds.push_back({http_space, POLLIN, 0});

    std::cout << "Sockets listos, esperando conecciones..." << std::endl;

    while (true){
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
                    std::cout << "Nuevo cliente añadido por chat " << new_client << std::endl;
                    std::string greet = "200 chat de prueba listo\r\n";
                    send(new_client, greet.c_str(), greet.size(), 0);
                }
            }

            // Si ocurrio algo donde escuchamos por http (reuest de status)
            else if (fds[i].fd == http_space) {
                int new_requester = accept(http_space, nullptr, nullptr);
                if (new_requester >= 0) {
                    fds.push_back({new_requester, POLLIN, 0});
                    std::cout << "Nueva peticion por http con fd " << new_requester << std::endl; 
                }
            }

            // Tenemos datos de algun cliente
            else {
                char tmp_buffer[1024];
                ssize_t bytes = recv(fds[i].fd, tmp_buffer, sizeof(tmp_buffer)-1, 0);

                // Si encontramos un error o no hay informacion, perdimos el cliente
                if (bytes <= 0) {
                    std::cout << "Cliente con FD " << fds[i].fd << " desonnectado" << std::endl;
                    close(fds[i].fd);
                    fds.erase(fds.begin() + i);
                }
                else {
                    tmp_buffer[bytes] = '\0';
                    std::cout << "Recivimos datos de " << fds[i].fd << ':' << tmp_buffer << std::endl;
                    send(fds[i].fd, tmp_buffer, bytes, 0);
                }
            }
        }
    }

    close(chat_space);
    close(http_space);
    return 0;
}