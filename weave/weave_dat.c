#include <stdlib.h>
#include <stdio.h>
#include "weave_dat.h"

/************************ DELAY BLOCK FUNCTIONS ************************************/

// allocate a new delay block
BLOCK* new_block(float seconds, int srate, double igain)
{
    BLOCK* block = (BLOCK*)malloc(sizeof(BLOCK));
    if(block == NULL)
        return NULL;
    block->dtime = (unsigned long)(seconds * srate);
    block->buffer = (float*)malloc(sizeof(float) * block->dtime);
    if(block->buffer == NULL)
        return NULL;
    block->srate = srate;
    block->input_gain = igain;
    block->writepos = 0;
    block->curinput = block->curoutput = 0.0;

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
    buf[block->writepos++] = (input * block->input_gain) + (fblevel * output);
    if(block->writepos == block->dtime)
        block->writepos = 0;

    return output;

}

// the delay playhead
float delay_read(BLOCK* block, float input)
{
    unsigned long readpos;
    float output;
    float* buf = block->buffer;

    // get the playhead position from the delaytime
    readpos = block->writepos + block->dtime;
    if(readpos >= block->dtime)
        readpos -= block->dtime;
    output = buf[readpos];

    block->curinput = input;
    block->curoutput = output;

    return output;
}

// the delay recordhead
void delay_write(BLOCK* block, double fblevel)
{
    float* buf = block->buffer;
    float input = block->curinput;
    float output = block->curoutput;

    buf[block->writepos++] = (input * block->input_gain) + (fblevel * output);
    if(block->writepos == block->dtime)
        block->writepos = 0;
}

/***************************** WEAVE NETWORK FUNCTIONS *************************************/

// create a new weave network
WEAVE* new_weave(int blocks, int srate, double* dtimes, double* igains, double** fblevels)
{
    // check the initial values
    if(dtimes == NULL || igains == NULL || fblevels == NULL){
        printf("Problem with value arrays.\n");
        return NULL;
    }

    // allocate space for the network
    WEAVE* weave = (WEAVE*)malloc(sizeof(WEAVE));
    if(weave == NULL)
        return NULL;
    weave->blocks = blocks;
    weave->delay = (BLOCK*)malloc(sizeof(BLOCK*) * blocks);
    if(weave->delay == NULL)
        return NULL;
    weave->feedback_level = (double*)malloc(sizeof(double) * blocks * blocks);
    if(weave->feedback_level == NULL)
        return NULL;
    weave->fb_mixes = (float*)malloc(sizeof(float) * blocks);
    if(weave->fb_mixes == NULL)
        return NULL;
    for(int i = 0; i < blocks; i++){
        weave->delay[i] = new_block(dtimes[i],srate,igains[i]);
        for(int j = 0; j < blocks; j++){
            weave->feedback_level[i][j] = fblevels[i][j];
        }
        weave->fb_mixes[i] = 0.0;
    }

    return weave;

}

// destroy everything
void unravel(WEAVE* weave)
{
    if(weave){
        int eraser = weave->blocks - 1;
        for(int i = eraser; i >= 0; i--){
            // the delays have their own destructor soooo....
            destroy_block(weave->delay[i]);

            /* I only made one malloc call for all the feedback levels
                so, we'll see if I actually need this
            for(int j = eraser; j >= 0; j--){
                if(weave->feedback_level[i][j]){
                    free(weave->feedback_level[i][j]);
                    weave->feedback_level[i][j] = NULL;
                }
            }
            if(weave->feedback_level[i]){
                free(weave->feedback_level[i]);
                weave->feedback_level[i] = NULL;
            }
            */

        }
        if(weave->fb_mixes){
            free(weave->fb_mixes);
            weave->fb_mixes = NULL;
        }
        if(weave->feedback_level){
            free(weave->feedback_level);
            weave->feedback_level = NULL;
        }
        if(weave->delay){
            free(weave->delay);
            weave->delay = NULL;
        }
        free(weave);
        weave = NULL;
    }
    // phew fuck
}