/* shatter_dat.c - source to hold unique functions */
#include "shatter_dat.h"

LAYER layer_init(int layers, unsigned long filesize)
{
    LAYER layer;
    layer.play = 1;
    layer.ampfac = 1.0 / (double) layers;
    layer.size = filesize;
    layer.index = 0;

    return layer;
}
