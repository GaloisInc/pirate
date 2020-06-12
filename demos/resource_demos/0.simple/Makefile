all: resources.o
	clang -fuse-ld=lld -Wl,-enclave,orange -g -o resources $^

%.o: %.c
	clang -fdata-sections -ffunction-sections -Wall -Werror -g -c $^

clean:
	$(RM) resources
