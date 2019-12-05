EXE := test

%.o: %.c
	$(CC) -g -c $< -o $@

SOURCE := $(wildcard *.c)
OBJS   := $(patsubst %.c,%.o,$(SOURCE))

$(EXE):$(OBJS)
	$(CC) -g -o ./$(EXE) $(OBJS)

clean:
	$(RM) -f *.o $(EXE)
