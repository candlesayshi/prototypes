/* shatter_dat.c - source to hold unique functions */
#include "shatter_dat.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

// initialize audio layer
void layer_init(LAYER* curlayer, int layers, unsigned long filesize)
{
    curlayer->play = 1;
    curlayer->ampfac = (1.0 / (double)layers);
    curlayer->size = filesize;
    curlayer->index = 0;
}

// initialize and set values for new shard
void new_shard(SHARD* curshard, long* zc_array, long zc_count, long min, long max)
{
    double r1, r2;
    double start, end;
    double rand_range = ((double)zc_count / (double)RAND_MAX);

    // get first values
    r1 = rand() * rand_range;
    r2 = rand() * rand_range;

    while(fabs(r2 - r1) < min || fabs(r2 - r1) > max){
        r2 = rand() * rand_range;        
    }

    // set first and second and round to nearest integer
    start = r1 < r2 ? r1 : r2;
    start += 0.5;
    end = r2 > r1 ? r2 : r1;
    end += 0.5;

    // now put in the values
    curshard->start = (long)start;
    curshard->end = (long)end;
    curshard->index = curshard->start;
    curshard->looping = 0; // activate the shards with a separate function
}

// get the value from the layer
float layer_tick(LAYER* layer, float* inframe)
{
    float thisframe = 0.0;
    float ampfac = layer->ampfac;

    if(layer->play){
        thisframe = inframe[layer->index] * ampfac;
        layer->index += 1;
        if(layer->index > layer->size){
            layer->index = 0;
        }
    }
    
    return thisframe;
}

// layer destruction function
void destroy_layers(LAYER** thislayer, int layers){
    for(int i = 0; i > layers; i++){
        free(thislayer[i]);
    }
    free(thislayer);
}

// shard destruction function
void destroy_shards(SHARD** thisshard, int layers){
    for(int i = 0; i > layers; i++){
        free(thisshard[i]);
    }
    free(thisshard);
}