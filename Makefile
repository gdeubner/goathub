all: WTF WTFserver

WTF: client.c gStructs.o fileManip.o network.o 
	gcc client.c gStructs.o fileManip.o network.o -o WTF -lssl -lcrypto

WTFserver: server.c gStructs.o fileManip.o network.o
	gcc server.c gStructs.o fileManip.o network.o -o WTFserver -lssl -lcrypto

gStructs.o:
	gcc -c gStructs.c

network.o:
	gcc -c network.c

fileManip.o:
	gcc -c fileManip.c

clean:
	rm WTF; rm WTFserver; rm gStructs.o; rm network.o; rm fileManip.o;



gdb: wtf.c gStructs.o network.o
	gcc -g wtf.c gStructs.o network.o -o test  -lssl -lcrypto

comp: wtf.c gStructs.o network.o
	gcc -g -Wall -fsanitize=address wtf.c gStructs.o network.o -o test  -lssl -lcrypto


test: wtf.c gStructs.o network.o
	gcc wtf.c gStructs.o network.o -o test -lssl -lcrypto

cleanall:
	rm -f test; rm -f gStructs.o; rm -f network.o; rm -f serverfd; rm -f clientfd; rm -f dir/.Manifest; rmdir dir; rm -f serverdir/.Manifest; rmdir serverdir
