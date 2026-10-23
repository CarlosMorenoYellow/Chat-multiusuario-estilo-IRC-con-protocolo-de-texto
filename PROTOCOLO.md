# Network Chat Protocol (NCP) Specification

## Estado de este documento

Este documento describe el protocolo de chat que implementa actualmente el
proyecto. Usa una estructura inspirada en RFC para facilitar su lectura; no es
un RFC publicado ni implica compatibilidad con IRC.

## 1. Introduccion



## 2. Convenciones y transporte

- Cada comando de cliente termina con `CRLF` (`\r\n`).
- Las respuestas y eventos del servidor también terminan con `CRLF`.
- TCP es un flujo de bytes: un comando puede llegar en varias lecturas.
	Cada conexión mantiene su propio buffer.
- El cuerpo de una línea de chat puede tener hasta 512 bytes, sin contar el
	terminador `CRLF`. El cliente de terminal rechaza entradas de más de 512
	bytes antes de enviarlas sin embargo tambien se chequea en el servidor.
- Los comandos no distinguen mayúsculas de minúsculas. Los argumentos conservan
	sus mayúsculas y minúsculas; apodos y nombres de canal, por tanto, se comparan distinguiendo mayúsculas.
- La implementación separa el comando de sus argumentos usando el primer
	espacio ASCII. No admite tabulador como separador de comando.
- Los argumentos no tienen sintaxis de comillas ni mecanismo de escape.

En los ejemplos, `C:` identifica datos enviados por el cliente y `S:` datos
enviados por el servidor. Los finales `\r\n` se muestran explícitamente.

## 3. Comandos

| Comando | Gramatica | Uso | Codigo de error |
| --- | --- | --- | --- |
| NICK | `NICK <apodo>` | Registra el apodo | 433, 501 |
| JOIN | `JOIN <#canal>` | Entra al canal o lo crea si no existe | 501, 530 |
| PART | `PART <#canal>` | Sale del canal | 442, 551 |
| MSG | `MSG <#canal> <texto>` | Envia mensaje al canal | 442, 551 |
| PRIV | `PRIV <apodo> <texto>` | Envia mensaje privado | 550 |
| CHANS | `CHANS` | WHO <#canal> | /// |
| WHO | `WHO <#canal>` | Lista los miembros del canal | 551 |
| STAT | `STAT` | Cantidad de mensajes en un buzon y tamano total de bytes | /// |
| LIST | `LIST` | Un reglon <numero> <n> por mensaje en buzon | /// |
| RETR | `RETR <n>` | Devuelve el mensaje <n> completo, en formato RFC 5322 | 552 |
| DELE | `DELE <n>` | Marca el mensaje <n> para borrar | 552 |
| QUIT | `QUIT` | Aplica los borrados pendientes, responde 299 y cierra la conexion. | /// |

## 4. Códigos de respuesta y eventos

Todas las respuestas del protocolo de chat llevan una línea de texto terminada
en `CRLF`. La tabla distingue los códigos que actualmente emite el servidor de
los códigos definidos en el encabezado pero todavía no conectados a un comando.

| Código | Significado |
| --- | --- |
| 200 | Saludo de conexión de chat |
| 201 | Apodo registrado o cambiado |
| 210 | Unión a canal confirmada |
| 211 | Salida de canal confirmada |
| 212 | Respuesta multilinea de CHANS; una linea 212-<#canal> <n> por canal y una final 212 <total> canales |
| 213 | Miembro de canal y total de miembros |
| 220 | Respuesta de STAT |
| 221 | Respuesta multilinea de LIST. Una linea 221-<numero> <bytes> por mensaje y una final 221 <n> mensajes|
| 222 | Respuesta multilinea de RETR. EL mensaje RFC 5322 completo, una linea 222- por cada linea del mensaje, y una final 222 fin del mensaje |
| 223 | DELE aceptado |
| 250 | Mensaje entregado |
| 251 | PRIV a un usuario desconectado: guardado en su buzon |
| 299 | Respuesta a QUIT; luego el servidor cierra la conexion. |
| 300 | Mensaje publicado en un canal |
| 301 | Mensaje privado recibido en vivo. |
| 302 | Aviso: 'entra', 'sale', o 'desconecta'. |
| 303 | EL usuario tiene n mensaje en su buzon |
| 421 | Servidor saturado; la conexion se cerrara |
| 433 | Apodo ocupado |
| 442 | Cliente no miembro del canal |
| 500 | Comando desconocido |
| 501 | Sintaxis o argumento inválido |
| 502 | Línea demasiado larga, el servidor descarta todo lo que esta antes del CRLF|
| 530 | Falta registrar un apodo |
| 550 | Usuario desconocido. Ese apodo nunca se ha registrado |
| 551 | Canal inexistente |
| 552 | No existe ese numero de mensaje en el buzon |


## 5. Ejemplos

### Registro y publicación

```text
S: 200 chat de prueba listo\r\n
C: NICK ana\r\n
S: 201 Bienvenido(a), ana\r\n
C: JOIN #redes\r\n
S: 210 Te uniste a #redes\r\n
C: MSG #redes hola\r\n
S: 250 OK!\r\n
```

Cada otro miembro de `#redes` recibe:

```text
S: 300 #redes ana: hola\r\n
```

### Error por falta de registro

```text
C: WHO #redes\r\n
S: 530 No te has gesitrado con NICK\r\n
```

### Consulta de miembros

```text
C: WHO #redes\r\n
S: 213-ana\r\n
S: 213-luis\r\n
S: 213 2 miembros\r\n
```

## 6. Casos borde y decisiones actuales

- **MSG a uno mismo:** si el emisor es miembro del canal, recibe `250 OK!`, pero
	no recibe evento `300` por su propio mensaje. El servidor excluye al socket
	emisor al distribuir el evento.
- **PRIV al mismo usuario o a otro:** no aplica todavía; el comando `PRIV` no
	existe. Después del registro produce `500`; antes del registro produce `530`.
- **Canales vacíos:** al salir el último miembro con `PART` o al desconectarse,
	el canal se elimina. Un `MSG` o `WHO` posterior para ese canal da `551` hasta
	que alguien vuelva a crearlo con `JOIN`.
- **Mayúsculas en apodos/canales:** la unicidad y búsqueda son sensibles a
	mayúsculas. `Ana` y `ana` pueden ser apodos distintos; `#redes` y `#REDES` no
	son el mismo canal, y el segundo no pasa la validación de `JOIN`/`WHO`.
- **Argumentos sobrantes:** `NICK`, `JOIN` y `WHO` no aceptan espacios dentro
	del argumento, por lo que suelen acabar en `501`. `QUIT` ignora el resto de
	la línea. `PART` usa la cadena completa como nombre de canal. `MSG` usa el
	primer espacio para separar el canal y conserva el resto como texto.
- **Mensaje vacío:** `MSG #canal ` pasa la comprobación sintáctica si el emisor
	pertenece al canal; se publica un texto vacío a los demás miembros.
- **Línea incompleta:** el servidor espera hasta recibir `CRLF`; no procesa una
	línea parcial. Varias líneas completas recibidas juntas se procesan en orden.
- **Línea larga:** el servidor responde `502` y descarta datos hasta encontrar
	un `CRLF`. El límite operativo se mide en bytes, no en caracteres Unicode.
- **Desconexión abrupta:** el servidor quita al socket desconectado de sus
	canales y elimina aquellos que queden vacíos.