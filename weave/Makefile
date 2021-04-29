INCLUDES	= -I/usr/include
LIBS		= -L/lib -lsndfile -lm
PROGS = weave

# make sure to check that libsndfile is installed correctly

CC = gcc

all: $(PROGS)

weave: weave.c weave_dat.c
	$(CC) -o weave weave.c weave_dat.c $(INCLUDES) $(LIBS)

clean:
	rm -f $(PROGS)
	rm -f *.o