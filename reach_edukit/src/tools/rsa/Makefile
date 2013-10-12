all:
	arm-none-linux-gnueabi-gcc -c rsa.c `pkg-config libssl --cflags`
	arm-none-linux-gnueabi-ar cqs librsa.a rsa.o 
	arm-none-linux-gnueabi-ranlib librsa.a
	arm-none-linux-gnueabi-gcc -o rsa_key_gen rsa_key_gen.c librsa.a ./libcrypto.a -ldl
	arm-none-linux-gnueabi-gcc -o cli_reg cli_reg.c librsa.a ./libcrypto.a -ldl
	arm-none-linux-gnueabi-gcc -o srv_build_key srv_build_key.c librsa.a ./libcrypto.a -ldl
	arm-none-linux-gnueabi-gcc -o cli_verify check.c cli_verify.c librsa.a ./libcrypto.a -ldl
	rm -rf ./bin
	rm -rf ./lib
	mkdir -p ./bin
	mkdir -p ./lib
	cp cli_verify ./bin
	cp cli_reg ./bin
	cp rsa_key_gen ./bin
	cp srv_build_key ./bin
	cp librsa.a ./lib
