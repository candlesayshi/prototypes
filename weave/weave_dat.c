#include <stdlib.h>
#include <stdio.h>
#include "weave_dat.h"

/************************ DELAY BLOCK FUNCTIONS ************************************/

// allocate a new delay block
BLOCK* new_block(float seconds, int srate)
{
    BLOCK* block = (BLOCK*)malloc(sizeof(BLOCK));
    if(block == NULL)
        return NULL;
    block->dtime = (unsigned long)(seconds * srate);
    block->buffer = (float*)malloc(sizeof(float) * block->dtime);
    if(block->buffer == NULL)
        return NULL;
    block->srate = srate;
    block->writepos = 0;

    return block;
}

// destroy a delay block
void destroy_block(BLOCK* block)
{
    if(block){
        if(block->buffer)
            free(block->buffer);
            block->buffer = NULL;
        free(block);
        block = NULL;
    }
    
}

// the main delay processor
float delay_tick(BLOCK* block, float input, double fblevel)
{
    unsigned long readpos;
    float output;
    float* buf = block->buffer;

    // get the read position from the delaytime
    readpos = block->writepos + block->dtime;
    if(readpos >= block->dtime)
        readpos -= block->dtime;
    output = buf[readpos];

    // write the delayed signal
    buf[block->writepos++] = (input) + (fblevel * output);
    if(block->writepos == block->dtime)
        block->writepos = 0;

    return output;

}

/***************************** WEAVE NETWORK FUNCTIONS *************************************/

// deleted for now - needs a big refactor, but I do have it diagrammed