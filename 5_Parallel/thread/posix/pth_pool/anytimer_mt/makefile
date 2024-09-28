CFLAGS+=-pthread
LDFLAGS+=-pthread



all: anytimer

anytimer: main.o anytimer.o
	gcc $^ -o $@ $(CFLAGS) $(LDFLAGS)

clean:
	rm -rf *.o anytimer
