# $@ - left side of :
# $^ - right side of :
# $< - first target
COMPFLAGS = gcc -Wall -c
LINKFLAGS = gcc -o
EXTRAFLAGS = -lpthread

all: gateway peer client gallery.o

gateway: gateway.o ring_list.o
	$(LINKFLAGS) $@ $^ $(EXTRAFLAGS)

gateway.o: gateway.c msgs.h linked_list.h
	$(COMPFLAGS) $<

linked_list.o: linked_list.c linked_list.h
	$(COMPFLAGS) $<

ring_list.o: ring_list.c ring_list.h
	$(COMPFLAGS) $<

peer: peer.o
	$(LINKFLAGS) $@ $^ $(EXTRAFLAGS)

peer.o: peer.c msgs.h
	$(COMPFLAGS) $<

client: gallery.o client.o msgs.o
	$(LINKFLAGS) $@ $^ $(EXTRAFLAGS)

client.o: client.c msgs.c gallery.c
	$(COMPFLAGS) $^

gallery.o: gallery.c gallery.h msgs.c
	$(COMPFLAGS) $< msgs.c

msgs.o: msgs.c msgs.h
	$(COMPFLAGS) $<

test: test.c
	$(LINKFLAGS) $@ $<

clean:
	rm -f gateway peer client *.o
