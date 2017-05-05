# $@ - left side of :
# $^ - right side of :
# $< - first target
COMPFLAGS = gcc -Wall -c
LINKFLAGS = gcc -o
EXTRAFLAGS = -lpthread

all: gateway peer client gallery.o

gateway: gateway.o linked_list.o
	$(LINKFLAGS) $@ $^ $(EXTRAFLAGS)

gateway.o: gateway.c msgs.h linked_list.h
	$(COMPFLAGS) $<

linked_list.o: linked_list.c linked_list.h
	$(COMPFLAGS) $<

peer: peer.o
	$(LINKFLAGS) $@ $^ $(EXTRAFLAGS)

peer.o: peer.c msgs.h
	$(COMPFLAGS) $<

client: client.o gallery.o
	$(LINKFLAGS) $@ $^ $(EXTRAFLAGS)

client.o: client.c msgs.h gallery.o
	$(COMPFLAGS) $<

gallery.o: gallery.c gallery.h
	$(COMPFLAGS) $<

clean:
	rm -f gateway peer client *.o
