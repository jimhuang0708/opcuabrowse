CC=gcc
CFLAGS=-I.
DEPS = 

%.o: %.c $(DEPS)
	$(CC) -g -c -o $@ $< $(CFLAGS)

opcuaserver: datacollect.o list.o open62541.o server.o
	$(CC) -g -o opcuaserver datacollect.o list.o open62541.o server.o -lssl -lcrypto -lcurl

opcuaclient:
	$(CC) -g -o opcuaclient open62541.c client/client.c client/api.c client/general.c -I./ -I/usr/include/cjson/ -lssl -lcrypto -lcjson

clean:
	rm -f *.o opcuaserver opcuaclient
