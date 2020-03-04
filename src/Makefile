CFLAGS = -pthread
GCC = gcc $(CFLAGS)

APPS = serverMain

all: $(APPS)

serverMain: serverMain.c
	$(GCC) -o serverMain serverMain.c
clean:
	rm -f $(APPS)
