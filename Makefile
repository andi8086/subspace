.PHONY: clean
all: server client


CFLAGS = \
         -g \
	 -MMD \
	 -Ihelpers/ \
	 -Ihelpers/external/zlib/ \
	 -Ihelpers/external/monocypher/src

LDFLAGS = \
        -lpthread \
        -L. \
        -lmonocypher \
        -lz \
        -lnetprot \
        -lnetcrypto


include detect_os.mk

COMMON_OBJs = \
              btrb.o \
              heap.o \
              server_command.o \
              server_client.o \
              con_worker.o \
              client_worker.o \
              work_horse.o

vpath %.c helpers/btrees/
vpath %.c helpers/heapq/

SERVER_OBJs = \
	      server.o \

CLIENT_OBJs = \
	      client.o

SERVER_DEPs = $(SERVER_OBJs:.o=.d)
CLIENT_DEPs = $(CLIENT_OBJs:.o=.d)

-include $(SERVER_DEPs)

-include $(CLIENT_DEPs)

%.o: %.c
	gcc $(CFLAGS) -c $< -o $@

server: $(SERVER_OBJs) $(COMMON_OBJs)
	gcc $^ $(LDFLAGS) -o $@

client: $(CLIENT_OBJs) $(COMMON_OBJs)
	gcc $^ $(LDFLAGS) -o $@


libs: libmonocypher.so libz.so libnetprot.so

libmonocypher.so:
	$(MAKE) -C helpers/external/monocypher
	cp helpers/external/monocypher/lib/*.so{,.*} .

libz.so:
	$(MAKE) -C helpers/external/zlib
	cp helpers/external/zlib/*.so{,.*} .

libnetcrypto.so:
	$(MAKE) -C helpers/net-core

libnetprot.so: libnetcrypto.so
	cp helpers/net-core/*.so{,.*} .

clean:
	$(RM) *.o
	$(RM) *.d
	$(RM) server
	$(RM) client

start_server:
	LD_LIBRARY_PATH=. ./server

start_client:
	LD_LIBRARY_PATH=. ./client
