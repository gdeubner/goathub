all: wtf.c gStructs.o network.o
	gcc wtf.c gStructs.o network.o -o WTF -lssl -lcrypto

gStructs.o:
	gcc -c gStructs.c

network.o:
	gcc -c network.c

gdb: wtf.c gStructs.o network.o
	gcc -g wtf.c gStructs.o network.o -o WTF  -lssl -lcrypto

comp: wtf.c gStructs.o network.o
	gcc -g -Wall -fsanitize=address wtf.c gStructs.o network.o -o WTF  -lssl -lcrypto
server: testServer.c
	gcc -g network.o -lcrypto testServer.c -o server
client: network.o test.c
	gcc test.c -g network.o -lcrypto -o client
clean:
	rm -f WTF; rm -f gStructs.o; rm -f network.o; rm -f serverfd; rm -f clientfd; rm -f dir/.Manifest; rmdir dir; rm -f serverdir/.Manifest; rmdir serverdir
