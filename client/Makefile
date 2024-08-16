CC = g++
CFLAGS = -Wall -Wno-deprecated-declarations
LDFLAGS = -lssl -lcrypto -lfmt
DEPS = headers.h  
OBJ = classes.o utils.o seederHandlers.o leecherHandlers.o client.o

%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

client: $(OBJ)
	$(CC) $(CFLAGS) -pthread -o $@ $^ $(LDFLAGS)

clean:
	rm -rf *.o client
