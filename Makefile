
sockdir = $(CURDIR)/libsocket
socksrc = sock-exp.c $(sockdir)/sock.c

serialdir = $(CURDIR)/libserial
serialsrc = serial-exp.c $(serialdir)/serial.c

all: sock-exp serial-exp;

sock-exp: $(socksrc)
	gcc -o $@ $^ -I$(sockdir) -lpthread
	
serial-exp: $(serialsrc)
	gcc -o $@ $^ -I$(serialdir) -lpthread

clean: sock-exp-clean serial-exp-clean;

sock-exp-clean:
	rm sock-exp

serial-exp-clean:
	rm serial-exp

.PHONY: sock-exp serial-exp
