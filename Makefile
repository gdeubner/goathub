all: goat goatServer

test: all goatTest goatTestServer

goat: client.c gStructs.o fileManip.o network.o 
	gcc -g client.c gStructs.o fileManip.o network.o -o goat -lssl -lcrypto

goatServer: server.c gStructs.o fileManip.o network.o
	gcc -g server.c gStructs.o fileManip.o network.o -o goatServer -lssl -lcrypto -lpthread

goatTest: test.c
	gcc test.c -o goatTest

goatTestServer: testserver.c
	mkdir server;
	gcc testserver.c -o ./server/goatTestServer;

gStructs.o:
	gcc -c -g gStructs.c

network.o:
	gcc -c -g network.c

fileManip.o:
	gcc -c -g fileManip.c

clean:
	rm -R goat goatServer gStructs.o network.o fileManip.o goatTest server testproject;

