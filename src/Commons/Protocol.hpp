#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace protocol {
// CONSTANTES DE LIMITES 

constexpr std::size_t MAXCLIENTLINE = 512;
constexpr std::size_t MAXNICKNAME = 16;
constexpr std::size_t MAXCANAL = 32;
constexpr std::size_t MAXCLIENTNUMBER = 300;

// CODIGOS 2XX: EXITO

constexpr int EX_HEYO = 200;
constexpr int EX_NICK = 201;
constexpr int EX_JOIN = 210;
constexpr int EX_PART = 211;
constexpr int EX_CHANS = 212;
constexpr int EX_WHO = 213;
constexpr int EX_STAT = 220;
constexpr int EX_LIST = 221;
constexpr int EX_RETR = 222;
constexpr int EX_DELE = 223;
constexpr int EX_MSG_DELIVER = 250;
constexpr int EX_PRIC = 251;
constexpr int EX_QUIT = 299;

// CODIGOS 3XX: EVENTO ASINCRONICO

constexpr int ASYNC_MSG_PUBLISHED = 300;
constexpr int ASYNC_PRIV_PUBLISHED = 301;
constexpr int ASYNC_EVENT = 302;
constexpr int ASYNC_MSG_MAIL = 303;

// CODIGOS 4XX: ERROR TEMPORAL

constexpr int ERRTEMP_SERVER_SATURATED = 421;
constexpr int ERRTEMP_NICK_OCUPPIED = 433;
constexpr int ERRTEMP_NON_MEMBER = 442;

// CODIGOS 5XX: ERROR PERMANENTE

constexpr int ERRPERM_UNKNOWN_COMMAND = 500;
constexpr int ERRPERM_INVALID_SYNTAX = 501;
constexpr int ERRPERM_LONG_LINE = 502;
constexpr int ERRPERM_NONREGISTERED_USER = 530;
constexpr int ERRPERM_UNREGISTERED_USER = 550;
constexpr int ERRPERM_NONEXISTENT_CHANNEL = 551;
constexpr int ERRPERM_NONEXISTENT_MSG_MAIL = 552;

// Firmas

bool validarApodo(std::string_view apodo);
bool validarCanal(std::string_view canal);
bool validarLargo(std::string_view linea);
void limpiarCRLF(std::string &linea);
void normalizarMayus(std::string &str);
bool parseComando(std::string_view linea, std::string &cmd, std::string &args);
}