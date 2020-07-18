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
    long start, end, val1, val2;
    double rand_range = ((double)zc_count / (double)RAND_MAX);

    // get first values
    r1 = rand() * rand_range;
    val1 = zc_array[(int)(r1 + 0.5)];
    r2 = rand() * rand_range;
    val2 = zc_array[(int)(r2 + 0.5)];

    while(abs(val2 - val1) < min || abs(val2 - val1) > max){
        r1 = rand() * rand_range;
        val1 = zc_array[(int)(r1 + 0.5)];
        r2 = rand() * rand_range;
        val2 = zc_array[(int)(r2 + 0.5)];     
    }

    // set first and second and round to nearest integer
    start = (r1 < r2 ? r1 : r2) + 0.5;
    end = (r2 > r1 ? r2 : r1) + 0.5;

    // now put in the values
    curshard->start = zc_array[start];
    curshard->end = zc_array[end];
    curshard->looping = 0; // activate the shards with a separate function
}

// set the current shard to start
void activate_shard(SHARD* curshard)
{
    curshard->looping = 1;
}

// activates all shards at once
void activate_all_shards(SHARD** shardarray, int layers){
    for(int i = 0; i < layers; i++){
        shardarray[i]->looping = 1;
    }
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

// get the value from the layer/shard
float layer_shard_tick(LAYER* layer, SHARD* shard, float* inframe)
{
    float thisframe = 0.0;
    float ampfac = layer->ampfac;

    if(layer->play = 1){
        if(shard->looping = 1 && layer->index > shard->end){
            layer->index = shard->start;
        }
        if(layer->index > layer->size){
            layer->index = 0;
        }
        thisframe = inframe[layer->index] * ampfac;
        layer->index += 1;
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