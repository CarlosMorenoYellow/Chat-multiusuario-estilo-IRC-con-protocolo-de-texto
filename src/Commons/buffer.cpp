#include "buffer.hpp"

//Inicializa
LineBuffer::LineBuffer(size_t maxlineIn) : maxline(maxlineIn) {}


//append para el buffer de LineBuffer
void LineBuffer::append(const char *data, size_t tamano){
    buffer.append(data, tamano);
}

// Intenta extraer una linea completa delimitada por "\r\n"
// Retorna True si lo encuentra y extrae la informacion.
//False en caso contrario.
bool LineBuffer::extraerLinea(std::string &linea, bool &errorLargo){
    //se asume que no hay error al iniciar.
    errorLargo = false;

    //Busca "\r\n". Al obtener la posicion se obtiene
    //la cantidad de bytes antes del CRLF
    size_t CRLFpos = buffer.find("\r\n");
    //Se encontro CRLF
    if (CRLFpos != std::string::npos) {
        //el mensaje es muy largo
        if (CRLFpos > maxline) {
            errorLargo = true;
            buffer.erase(0, CRLFpos + 2);
            return false;
        }
        
        //El mensaje es de un largo aceptable
        linea = buffer.substr(0, CRLFpos + 2);
        buffer.erase(0, CRLFpos + 2);
        return true;
    }

    //no se encontro CRLF pero el tamano del buffer excede el maximo
    if (buffer.size() > maxline){
        errorLargo = true;
        buffer.clear();
        return false;
    }
    //msj incompleto. se sigue esperando informacion del socket
    return false;
}

//Clear para LineBuffer
void LineBuffer::clear() {
    buffer.clear()
}