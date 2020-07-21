/* shatter.c - opens an audio file and shatters it over a number of layers */
/*
    for file I/O this uses the libsndfile C library - http://www.mega-nerd.com/libsndfile/
    which is released under an LGPL license - https://www.gnu.org/licenses/lgpl-3.0.html
*/
#include <stdio.h>
#include <stdlib.h>
#include <sndfile.h>
#include <time.h>
#include "shatter_dat.h"

#define NFRAMES (1024) // defines the size of the read/write buffer
#define DEFAULTMIN (62.0) // the default minimum shard size (62ms)
#define DEFAULTMAX (495.0) // for an override the default maximum is the full size of the track

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
    long framesread = 0;
    long frameswrite = 0;
    long totalsamples;

    // variable that handle the layers and shards
    int layers;
    int stopped_layers = 0;
    long zc_count = 0; // a counter to track zero crossings for building an array
    int zc_override = 0; // flag to check for overriding the zero crossing check
    long* zero_crossings = NULL; // array to hold the zero crossing locations
    long min; // min and max of the shard size, should be user changable in time
    long max = DEFAULTMAX;
    int min_override = 0;   // flags to track if the the min/max values are being overridden
    int max_override = 0;
    int tail = 1;   // flag that determines if the tail of the layers plays after the shards deactivate
    LAYER** curlayer = NULL;
    SHARD** curshard = NULL;

    printf("SHATTER: shatters an audio file over a number of layers\n");

    // handle options
    if(argc > 1){
		char flag;
		while(argv[1][0] == '-'){
			flag = argv[1][1];
			switch(flag){
			case('\0'):
				printf("Error: missing flag name\n");
				return 1;
            case('z'):
                zc_override = 1;
                break;
            case('t'):
                tail = 0;
                break;
			default:
				break;
			}
			argc--;
			argv++;
		}
	}

    // usage message
    if(argc != ARG_NARGS){
        printf( "Insufficent arguments.\n"
                "usage: shatter [-options] infile length layers\n"
                "options:\t-z :\tOverrides the check to split shards at zero\n"
                "\t\t\tcrossings, allowing them to split anywhere.\n"
                "\t\t-t :\tStops processing after shards stop, forcing\n"
                "\t\t\tthe audio to end immediately at length.\n"
                );
        return 1;
    }

    // seed the randomness
    srand(time(NULL));

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

    if(info.channels > 1){
        printf("Currently only mono soundfiles are supported.\n");
        error++;
        goto exit;
    }

    /**** necessary calculations ****/
    filesize = info.frames;                         // get number of samples in input file
    totalsamples = length_secs * info.samplerate;   // calculate size of output file
    if(min_override){                               // calculate min and max of shard size
        min = ((double)min * 0.001) * info.samplerate;
    } else {
        min = (double)(DEFAULTMIN * 0.001) * info.samplerate;
    }
    if(max_override){
        max = ((double)max * 0.001) * info.samplerate;
    } else {
        max = filesize;
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
    if(zc_override){
        for(long i = 0; i < filesize; i++){
            /* there's obviously a much more memory efficent way to 
               do this, but I'm implementing this so far to check the sound.
               Post-listen: after a few tests the end result doesn't really
               sound much different if at all. So, I expect for this to be
               used rarely. Might come back to it. */
            zero_crossings = (long*)realloc(zero_crossings,sizeof(long) * ++zc_count);
            zero_crossings[zc_count-1] = i;
        }
    } else {
        for(long i = 0; i < filesize; i++){
            if(inframe[i] == 0.0){
                zero_crossings = (long*)realloc(zero_crossings,sizeof(long) * ++zc_count);
                zero_crossings[zc_count-1] = i;
            }
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

    // build and prepare the shards
    curshard = (SHARD**)malloc(sizeof(SHARD*) * layers);
    for(int i = 0; i < layers; i++){
        curshard[i] = (SHARD*)malloc(sizeof(SHARD));
        new_shard(curshard[i],zero_crossings,zc_count,min,max);
        activate_shard(curshard[i]);
    }

    // DEBUG to check the values of the shards
    /*
    for(int i = 0; i < layers; i++){
        printf("SHARD %d:\n"
                "\tstart sample =\t%ld\n"
                "\tend sample =\t%ld\n",i,curshard[i]->start,curshard[i]->end);
    }
    */

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

    /**** processing loop that writes to the output ****/
    while(frameswrite < totalsamples){
        for(long i = 0; i < nframes; i++){
            float curframe = 0.0;
            for(int j = 0; j < layers; j++){
                // curframe += layer_tick(curlayer[j],inframe);
                curframe += layer_shard_tick(curlayer[j],curshard[j],inframe);
            }
            outframe[i] = curframe;
        }
        frameswrite += sf_write_float(outfile,outframe,nframes);
    }

    if(tail){
        deactivate_all_shards(curshard,layers);
        while(stopped_layers < layers){
            for(long i = 0; i < nframes; i++){
                float curframe = 0.0;
                stopped_layers = 0;
                for(int j = 0; j < layers; j++){
                    curframe += layer_shard_tick(curlayer[j],curshard[j],inframe);
                    if(curlayer[j]->play == 0) stopped_layers++;
                }
                outframe[i] = curframe;
            }
            frameswrite += sf_write_float(outfile,outframe,nframes);
        }
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
    if(curshard){
        destroy_shards(curshard,layers);
    }
    if(zero_crossings) free(zero_crossings);

    return 0;
}