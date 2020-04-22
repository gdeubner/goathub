all: wtf.c gStructs.o fileManip.o network.o
	gcc wtf.c gStructs.o fileManip.o network.o  -o WTF -lssl -lcrypto

gStructs.o:
	gcc -c gStructs.c

network.o:
	gcc -c network.c

fileManip.o:
	gcc -c fileManip.c

gdb: wtf.c gStructs.o network.o fileManip.o
	gcc -g wtf.c gStructs.o network.o fileManip.o -o WTF  -lssl -lcrypto

comp: wtf.c gStructs.o network.o fileManip.o
	gcc -g -Wall -fsanitize=address wtf.c gStructs.o network.o fileManip.o -o WTF  -lssl -lcrypto

clean:
	rm -f WTF; rm -f gStructs.o; rm -f network.o; rm -f fileManip.o; rm -f serverfd; rm -f clientfd; rm dir/.Manifest; rmdir dir; rm  serverdir/.Manifest; rmdir serverdir
