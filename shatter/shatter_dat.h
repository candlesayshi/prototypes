typedef struct shard
{
    int looping;            // a flag to check whether the shard is currently looping
    unsigned long start;    // the start point of the current shard
    unsigned long end;      // the end point of the current shard
    unsigned long index;    // current position of the shard
} SHARD;

typedef struct layer
{
    int play;               // a flag to determine if the layer is active
    unsigned long size;     // the number of frames in the audio file
    double ampfac;          // amplitude of the layer - usually 1.0/(number of layers)
    unsigned long index;    // position in the audio file
} LAYER;

// initalize audio layer
void layer_init(LAYER* curlayer, int layers, unsigned long filesize);

// get the value from the layer
float layer_tick(LAYER* layer, float* inframe);

// layer destruction function
void destroy_layers(LAYER** thislayer, int layers);