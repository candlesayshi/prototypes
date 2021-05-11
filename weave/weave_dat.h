// a simple delay block for testing
typedef struct delay_block
{
    long srate;                 // sample rate is used to calc delaytime

    unsigned long dtime;        // the delay time in samples
    long writepos;              // the position of record head
    float* buffer;              // buffer to store the delay

    double input;               // trying this to build mixes within the delay block
    double output;
} BLOCK;

typedef struct delay_network
{
    BLOCK* delayA;              // building the delay blocks
    BLOCK* delayB;

    double wetdrymix;

    // parameters for delay block A
    double inputgainA;
    double feedbackfromAtoA;
    double feedbackfromBtoA;
    double delaytimeA;

    // parameters for delay block B
    double inputgainB;
    double feedbackfromAtoB;
    double feedbackfromBtoB;
    double delaytimeB;

} WEAVE;


// allocate a new delay block
BLOCK* new_block(float seconds, int srate);

// destroy a delay block
void destroy_block(BLOCK* block);

// the main delay processor
float delay_tick(BLOCK* block, float input, double fblevel);

// allocate a new weave
WEAVE* new_weave(double dtimeA, double dtimeB, int srate);

// read the delay block signal
float block_read(BLOCK* block);

// write into the delay
void block_write(BLOCK* block, float input);

// weave destructor
void unravel(WEAVE* weave);

// the main effect process
float weave_tick(WEAVE* weave, float input);

// push a default patch to the weave
void weave_default(WEAVE* weave, int srate);