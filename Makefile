CC = g++
OPENCV =  `pkg-config --cflags --libs opencv`
PTHREAD = -pthread

CLIENT = src/client.cpp src/client_connection.cpp src/client_data_transfer.cpp src/client_video.cpp
SERVER = src/server.cpp src/server_connection.cpp src/server_data_transfer.cpp src/server_video.cpp
CLI = client
SER = server

all: server client
  
server: $(SERVER)
	$(CC) $(SERVER) -o $(SER)  $(OPENCV) $(PTHREAD) 
client: $(CLIENT)
	$(CC) $(CLIENT) -o $(CLI)  $(OPENCV) $(PTHREAD)

debug: serverDEBUG clientDEBUG

serverDEBUG: $(SERVER)
	$(CC) $(SERVER) -DDEBUG -o $(SER)  $(OPENCV) $(PTHREAD) 
clientDEBUG: $(CLIENT)
	$(CC) $(CLIENT) -DDEBUG -o $(CLI)  $(OPENCV) $(PTHREAD)

.PHONY: clean

clean:
	rm $(CLI) $(SER)