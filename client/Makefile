CC = g++
CFLAGS = -Wall -Wno-deprecated-declarations
LDFLAGS = -lssl -lcrypto
DEPS = headers.h  
OBJ = utils.o handlers.o client.o

%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

client: $(OBJ)
	$(CC) $(CFLAGS) -pthread -o $@ $^ $(LDFLAGS)

clean:
	rm -rf *.o client
