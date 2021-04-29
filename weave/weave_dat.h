// a simple delay block for testing
typedef struct delay_block
{
    double input_gain;          // gain added/removed to input level
    long srate;                 // sample rate is used to calc delaytime

    unsigned long dtime;        // the delay time in samples
    long writepos;              // the position of record head
    float* buffer;              // buffer to store the delay

    float curinput, curoutput;  // to carry values between functions

} BLOCK;

typedef struct delay_network
{
    int blocks;                 // the number of delay blocks
    BLOCK** delay;              // building the delay blocks as an array
    double** feedback_level;    // the feedback levels as a two-dimensional array (x = delay index, y = fb input index)
    float* fb_mixes;            // the feedback mixes to send to each block

} WEAVE;


// allocate a new delay block
BLOCK* new_block(float seconds, int srate, double igain);

// destroy a delay block
void destroy_block(BLOCK* block);

// the main delay processor
float delay_tick(BLOCK* block, float input, double fblevel);

// the delay playhead
float delay_read(BLOCK* block, float input);

// the delay recordhead
void delay_write(BLOCK* block, double fblevel);

// create a new weave network
WEAVE* new_weave(int blocks, int srate, double* dtimes, double* igains, double** fblevels);
