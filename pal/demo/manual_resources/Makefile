.PHONY: all
all:
	clang -Wall -Werror -g -o app -I ../../include app.c \
		-fuse-ld=lld -Wl,-enclave,my_app -Wl,-rpath,../../build -L ../../build -lpal

.PHONY: clean
clean:
	$(RM) app
