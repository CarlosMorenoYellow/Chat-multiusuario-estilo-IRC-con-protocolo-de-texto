#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include "../Commons/buffer.hpp"
#include "../Commons/buffer.cpp"
#include "../Commons/Protocol.hpp"
#include "../Commons/Protocol.cpp"

int main(int argc, char* argv[]) {

    //chequeo inicial de argumentos
    if (argc != 3) {
        std::cerr << "Uso: " << argv[0] << " <IP> <Puerto> \n";
        return 1;
    }
    
    //agarra ip y el puerto de los argumentos
    std::string ip = argv[1];
    int puerto = 0;

    try {

        size_t idx = 0;
        std::string puertoStr = argv[2];
        puerto = std::stoi(puertoStr, &idx);

        if (idx != puertoStr.length() || puerto <= 1023 || puerto > 65535){
            std::cerr << "Error: Puerto debe ser un entero entre 1024 y 65535\n";
            return 1;
        }
    } catch (...){
        std::cerr << "Error: Puerto invalido ingresado. \n";
        return 1;
    }

    // Configurar direccion del servidor
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(puerto);

    if (ip == "localhost") {
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    } else if (inet_pton(AF_INET, ip.c_str(), &address.sin_addr) != 1) {
        std::cerr << "Error: ingresa una IPv4 valida o localhost\n";
        return 1;
    }

    // crea el socket TCP
    int socfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socfd < 0) {
        perror("Error al crear socket\n");
        return 1;
    }

    // Conectar socket. Si falla se cierra el socket.
    if (connect(socfd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Error al conectar con el servidor\n");
        close(socfd);
        return 1;
    }

    std::cout << "Conexion con el servidor establecida\n" << std::flush;

    LineBuffer bufferCliente;

    //se inicializa la estructura pollfd para que contenga los
    //2 fds necesarios.
    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    fds[1].fd = socfd;
    fds[1].events = POLLIN;

    bool run = true;
    while (run) {

        int ret = poll(fds,2, -1);
        if (ret < 0) {
            perror("Error en poll\n");
            break;
        }

        // Usuario escribe algo en la terminal.

        if (fds[0].revents & POLLIN) {
            std::string input;
            //se recibe lo que el usuario pone por el teclado
            if (std::getline(std::cin, input)) {
                //si el mensaje es muy grande se reporta error
                if (input.size() > protocol::MAXCLIENTLINE){
                    std::cout << std::to_string(protocol::ERRPERM_LONG_LINE) + " Linea demasiado larga\r\n";
                    continue;
                }

                //si todo esta en orden, envia por el socket.
                input += "\r\n";
                send(socfd, input.c_str(), input.size(), 0);
            } else {
                //si EOF en la terminal, se cierra todo (Ctrl + D)
                run = false;
            }
        }

        // El servidor envio datos por la red

        if (fds[1].revents & POLLIN) {
            char temp[1024];
            //lee y guarda bytes crudos temporalmente en temp
            ssize_t bytesRecibidos = recv(socfd, temp, sizeof(temp), 0);


            //Si no se recibe nada
            if (bytesRecibidos <= 0){
                std::cout << "Servidor desconectado. \r\n";
                break;
            }
            
            //almacena bytes recibidos en la estructura LineBuffer
            bufferCliente.append(temp, bytesRecibidos);

            //variables para usar en la funcion extraerLinea
            std::string msgServer;
            bool errorLargo = false;

            // se extraen la lineas delimitadas por \r\n que esten en el bufer
            //y las imprime en pantalla
            while (bufferCliente.extraerLinea(msgServer, errorLargo)) {
                std::cout << "[Servidor] " << msgServer + "\n";

                if (msgServer.rfind("299", 0) == 0) {
                    run = false;
                    break;
                }
            }
        }
    }

    close(socfd);
    return 0;
}