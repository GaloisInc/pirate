.PHONY: all
all:
	gcc -Wall -Werror -g -o app -I ../../include app.c -L ../../build -Wl,-rpath,../../build -lpal

.PHONY: clean
clean:
	$(RM) app
