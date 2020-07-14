typedef struct shard
{
    int looping;            // a flag to check whether the shard is currently looping
    unsigned long start;    // the start point of the current shard
    unsigned long end;      // the end point of the current shard
    unsigned long index;    // current position of the shard
} SHARD;

typedef struct layer
{
    unsigned long size;     // the number of frames in the audio file
    double amp;             // amplitude of the layer - usually (global amp)/(number of layers)
    unsigned long index;    // position in the audio file
} LAYER;

