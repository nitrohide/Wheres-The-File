all: ./client/WTF ./server/WTFserver WTFtest

./client/WTF: ./client/WTF.c
	gcc ./client/WTF.c -lssl -lcrypto -o ./client/WTF

./server/WTFserver: ./server/WTFserver.c
	gcc ./server/WTFserver.c -o ./server/WTFserver -lpthread

WTFtest:
	gcc WTFtest.c -o WTFtest -lz

test:
	gcc WTFtest.c -o WTFtest -lz; gcc ./client/WTF.c -lssl -lcrypto -o ./client/WTF; gcc ./server/WTFserver.c -o ./server/WTFserver -lpthread;

clean:
	rm ./client/WTF; rm ./server/WTFserver; rm WTFtest;


