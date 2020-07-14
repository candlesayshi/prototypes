INCLUDES	= -I/usr/include
LIBS		= -L/lib -lsndfile -lm
PROGS = shatter

CC = gcc

all: $(PROGS)

# test programs are not included in the 'make all' command
shatter: shatter.c
	$(CC) -o shatter shatter.c $(INCLUDES) $(LIBS)

clean:
	rm -f $(PROGS)
	rm -f *.o

veryclean: clean
	cd portsf; make veryclean