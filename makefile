all: client server

publicHeader := public.h

client:client.cpp $(publicHeader)
	@g++ -g -o client client.cpp $(publicHeader) -lstdc++fs

server:server.cpp $(publicHeader)
	@g++ -g -o server server.cpp $(publicHeader) -lstdc++fs

clean:
	rm -f client server