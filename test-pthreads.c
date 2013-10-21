#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <openjpeg.h>
#include <string.h>
#include <sys/time.h>
#include <jpeglib.h>
#include "lib/url2cache.h"
#include "lib/opj_res.h"
#include "lib/opj2png.h"

typedef struct thr_res {
        JSAMPLE *rgb;
        unsigned done;
} thr_res_t;

typedef struct thr_arg {
        opj_dparameters_t *params;
        thr_res_t *tres;
        unsigned tile_index_start;
        unsigned tile_index_end;
        char *cachefile;
}  thr_args_t;

void *processTile(void *args) {
        unsigned tile_index_start = ((thr_args_t*)args)->tile_index_start;
        unsigned tile_index_end = ((thr_args_t*)args)->tile_index_end;
        unsigned tile_index;
        for(tile_index = tile_index_start; tile_index < tile_index_end; tile_index++) {
                opj_dparameters_t *params = ((thr_args_t*)args)->params;
                struct opj_res res = opj_init(((thr_args_t*)args)->cachefile, params);                      
                opj_get_decoded_tile(res.l_codec, res.l_stream, res.image, tile_index);
                opj_cleanup(&res);
        } 	
        ((thr_args_t*)args)->tres->done++;
        pthread_exit(NULL);
}

int main(int argc, char **argv) {
        if(argc < 2) { return; }
        char *cachefile = download_to_cache(argv[1], ".");
        struct opj_res resources;

	opj_dparameters_t parameters;
	opj_set_default_decoder_parameters(&parameters);
        parameters.cp_reduce = 5;
        resources = opj_init(cachefile, &parameters);
	opj_codestream_info_v2_t* info = opj_get_cstr_info(resources.l_codec);


        unsigned tilesX = info->tw;
        unsigned tilesY = info->th;
        unsigned n_thr = 3;
        unsigned x, y, n = 0;
        pthread_t t[n_thr];

        struct thr_res tres;
        tres.done = 0;
        tres.rgb = malloc(sizeof(JSAMPLE) * ((tilesX * tilesY * 16) * resources.image->numcomps));

        printf("%d / %d\n", tilesX, tilesY);
        for(n = 0; n < n_thr; n++) {
                thr_args_t args;
                args.tres = &tres;
                args.cachefile = cachefile;
                args.tile_index_start = (n * tilesX * tilesY) / n_thr;
                args.tile_index_end = ((n+1) * tilesX * tilesY) / n_thr;
                printf("%d-%d\n", args.tile_index_start, args.tile_index_end);	
                args.params = &parameters;
                pthread_create(&t[n], NULL, processTile, &args);                        
        }
        while(tres.done < n_thr) { }

        free(tres.rgb);
        opj_destroy_cstr_info(&info);
        opj_cleanup(&resources);
        return 0;
}
