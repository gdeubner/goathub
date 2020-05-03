all: WTF WTFserver

test: all WTFtest WTFtestserver

WTF: client.c gStructs.o fileManip.o network.o 
	gcc -g client.c gStructs.o fileManip.o network.o -o WTF -lssl -lcrypto

WTFserver: server.c gStructs.o fileManip.o network.o
	gcc -g server.c gStructs.o fileManip.o network.o -o WTFserver -lssl -lcrypto

WTFtest: test.c
	gcc test.c -o WTFtest

WTFtestserver: testserver.c
	mkdir server;
	gcc testserver.c -o ./server/WTFtestserver;

gStructs.o:
	gcc -c -g gStructs.c

network.o:
	gcc -c -g network.c

fileManip.o:
	gcc -c -g fileManip.c

clean:
	rm -R WTF WTFserver gStructs.o network.o fileManip.o WTFtest server testproject;

