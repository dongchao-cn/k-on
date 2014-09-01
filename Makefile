DEBUG = -DDEBUG -g
# DEBUG = 

all: 
	@echo DONE!

azusa: protocol node socketop
	-mkdir bin
	g++ -o bin/azusa src/azusa.cpp build/Protocol.o build/Node.o build/SocketOp.o -lpthread $(DEBUG)

tsumugi: protocol node socketop
	-mkdir bin
	g++ -o bin/tsumugi src/tsumugi.cpp build/Protocol.o build/Node.o build/SocketOp.o $(DEBUG)


protocol:
	-mkdir build
	g++ -c src/Protocol.cpp -o build/Protocol.o $(DEBUG)

node:
	-mkdir build
	g++ -c src/Node.cpp -o build/Node.o $(DEBUG)

socketop:
	-mkdir build
	g++ -c src/SocketOp.cpp -o build/SocketOp.o $(DEBUG)

.PHONY : clean
clean:
	-rm -rf bin
