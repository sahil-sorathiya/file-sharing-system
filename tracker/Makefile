CC = g++
CFLAGS = -Wall
LDFLAGS = -lssl -lcrypto
DEPS = headers.h  
OBJ = classes.o utils.o handlers.o tracker.o

%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

tracker: $(OBJ)
	$(CC) $(CFLAGS) -pthread -o $@ $^ $(LDFLAGS)

clean:
	rm -rf *.o tracker
