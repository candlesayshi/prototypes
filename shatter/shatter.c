/* shatter.c - opens an audio file and shatters it over a number of layers */
/*
    for file I/O this uses the libsndfile C library - http://www.mega-nerd.com/libsndfile/
    which is released under an LGPL license - https://www.gnu.org/licenses/lgpl-3.0.html
*/
#include <stdio.h>
#include <stdlib.h>
#include <sndfile.h>
#include <time.h>
#include <math.h>
#include "shatter_dat.h"

#define NFRAMES (1024)      // defines the size of the read/write buffer
#define DEFAULTMIN (62.0)   // the default minimum shard size (62ms)
#define DEFAULTMAX (495.0)  // for an override the default maximum is the full size of the track

enum arg_list {ARG_PROGNAME,ARG_INFILE,ARG_OUTFILE,ARG_LENGTH,ARG_LAYERS,ARG_NARGS};

int main(int argc, char** argv)
{
    int error = 0;

    // variables that handle the files
    SNDFILE* infile = NULL;
    SNDFILE* outfile = NULL;
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
    long zc_count = 0;              // a counter to track zero crossings for building an array
    int possible_shards = 0;        // used to calculate the number of possible shards that there are
    int zc_override = 0;            // flag to check for overriding the zero crossing check
    int near_zero_mode = 0;         // flag to change the zero crossing to a quietness detector
    double near_zero = 0.0;         // the value for what set the shards
    long* zero_crossings = NULL;    // array to hold the zero crossing locations
    long min = DEFAULTMIN;          // min and max of the shard size, should be user changable in time
    long max = DEFAULTMAX;
    float start_lim_in = 0.0;       // limit the start and end points that shards can be collected from
    float end_lim_in = 0.0;
    long start_lim = 0;
    long end_lim = 1;
    int min_override = 0;           // flags to track if the the min/max values are being overridden
    int max_override = 0;
    int change_check = 0;
    double bias = 0.75;             // sets the chance for the shards to shift to new configurations
    int tail = 1;                   // flag that determines if the tail of the layers plays after the shards deactivate
    int list_shards = 0;            // flag that enables writing the shards to the output
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
            case('n'):
                near_zero_mode = 1;
                near_zero = atof(&(argv[1][2]));
                if(near_zero < 0.0 || near_zero > 1.0){
                    printf("Near zero threshold out of range! Must be between 0.0 and 1.0.\n");
                    return 1;
                }
                break;
            case('b'):
                bias = atof(&(argv[1][2]));
                if(bias < 0.0 || bias > 1.0){
                    printf("Shift bias cannot be < 0 or > 1.\n");
                    return 1;
                }
                break;
            case('l'):
                list_shards = 1;
                break;
            case('m'):
                min_override = 1;
                min = atoi(&(argv[1][2]));
                if(min < 0 || min > max){
                    printf("Minimum shard size cannot be < 0 or > maximum.\n");
                    return 1;
                }
                break;
            case('x'):
                max_override = 1;
                max = atoi(&(argv[1][2]));
                if(max < 0 || max < min){
                    printf("Maximum shard size cannot be < minimum\n");
                    return 1;
                }
                break;
            case('s'):
                start_lim_in = atof(&(argv[1][2]));
                if(start_lim < 0.0){
                    printf("Start point limit cannot be < 0\n");
                    return 1;
                }
            case('e'):
                end_lim_in = atof(&(argv[1][2]));
                if(end_lim < 0.0){
                    printf("End point limit cannot be < 0\n");
                    return 1;
                }
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
                "usage: shatter [-options] infile outfile length layers\n"
                "options:\t-z :\tOverrides the check to split shards at zero\n"
                "\t\t\tcrossings, allowing them to split anywhere.\n"
                "\t\t-n :\tChanges the zero crossing check to use an amplitude\n"
                "\t\t\tthreshold for determining where shards split (ex. -n0.01)\n"
                "\t\t-s :\tIgnore zero crossings (or threshold points) before\n"
                "\t\t\tthis position (in seconds) (ex. -s2.3)\n"
                "\t\t-e :\tIgnore zero crossings (or threshold points) after\n"
                "\t\t\tthis position (in seconds) (ex. -e58.2)\n"
                "\t\t-t :\tStops processing after shards stop, forcing\n"
                "\t\t\tthe audio to end immediately at length.\n"
                "\t\t-l :\tLists shards when they are collected along with data\n"
                "\t\t\tabout them. (Disables output progress display)\n"
                "\t\t-b :\tSets the bias at which the shards become more\n"
                "\t\t\tlikely to change with each play (0.0 < bias < 1.0)\n"
                "\t\t\t(default bias: 0.75, 1.0 = never changes) (ex. -b0.5)\n"
                "\t\t-m :\tSets the minimum size of the shard(s) (in microseconds)\n"
                "\t\t\t(default minimum: 62 ms) (ex. -m200)\n"
                "\t\t-x :\tSets the maximum size of the shard(s) (in microseconds)\n"
                "\t\t\t(default maximum is the length of the file) (ex. -x2000)\n"
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
    end_lim = filesize = info.frames;               // get number of samples in input file
    totalsamples = length_secs * info.samplerate;   // calculate size of output file
    if(min_override){                               // calculate min and max of shard size
        min = ((double)min * 0.001) * info.samplerate;
    } else {
        min = (double)(DEFAULTMIN * 0.001) * info.samplerate;
    }
    if(max_override){
        max = ((double)max * 0.001) * info.samplerate;
    } else {
        max = end_lim;
    }
    if(start_lim_in > 0.0){
        float start_in_samps = (start_lim_in * (float)info.samplerate) + 0.5;
        start_lim = (long)start_in_samps;
    }
    if(end_lim_in > 0.0){
        float end_in_samps = (end_lim_in * (float)info.samplerate) + 0.5;
        end_lim = (long)end_in_samps;
    }
    if(end_lim <= start_lim){
        printf("Error!: End point limit is at or before the start point limit!\n");
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

    // fill the input buffer (find zero crossings later in this loop, maybe?)
    printf("Copying file to input... ");
    for(long i = 0; i < filesize; i++){
        framesread = sf_read_float(infile,&curframe,1);
        if(framesread != 1){
            printf("Error reading audio frame from input.\n");
            error++;
            goto exit;
        }
        inframe[i] = curframe;
        printf("\rCopying file to input... %.0f%% done.",((double)i / (double)filesize) * 100.0);
    }
    printf("\rCopying file to input... 100%% done.\n");

    // find zero crossings and build zero crossings array
    /*  I could probably do this at the same time as I copy the audio file into the
        buffer, but I was having problems with the zero_crossings array. So, I'm
        keeping it compartmentalizing it all for now */
    if(zc_override){
        for(long i = start_lim; i < end_lim; i++){
            /* there's obviously a much more memory efficent way to 
               do this, but I'm implementing this so far to check the sound.
               Post-listen: after a few tests the end result doesn't really
               sound much different if at all. So, I expect for this to be
               used rarely. Might come back to it. */
            zero_crossings = (long*)realloc(zero_crossings,sizeof(long) * ++zc_count);
            zero_crossings[zc_count-1] = i;
        }
    } else if (near_zero_mode){
        printf("Scanning for near zero points... ");
        for(long i = start_lim; i < end_lim; i++){
            double current_value = fabs(inframe[i]);
            if(current_value <= near_zero){
                zero_crossings = (long*)realloc(zero_crossings,sizeof(long) * (++zc_count + 1)); // with guard point
                zero_crossings[zc_count-1] = i;
            }
            printf("\rScanning for near zero points... %ld found.",zc_count);
        }
        printf("\rScanning for near zero points... %ld found.\n",zc_count);
    } else {
        printf("Scanning for zero crossings... ");
        for(long i = 0; i < end_lim; i++){
            if(inframe[i] == 0.0){
                zero_crossings = (long*)realloc(zero_crossings,sizeof(long) * (++zc_count + 1)); // with guard point
                zero_crossings[zc_count-1] = i;
            }
            printf("\rScanning for zero crossings... %ld found.",zc_count);
        }
        printf("\rScanning for zero crossings... %ld found.\n",zc_count);
    }
    if(end_lim != filesize){
        zero_crossings[zc_count] = zero_crossings[zc_count-1]; // just make sure it's a zc
    } else
        zero_crossings[zc_count] = end_lim; // make last value the end of file
    

    printf("Shattering input... ");
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
    if(list_shards) printf("Collecting first shards...\n");

    // build and prepare the shards
    curshard = (SHARD**)malloc(sizeof(SHARD*) * layers);
    for(int i = 0; i < layers; i++){
        curshard[i] = (SHARD*)malloc(sizeof(SHARD));
        new_shard(curshard[i],zero_crossings,zc_count,min,max);
        if(list_shards){
            observe_shard(i,curshard[i],info.samplerate);
        }
        activate_shard(curshard[i]);
    }
    printf("Done.\n");

    // this way of tracking the number of possible shards could be used 
    // as a better method for creating shards... will give it thought
    for(int i = 0; i < zc_count; i++){
        long base = zero_crossings[i];
        int j = 1;
        while((zero_crossings[i + j] - zero_crossings[i]) < min && (i + j) < zc_count)
            j++;
        for(;(i + j) < zc_count; j++){
            if((zero_crossings[i + j] - zero_crossings[i]) > max) break;
            possible_shards++;
        }
    }

    printf("Shattered into %d possible shard(s)...\n",possible_shards);

    /**** get the output ready ****/
    outframe = (float*)malloc(sizeof(float) * nframes * info.channels);
    if(outframe == NULL){
        printf("Error allocating memory for output.\n");
        error++;
        goto exit;
    }
    // if that's all okay, create the output file
    outfile = sf_open(argv[ARG_OUTFILE],SFM_WRITE,&info);
    if(outfile == NULL){
        printf("Error creating file: %s\n",argv[ARG_OUTFILE]);
        error++;
        goto exit;
    }
    if(list_shards)
        printf("Writing output...\n");
    /**** processing loop that writes to the output ****/
    while(frameswrite < totalsamples){
        for(long i = 0; i < nframes; i++){
            float curframe = 0.0;
            for(int j = 0; j < layers; j++){
                change_check = 0;
                curframe += shard_tick(curlayer[j],curshard[j],inframe,&change_check,bias);
                if(change_check == 1){
                    new_shard(curshard[j],zero_crossings,zc_count,min,max);
                    if(list_shards) observe_shard(j,curshard[j],info.samplerate);
                    activate_shard(curshard[j]);
                }
            }
            outframe[i] = curframe;
        }
        if(!list_shards)
            printf("\rWriting output... %.0f%% done.",((double)frameswrite / (double)totalsamples) * 100.0);
        frameswrite += sf_write_float(outfile,outframe,nframes);
    }
    printf("\nCleaning shards... ");
    if(tail){
        deactivate_all_shards(curshard,layers);
        while(stopped_layers < layers){
            for(long i = 0; i < nframes; i++){
                float curframe = 0.0;
                stopped_layers = 0;
                for(int j = 0; j < layers; j++){
                    curframe += shard_tick(curlayer[j],curshard[j],inframe,0,1);
                    if(curlayer[j]->play == 0) stopped_layers++;
                }
                outframe[i] = curframe;
            }
            frameswrite += sf_write_float(outfile,outframe,nframes);
        }
    }


    printf("Done.\nOutput saved to %s\n",argv[ARG_OUTFILE]);

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