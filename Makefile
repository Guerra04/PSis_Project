# $@ - left side of :
# $^ - right side of :
# $< - first target
COMPFLAGS = gcc -std=gnu99 -Wall -g -c
LINKFLAGS = gcc -g -o
EXTRAFLAGS = -lpthread

all: gateway peer client library.o

# THREE MAIN EXECUTABLES
gateway: gateway.o msgs.o ring_list.o linked_list.o
	$(LINKFLAGS) $@ $^ $(EXTRAFLAGS)

peer: peer.o msgs.o ring_list.o linked_list.o
	$(LINKFLAGS) $@ $^ $(EXTRAFLAGS)

client: client.o library.o msgs.o ring_list.o linked_list.o
	$(LINKFLAGS) $@ $^ $(EXTRAFLAGS)

# OBJECT FILES
gateway.o: gateway.c msgs.h
	$(COMPFLAGS) $<

peer.o: peer.c msgs.h
	$(COMPFLAGS) $<

client.o: client.c msgs.h
	$(COMPFLAGS) $<

library.o: library.c API.h
	$(COMPFLAGS) $<

msgs.o: msgs.c msgs.h
	$(COMPFLAGS) $<

ring_list.o: ring_list.c ring_list.h
	$(COMPFLAGS) $<

linked_list.o: linked_list.c linked_list.h
	$(COMPFLAGS) $<

# TEST PROGRAM
test: test.c
	$(LINKFLAGS) $@ $<

# CLEAN
clean:
	rm -f gateway peer client test *.o
