CC=gcc
EXEC = server
MYSQL_CFLAGS := $(shell mysql_config --cflags)
MYSQL_LIBS := $(shell mysql_config --libs)
%.o: %.c $(DEPS)
	$(CC) -c -o $@ $<

server: server.o
	$(CC) -o $(EXEC) $(MYSQL_CFLAGS) server.o $(MYSQL_LIBS)

clean: 
	rm -f *.o
	rm -f $(EXEC)

exec:
	./$(EXEC)
	