CC=gcc
CFLAGS=-I.
DEPS = 

all:opcuaserver opcuaclient

opcuaserver:
	$(CC) -g -o opcuaserver open62541.c server/server.c -lssl -lcrypto -lcurl

opcuaclient:
	$(CC) -g -o opcuaclient open62541.c client/client.c client/api.c client/general.c -I./ -I/usr/include/cjson/ -lssl -lcrypto -lcjson


clean:
	rm -f *.o opcuaserver opcuaclient
