# $@ - left side of :
# $^ - right side of :
# $< - first target
COMPFLAGS = gcc -std=gnu99 -Wall -c
LINKFLAGS = gcc -o
EXTRAFLAGS = -lpthread

all: gateway peer client gallery.o receiver sender sender2

receiver: peer.o linked_list.o msgs.o ring_list.o linked_list.o
	$(LINKFLAGS) Receiver/peer $^ $(EXTRAFLAGS)

sender: gallery.o client.o msgs.o ring_list.o linked_list.o
	$(LINKFLAGS) Sender/client $^ $(EXTRAFLAGS)

sender2: gallery.o client.o msgs.o ring_list.o linked_list.o
	$(LINKFLAGS) Sender2/client $^ $(EXTRAFLAGS)

gateway: gateway.o ring_list.o msgs.o linked_list.o
	$(LINKFLAGS) $@ $^ $(EXTRAFLAGS)

gateway.o: gateway.c msgs.c ring_list.c
	$(COMPFLAGS) $^

linked_list.o: linked_list.c linked_list.h
	$(COMPFLAGS) $<

ring_list.o: ring_list.c ring_list.h
	$(COMPFLAGS) $<

peer: peer.o linked_list.o msgs.o ring_list.o linked_list.o
	$(LINKFLAGS) $@ $^ $(EXTRAFLAGS)

peer.o: peer.c msgs.c linked_list.c ring_list.c
	$(COMPFLAGS) $^

client: gallery.o client.o msgs.o ring_list.o linked_list.o
	$(LINKFLAGS) $@ $^ $(EXTRAFLAGS)

client.o: client.c msgs.c gallery.c
	$(COMPFLAGS) $^

gallery.o: gallery.c gallery.h msgs.c
	$(COMPFLAGS) $< msgs.c

msgs.o: msgs.c msgs.h ring_list.c linked_list.c
	$(COMPFLAGS) $< ring_list.c linked_list.c

test: test.c
	$(LINKFLAGS) $@ $<

clean:
	rm -f gateway peer client test *.o
