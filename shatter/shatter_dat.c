/* shatter_dat.c - source to hold unique functions */
#include "shatter_dat.h"
#include <stdio.h>
#include <stdlib.h>

// initialize audio layer
void layer_init(LAYER* curlayer, int layers, unsigned long filesize)
{
    curlayer->play = 1;
    curlayer->ampfac = (1.0 / (double)layers);
    curlayer->size = filesize;
    curlayer->index = 0;
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
