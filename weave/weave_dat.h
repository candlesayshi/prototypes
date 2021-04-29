// a simple delay block for testing
typedef struct delay_block
{
    long writepos;
    long srate;
    unsigned long dtime;
    float* buffer;
} BLOCK;

// allocate a new delay block
BLOCK* new_block(float seconds, int srate);

// the main delay processor
float delay_tick(BLOCK* block, float input);
