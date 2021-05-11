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

    block->input = 0.0;
    block->output = 0.0;

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

// allocate a new weave
WEAVE* new_weave(double dtimeA, double dtimeB, int srate)
{
    // allocate the weave itself and initialize parameters
    WEAVE* weave = (WEAVE*)malloc(sizeof(WEAVE));

    weave->wetdrymix = 0.5;
    weave->inputgainA = 1.0;
    weave->inputgainB = 1.0;

    weave->delaytimeA = dtimeA;
    weave->delaytimeB = dtimeB;

    // might as well do all these at once
    weave->feedbackfromAtoA =
    weave->feedbackfromAtoB =
    weave->feedbackfromBtoA =
    weave->feedbackfromBtoB = 0.0;

    // now let's initialize the internal delay blocks
    BLOCK* blockA = (BLOCK*)malloc(sizeof(BLOCK));
    BLOCK* blockB = (BLOCK*)malloc(sizeof(BLOCK));

    blockA = new_block(weave->delaytimeA,srate);
    blockB = new_block(weave->delaytimeB,srate);

    weave->delayA = blockA;
    weave->delayB = blockB;

    return weave;

}

// weave destructor
void unravel(WEAVE* weave)
{
    if(weave){
        destroy_block(weave->delayA);
        destroy_block(weave->delayB);
        free(weave);
        weave = NULL;
    }
}

float weave_tick(WEAVE* weave, float input)
{
    float output;

    unsigned long readposA, readposB;
    float* bufA = weave->delayA->buffer;
    float* bufB = weave->delayB->buffer;

    // get read position from delaytime
    readposA = weave->delayA->writepos + weave->delayA->dtime;
    if(readposA >= weave->delayA->dtime)
        readposA -= weave->delayA->dtime;
    weave->delayA->output = bufA[readposA];

    readposB = weave->delayB->writepos + weave->delayB->dtime;
    if(readposB >= weave->delayB->dtime)
        readposB -= weave->delayB->dtime;
    weave->delayB->output = bufB[readposB];

    // write the delayed signal
    bufA[weave->delayA->writepos++] = (input);
    if(weave->delayA->writepos == weave->delayA->dtime)
        weave->delayA->writepos = 0;

    bufB[weave->delayB->writepos++] = (input);
    if(weave->delayB->writepos == weave->delayB->dtime)
        weave->delayB->writepos = 0;

    output = weave->delayA->output + weave->delayB->output;

    return output;

}