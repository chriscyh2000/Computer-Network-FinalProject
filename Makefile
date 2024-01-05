CC = gcc
CXX = g++
INCLUDE_OPENCV = `pkg-config --cflags --libs opencv4` # wrong include
LINK_PTHREAD = -lpthread

CLIENT = client.cpp
SERVER = server.cpp
OPEN_CV = openCV.cpp
PTHREAD = pthread.c
CLI = client
SER = server
CV = openCV
PTH = pthread

all: server client  
  
server: $(SERVER)
	$(CXX) -std=c++11 $(SERVER) -o $(SER) $(INCLUDE_OPENCV) $(LINK_PTHREAD) 
client: $(CLIENT)
	$(CXX) -std=c++11 $(CLIENT) -o $(CLI) $(INCLUDE_OPENCV) 
opencv: $(OPEN_CV)
	$(CXX) -std=c++11 $(OPEN_CV) -o $(CV) $(INCLUDE_OPENCV)
pthread: $(PTHREAD)
	$(CXX) -std=c++11 $(PTHREAD) -o $(PTH) $(LINK_PTHREAD)

.PHONY: clean

clean:
	rm $(CLI) $(SER) $(CV)
