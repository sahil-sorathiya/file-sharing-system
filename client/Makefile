CC = g++
CFLAGS = -Wall
DEPS = headers.h
OBJ = utils.o client.o
%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

client: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -pthread -I/usr/include/openssl
 
clean:
	rm -rf *o client