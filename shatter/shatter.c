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
    float* inframe = NULL;
    float* outframe = NULL;
    float curframe;
    int nframes = NFRAMES;
    long framesread;
    long frameswrite;
    long totalsamples;

    // variable that handle the layers and shards
    int layers;
    LAYER** curlayer;
    SHARD** curshard;

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

    // get filesize
    filesize = info.frames;

    // calculate size of output file
    totalsamples = length_secs * info.samplerate;

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
        inframe[i] = curframe;
    }

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

        }
        frameswrite += sf_write_float(outfile,&outframe,nframes);
    }

    printf("Done. Output saved to %s\n",filename);

    exit:

    if(infile){
        if(sf_close(infile)){
            printf("Error closing %s\n",argv[ARG_INFILE]);
        }
    }
    if(outfile){
        if(sf_close(outfile)){
            printf("Error closing output file.\n");
        }
    }
    if(inframe) free(inframe);
    if(outframe) free(outframe);

    return 0;
}