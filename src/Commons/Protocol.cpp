#include "Protocol.hpp"

#include <cctype>
#include <algorithm>

namespace protocol {


// revisa string conteniendo apodo para verificar que cumpla formato
bool validarApodo(std::string_view apodo) {
    if (apodo.empty() || apodo.size() > MAXNICKNAME) return false;
    for (unsigned char simb : apodo) {
        if (!std::isalnum(simb) && simb != '-' && simb != '_') return false;
    }
    return true;
}

// revisa string conteniendo canal para verificar que cumpla formato
bool validarCanal(std::string_view canal) {
    if (canal.size() < 2 || canal.front() != '#') return false;
    const std::string_view nombre = canal.substr(1);
    if (nombre.size() > MAXCANAL) return false;
    for (unsigned char simb : nombre) {
        if (!std::isdigit(simb) && !std::islower(simb) && simb != '-' && simb != '_') {
            return false;
        }
    }
    return true;
}

// revisa string conteniendo msg para verificar que con el largo maximo.
bool validarLargo(std::string_view linea) {
    return linea.size() <= MAXCLIENTLINE;
}

// revisa string dado y elimina caracteres \n y \r
void limpiarCRLF(std::string &linea) {
    if (!linea.empty() && linea.back() == '\n') {
        linea.pop_back();
        if (!linea.empty() && linea.back() == '\r') linea.pop_back();
    }
}

// revisa string dado y convierte todo en mayusculas
void normalizarMayus(std::string &linea){
    std::transform(linea.begin(), linea.end(), linea.begin(), [](unsigned char symb) {
        return static_cast<char>(std::toupper(symb));
    });
}

// Toma un string de entrada y dos variables en las cuales se guardara el comando y el resto de la linea respectivamente.
// Retorna True si el parseo fue exitoso. Caso contrario retorna False
bool parseComando(std::string_view linea_entrada, std::string &cmd, std::string &args) {
    //elimina comandos y contenido anteriores.
    cmd.clear();
    args.clear();

    //elimina simbolos terminales CRLF
    std::string temp(linea_entrada);
    limpiarCRLF(temp);

    // Encuentra inicio del comando. Retorna false si solo hay espacios o esta vacia
    size_t inicio = temp.find_first_not_of(" \t");
    if (inicio == std::string::npos) return false;

    // Encuentra el final del comando. si solo esta el comando lo extrae
    size_t fin_cmd = temp.find_first_of(" \t", inicio);
    if (fin_cmd == std::string::npos) {
        cmd = temp.substr(inicio);
    } else {
        // Hay mas aparte del comando. Lo extrae.
        cmd = temp.substr(inicio, fin_cmd - inicio);
        // Encontrar el inicio de los argumentos y los extrae.
        size_t inicio_args = temp.find_first_not_of(" \t", fin_cmd);
        if (inicio_args != std::string::npos) {
            args = temp.substr(inicio_args);
        }
    }

    // lleva el comando a mayusculas ya que el programa no le importa si son mayusculas o minusculas
    normalizarMayus(cmd);
    return !cmd.empty();
}

}