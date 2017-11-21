CC=gcc
EXEC = client
%.o: %.c $(DEPS)
	$(CC) -c -o $@ $<

server: client.o
	$(CC) -o $(EXEC) client.o

clean: 
	rm -f *.o
	rm -f $(EXEC)

exec:
	./$(EXEC)