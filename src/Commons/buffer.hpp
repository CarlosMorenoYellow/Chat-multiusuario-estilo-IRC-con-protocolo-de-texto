#pragma once

#include <string>
#include <string_view>
#include "Protocol.hpp"

class LineBuffer {

private:
    //variables privadas
    std::string buffer;
    size_t maxline;

public:
    //COnstructor de la clase
    explicit LineBuffer(size_t maxline = protocol::MAXCLIENTLINE);
    // Agrega bytes leidos del socket al final del buffer
    void append(const char *data, size_t tamano);
    
    // Intenta extraer una linea completa delimitada por "\r\n"
    // Retorna True si lo encuentra y extrae la informacion.
    //False en caso contrario.
    bool extraerLinea(std::string &linea, bool &errorLArgo);

    //limpia por completo el buffer.
    void clear();

};