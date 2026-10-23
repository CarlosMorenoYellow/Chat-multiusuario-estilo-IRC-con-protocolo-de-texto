# Network Chat Protocol (NCP)

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
	espacio. No admite tabulador como separador de comando.
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
| 201 | Apodo registrado |
| 210 | Unión a canal confirmada |
| 211 | Salida de canal confirmada |
| 212 | Respuesta multilinea de CHANS; una linea 212-<#canal> <n> por canal (n son miembros) y una final 212 <total> canales |
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
S: 200 chat de prueba listo
C: NICK ana
S: 201 Bienvenido(a), ana
C: JOIN #redes
S: 210 Te uniste a #redes
C: MSG #redes hola
S: 250 OK!
```

Cada otro miembro de `#redes` recibe:

```text
S: 300 #redes ana: hola
```

### Mensaje privado en vivo

Si `luis` está conectado, `ana` envía:

```text
C: PRIV luis Hola, Luis
S (ana): 250 OK
S (luis): 301 ana Hola, Luis
```

### Mensaje privado guardado en el buzón

Si `luis` ya fue registrado anteriormente pero está desconectado, el servidor
guarda el mensaje para su próxima sesión:

```text
C (ana): PRIV luis Hola, Luis
S (ana): 251 Mensaje guardado en buzon
```

Cuando `luis` vuelva a conectarse y ejecute `NICK luis`, el servidor le avisará
cuántos mensajes tiene pendientes:

```text
S (luis): 303 Tienes 1 mensajes!
```

### Consultar el buzón

Suponiendo que el buzón de `luis` contiene un mensaje, `STAT` devuelve la
cantidad de mensajes activos y el total de bytes de sus archivos. `<bytes>` es
un marcador: el tamaño concreto depende del mensaje guardado.

```text
C (luis): STAT
S (luis): 220 1 <bytes>
```

`LIST` enumera los mensajes activos con su número y tamaño, seguido del total:

```text
C (luis): LIST
S (luis): 221-1 <bytes>
S (luis): 221 1 mensajes
```

`RETR` devuelve el mensaje numerado. Cada línea del archivo se anuncia con el
código `222-`; el último renglón termina la respuesta:

```text
C (luis): RETR 1
S (luis): 222-From: ana@chat.ci4835
S (luis): 222-To: luis@chat.ci4835
S (luis): 222-Date: <fecha del servidor>
S (luis): 222-Subject: Mensaje privado de ana
S (luis): 222-Message-ID: <identificador generado por el servidor
S (luis): 222-
S (luis): 222-Hola, Luis
S (luis): 222 fin del mensaje
```

`DELE` marca el mensaje para borrarlo; el borrado se aplica al cerrar la sesión
con `QUIT`:

```text
C (luis): DELE 1
S (luis): 223 Mensaje 1 marcado para borrar
C (luis): QUIT
S (luis): 299 Bye bye!
```

Antes de `QUIT`, el mensaje marcado ya no aparece en `STAT` ni `LIST`, y no se
puede recuperar con `RETR`. Si el número no existe o ya fue marcado, el servidor
responde `552`.

### Error por falta de registro

```text
C: WHO #redes
S: 530 No te has registrado con NICK
```

### Consulta de miembros

```text
C: WHO #redes
S: 213-ana
S: 213-luis
S: 213 2 miembros
```

## 6. Casos borde y decisiones actuales

- **MSG a uno mismo:** si el emisor es miembro del canal, recibe `250 OK!`, pero
	no recibe evento `300` por su propio mensaje. El servidor excluye al socket
	emisor al distribuir el evento.
- **PRIV al mismo usuario o a otro:** Se envia un correo a si mismo, funciona como cualquier otro usuario
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
- **Desconexión abrupta:** el servidor quita al socket desconectado de sus canales y elimina aquellos que queden vacíos.
- **Usuario hace join a un canal en el que ya esta:** Manda verificacion al usuario de que se unio y se lee en el servidor su accion, mas no crea una segunda instancia del usuario. Efectivamente no hace nada.
- **Usuario usa nick por segunda vez:** Directamente no lo permite. manda un mensaje informando que aquello no esta permitido.