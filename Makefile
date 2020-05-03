all: WTF WTFserver

WTF: client.c gStructs.o fileManip.o network.o 
	gcc -g client.c gStructs.o fileManip.o network.o -o WTF -lssl -lcrypto

WTFserver: server.c gStructs.o fileManip.o network.o
	gcc -g server.c gStructs.o fileManip.o network.o -o WTFserver -lssl -lcrypto

gStructs.o:
	gcc -c -g gStructs.c

network.o:
	gcc -c -g network.c

fileManip.o:
	gcc -c -g fileManip.c

clean:
	rm WTF WTFserver gStructs.o network.o fileManip.o;

cleantest:
#	mv ./project/test1.txt .
	rm -r project server .configure WTF WTFserver gStructs.o network.o fileManip.o

test: WTF WTFserver
	mkdir server; mv WTFserver server; 

run:
	./WTF configure 127.0.0.1 5004;
	./WTF create project; 
	mv test1.txt project;
	./WTF add project test1.txt;
	mkdir temp; mv project temp; mv server/project .; mv temp/project server; rmdir temp;
	emacs server/project/.Manifest;

run1:
	./WTF update project;
	./WTF upgrade project;

gdb: wtf.c gStructs.o network.o
	gcc -g wtf.c gStructs.o network.o -o test  -lssl -lcrypto

comp: wtf.c gStructs.o network.o
	gcc -g -Wall -fsanitize=address wtf.c gStructs.o network.o -o test  -lssl -lcrypto

cleanall:
	rm -f test; rm -f gStructs.o; rm -f network.o; rm -f serverfd; rm -f clientfd; rm -f dir/.Manifest; rmdir dir; rm -f serverdir/.Manifest; rmdir serverdir
