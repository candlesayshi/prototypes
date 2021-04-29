/* weave.c - a four-part delay network with feedback routing */
/*
    for file I/O this uses the libsndfile C library - http://www.mega-nerd.com/libsndfile/
    which is released under an LGPL license - https://www.gnu.org/licenses/lgpl-3.0.html
*/
#include <stdio.h>
#include <stdlib.h>
#include <sndfile.h>
#include <math.h>

#define NFRAMES (1024)      // defines the size of the read/write buffer

enum arg_list {ARG_PROGNAME,ARG_INFILE,ARG_OUTFILE,ARG_NARGS};

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

    printf("WEAVE: four-device delay with feedback crosstalk\n");

    // handle options
    if(argc > 1){
		char flag;
		while(argv[1][0] == '-'){
			flag = argv[1][1];
			switch(flag){
			case('\0'):
				printf("Error: missing flag name\n");
				return 1;
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
                "usage: weave infile outfile\n" // nothing so far
                );
        return 1;
    }

    /******* handle the arguments *******/

    infile = sf_open(argv[ARG_INFILE],SFM_READ,&info);
    if(infile == NULL){
        printf("Error opening %s\n",argv[ARG_INFILE]);
        error++;
        goto exit;
    }

    /**** necessary calculations ****/
    filesize = info.frames;                         // get number of samples in input file
    totalsamples = length_secs * info.samplerate;   // calculate size of output file

    // allocate memory for the I/O buffers
    inframe = (float*)malloc(sizeof(float) * filesize);
    if(inframe == NULL){
        printf("Error allocating memory for input.\n");
        error++;
        goto exit;
    }
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

    /**** processing loop that writes to the output ****/

    while ((framesread = sf_read_float(infile,inframe,nframes)) > 0){
	
		for(long i = 0; i < framesread;i++){
            outframe[i] = inframe[i];
        }
		if(sf_write_float(outfile,outframe,framesread) != framesread){
			printf("Error writing to outfile\n");
			error++;
			break;
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

    return 0;
}