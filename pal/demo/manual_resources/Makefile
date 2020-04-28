.PHONY: all
all:
	clang -Wall -Werror -g -o app -I ../../include app.c \
		-fuse-ld=lld -L ../../build -Wl,-rpath,../../build -lpal

.PHONY: clean
clean:
	$(RM) app
