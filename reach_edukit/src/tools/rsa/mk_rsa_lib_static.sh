gcc -c rsa.c `pkg-config libssl --cflags`
ar cqs librsa.a rsa.o 
ranlib librsa.a
