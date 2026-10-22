cc = g++
flags = -std=c++17 -Wall -Wextra -pthread

all: servidor 

servidor: src/Server/server.cpp src/Server/server.hpp
		$(cc) $(flags) src/Server/server.cpp -o servidor

cliente: src/Client/client.cpp
		$(cc) $(flags) src/Client/client.cpp -o cliente

clean:
		rm -f servidor 
		rm -f cliente