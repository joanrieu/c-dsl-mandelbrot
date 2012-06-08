LDFLAGS+=-lSDL
mandelbrot: mandelbrot.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
