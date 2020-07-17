/* shatter.c - opens an audio file and shatters it over a number of layers */
/*
    for file I/O this uses the libsndfile C library - http://www.mega-nerd.com/libsndfile/
    which is released under an LGPL license - https://www.gnu.org/licenses/lgpl-3.0.html
*/
#include <stdio.h>
#include <stdlib.h>
#include <sndfile.h>
#include "shatter_dat.h"

#define NFRAMES (1024); // defines the size of the read/write buffer

enum arg_list {ARG_PROGNAME,ARG_INFILE,ARG_LENGTH,ARG_LAYERS,ARG_NARGS};

int main(int argc, char** argv)
{
    int error = 0;

    // variables that handle the files
    SNDFILE* infile = NULL;
    SNDFILE* outfile = NULL;
    char filename[] = {"shattered-output.wav"};
    SF_INFO info;
    unsigned long filesize;
    double length_secs;

    // variables that handle the read/write buffers
    float* inframe;
    float* outframe;
    float curframe;
    int nframes = NFRAMES;
    long framesread;
    long frameswrite;
    long totalsamples;

    // variable that handle the layers and shards
    int layers;
    long zc_count = 0; // a counter to track zero crossings for building an array
    long* zero_crossings = NULL; // array to hold the zero crossing locations
    long min; // min and max of the shard size, should be user changable in time
    long max;
    LAYER** curlayer = NULL;
    SHARD** curshard = NULL;

    printf("SHATTER: shatters an audio file over a number of layers\n");
    if(argc != ARG_NARGS){
        printf( "Insufficent arguments.\n"
                "usage: shatter infile length_in_seconds layers\n");
        return 1;
    }

    /******* handle the arguments *******/

    infile = sf_open(argv[ARG_INFILE],SFM_READ,&info);
    if(infile == NULL){
        printf("Error opening %s\n",argv[ARG_INFILE]);
        error++;
        goto exit;
    }
    length_secs = atof(argv[ARG_LENGTH]);
    if(length_secs < 0.0){
        printf("Length cannot be less than 0.0 seconds.\n");
        error++;
        goto exit;
    }
    layers = atoi(argv[ARG_LAYERS]);
    if(layers <= 0){
        printf("Number of layers cannot be 0 or less.\n");
        error++;
        goto exit;
    }

    /**** necessary calculations ****/
    filesize = info.frames;                         // get number of samples in input file
    totalsamples = length_secs * info.samplerate;   // calculate size of output file
    min = ((double)min * 0.001) * info.samplerate;  // calculate min and max of shard size
    max = ((double)max * 0.001) * info.samplerate;
    if(info.channels > 1){
        printf("Currently only mono soundfiles are supported.\n");
        error++;
        goto exit;
    }

    // allocate memory for the I/O buffers
    inframe = (float*)malloc(sizeof(float) * filesize);
    if(inframe == NULL){
        printf("Error allocating memory for input.\n");
        error++;
        goto exit;
    }

    // fill the input buffer (find zero crossings later in this loop)
    for(long i = 0; i < filesize; i++){
        framesread = sf_read_float(infile,&curframe,1);
        if(framesread != 1){
            printf("Error reading audio frame from input.\n");
            error++;
            goto exit;
        }
        inframe[i] = curframe;
    }

    // find zero crossings and build zero crossings array
    /*  I could probably do this at the same time as I copy the audio file into the
        buffer, but I was having problems with the zero_crossings array. So, I'm
        keeping it compartmentalizing it all for now */
    for(long i = 0; i < filesize; i++){
        if(inframe[i] == 0.0){
            zero_crossings = (long*)realloc(zero_crossings,sizeof(long) * ++zc_count);
            zero_crossings[zc_count-1] = i;
        }
    }
    zero_crossings[zc_count] = filesize; // make last value the end of file

    // build the layers
    curlayer = (LAYER**)malloc(sizeof(LAYER*) * layers);
    for(int i = 0; i < layers; i++){
        curlayer[i] = (LAYER*)malloc(sizeof(LAYER));
        layer_init(curlayer[i],layers,filesize);
        if(curlayer[i] == NULL){
            printf("Error creating audio layer.\n");
            error++;
            goto exit;
        }
    }

    /**** get the output ready ****/
    outframe = (float*)malloc(sizeof(float) * nframes * info.channels);
    if(outframe == NULL){
        printf("Error allocating memory for output.\n");
        error++;
        goto exit;
    }
    // if that's all okay, create the output file
    outfile = sf_open(filename,SFM_WRITE,&info);
    if(outfile == NULL){
        printf("Error creating file: %s\n",filename);
        error++;
        goto exit;
    }

    while(frameswrite < totalsamples){
        for(long i = 0; i < nframes; i++){
            float curframe = 0.0;
            for(int j = 0; j < layers; j++){
                curframe += layer_tick(curlayer[j],inframe);
            }
            outframe[i] = curframe;
        }
        frameswrite += sf_write_float(outfile,outframe,nframes);
    }

    printf("Done. Output saved to %s\n",filename);

exit:
    if(error){
        printf("%d error(s)\n",error);
    }
    if(outfile){
        if(sf_close(outfile)){
            printf("Error closing output file.\n");
        }
    }
    if(infile){
        if(sf_close(infile)){
            printf("Error closing %s\n",argv[ARG_INFILE]);
        }
    }
    if(inframe)  free(inframe);
    if(outframe) free(outframe);
    if(curlayer){
        destroy_layers(curlayer,layers);
    }
    if(curshard) free(curshard);
    if(zero_crossings) free(zero_crossings);

    return 0;
}