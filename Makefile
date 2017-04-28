# $@ - left side of :
# $^ - right side of :
# $< - first target
COMPFLAGS = gcc -Wall -c
LINKFLAGS = gcc -o
EXTRAFLAGS = -lpthread

all: gateway peer client

gateway: gateway.o linked_list.o
	$(LINKFLAGS) $@ $^ $(EXTRAFLAGS)

gateway.o: gateway.c msgs.h linked_list.c
	$(COMPFLAGS) $< linked_list.c

peer: peer.o
	$(LINKFLAGS) $@ $^ $(EXTRAFLAGS)

peer.o: peer.c msgs.h
	$(COMPFLAGS) $<

client: client.o
	$(LINKFLAGS) $@ $^ $(EXTRAFLAGS)

client.o: client.c msgs.h
	$(COMPFLAGS) $<

clean:
	rm -f gateway peer client *.o
