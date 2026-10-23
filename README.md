# Chat multiusuario con protocolo de texto

Proyecto 1 de Redes del Computador I (CI-4835).

## Requisitos

- Fedora o una distribución Linux compatible con sockets POSIX.
- `g++` con soporte para C++17, `make` y `pthread`.

En Fedora, instala las herramientas de compilación si aún no están disponibles:

```bash
sudo dnf install gcc-c++ make
```

## Compilar

Desde la carpeta raíz del proyecto:

```bash
make
```

Esto genera los ejecutables `servidor` y `cliente`. Para borrar los binarios:

```bash
make clean
```

## Ejecutar

La interfaz de ejecución prevista permite elegir los puertos y la carpeta de
buzones al iniciar el servidor:

```bash
./servidor --chat <Puerto> --http <Puerto> --buzones ./buzones
./cliente <host> <Puerto>
```

El cliente puede recibir `localhost` cuando se ejecuta en la misma máquina o
una IPv4 numérica cuando se conecta desde otra computadora de la red local.


 Para detener el servidor actual, usa `Ctrl+C`.

`localhost` sirve cuando cliente y servidor se ejecutan en la misma máquina.
Desde otra computadora de la red local, usa la IPv4 de la máquina donde corre el
servidor.

## Probar el chat

Con el servidor actual en ejecución, abre dos terminales adicionales y conecta
un cliente en cada una:

```text
NICK ana
JOIN #redes
```

En el segundo:

```text
NICK luis
JOIN #redes
```

Envía un mensaje desde cualquiera de los clientes:

```text
MSG #redes Hola, equipo
```

El emisor recibe la confirmación `250`; los demás miembros reciben un evento
`300` con el mensaje. También puedes probar:

```text
WHO #redes
CHANS
PART #redes
QUIT
```

Cada línea se envía al presionar Enter. Los comandos disponibles y sus
respuestas se describen con más detalle en [PROTOCOLO.md](PROTOCOLO.md).

## Probar las consultas HTTP

Con el servidor activo, consulta los usuarios conectados y los canales desde
otra terminal:

```bash
curl http://localhost:8080/usuarios
curl http://localhost:8080/canales
curl http://localhost:8080/historial/<canal>
```

El historial contiene hasta los últimos 50 mensajes del canal mientras este
exista. Para consultar desde otra máquina, sustituye `localhost` por la IPv4 del
servidor.
