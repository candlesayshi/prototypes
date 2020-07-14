INCLUDES	= -I/usr/include
LIBS		= -L/lib -lsndfile -lm
PROGS = shatter

# make sure to check that libsndfile is installed correctly

CC = gcc

all: $(PROGS)

shatter: shatter.c
	$(CC) -o shatter shatter.c $(INCLUDES) $(LIBS)

clean:
	rm -f $(PROGS)
	rm -f *.o

veryclean: clean
	cd portsf; make veryclean