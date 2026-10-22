cc = g++
flags = -std=c++17 -Wall -Wextra -pthread

all: servidor

servidor: src/Server/server.cpp src/Server/server.hpp
		$(cc) $(flags) src/Server/server.cpp -o servidor

cliente: src/Client/main.c
		$(cc) $(flags) src/Client/main.c cliente

clean:
		rm -f servidor 