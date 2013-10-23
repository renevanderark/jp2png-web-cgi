#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <openjpeg.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <jpeglib.h>
#include "lib/url2cache.h"
#include "lib/opj_res.h"
#include "lib/opj2png.h"


typedef struct chunked_scanline {
	JSAMPLE *rgb;
	unsigned n_chunks;
	unsigned chunks_done;
} chunked_scanline_t;

typedef struct thr_res {
		unsigned done;
		unsigned tilesX;
		unsigned tilesY;
		unsigned num_comps;
		OPJ_UINT32 tw;
		OPJ_UINT32 th;
//		JSAMPLE **rgb;
		chunked_scanline_t *scanlines;
} thr_res_t;



typedef struct thr_arg {
		opj_dparameters_t *params;
		thr_res_t *tres;
		unsigned tile_index_start;
		unsigned tile_index_end;
		char *cachefile;
} thr_args_t;



static void write_scanlines(opj_image_t *image, unsigned tile_index, thr_res_t *tres) {
	unsigned tileY = tile_index / tres->tilesX;
	unsigned tileX = tile_index - (tres->tilesX * tileY);
	unsigned xPos = tileX * tres->tw;
	unsigned yPos = tileY * tres->th;

/*	printf("write scanlines from: %dpx / %dpx\n", xPos, yPos);
	printf("to: %dpx %dpx\n", xPos + image->comps[0].w, yPos+ image->comps[0].h);*/
	unsigned x, y, i = 0;
	
	for(y = yPos; y < yPos + image->comps[0].h; y++) {
		for(x = xPos; x < xPos + image->comps[0].w; x++, i++) {
//			printf("(%d/%d)\n", x, y);

			if(tres->num_comps < 3) {
				tres->scanlines[y].rgb[x] = image->comps[0].data[i];
			} else {
				tres->scanlines[y].rgb[x*tres->num_comps] = image->comps[0].data[i];
				tres->scanlines[y].rgb[x*tres->num_comps+1] = image->comps[1].data[i];
				tres->scanlines[y].rgb[x*tres->num_comps+2] = image->comps[2].data[i];
			}
//			printf("\t--> (%d / %d / %d)\n", tres->rgb[y][x*tres->num_comps], tres->rgb[y][x*tres->num_comps + 1], tres->rgb[y][x*tres->num_comps + 2]);
		}
		tres->scanlines[y].chunks_done++;
	}
//	printf("FINE\n");
}

static void *processTile(void *args) {
		unsigned tile_index_start = ((thr_args_t*)args)->tile_index_start;
		unsigned tile_index_end = ((thr_args_t*)args)->tile_index_end;
		unsigned tile_index;
		for(tile_index = tile_index_start; tile_index < tile_index_end; tile_index++) {

/*				char *outfile = malloc(sizeof(char) * 21);
				sprintf(outfile, "output/tile-%03d.jpg", tile_index);
				FILE *fp = fopen(outfile, "wb");*/

				opj_dparameters_t *params = ((thr_args_t*)args)->params;
				struct opj_res res = opj_init(((thr_args_t*)args)->cachefile, params);
				opj_get_decoded_tile(res.l_codec, res.l_stream, res.image, tile_index);

				write_scanlines(res.image, tile_index, ((thr_args_t*)args)->tres);

/*				writeJPEG(&res, 0, 0, 0, 0, 3, fp);
				fclose(fp);
				free(outfile);*/

				opj_cleanup(&res);
		}
		((thr_args_t*)args)->tres->done++;
		pthread_exit(NULL);
}


static OPJ_UINT32 reduce( OPJ_UINT32 val, OPJ_UINT32 redux ) {
	double v = (double) val;
	while(redux-- > 0) {
		v /= 2.0;
	}
	return (OPJ_UINT32) ceil(v);
}

int main(int argc, char **argv) {
		if(argc < 3) { return 0; }
		char *cachefile = download_to_cache(argv[3], ".");
		struct opj_res resources;

		opj_dparameters_t parameters;
		opj_set_default_decoder_parameters(&parameters);
		parameters.cp_reduce = strtol(argv[2], NULL, 0);
		resources = opj_init(cachefile, &parameters);
		opj_codestream_info_v2_t* info = opj_get_cstr_info(resources.l_codec);


		struct thr_res tres;

		tres.tilesX = info->tw;
		tres.tilesY = info->th;
		tres.tw = reduce(info->tdx, parameters.cp_reduce);
		tres.th = reduce(info->tdy, parameters.cp_reduce);

		unsigned y1=reduce(resources.image->y1, parameters.cp_reduce);
		unsigned x1=reduce(resources.image->x1, parameters.cp_reduce);

		unsigned n_thr = strtol(argv[1], NULL, 0);
		unsigned n = 0;
		pthread_t t[n_thr];
		thr_args_t args[n_thr];

		tres.done = 0;
		tres.num_comps = resources.image->numcomps;
		tres.scanlines = malloc(sizeof(chunked_scanline_t) * (y1+1));
		unsigned y, x;
		for(y = 0; y < y1; y++) {
			tres.scanlines[y].chunks_done = 0;
			tres.scanlines[y].n_chunks = x1 / tres.tw;
			tres.scanlines[y].rgb = malloc(sizeof(JSAMPLE) * (x1+1) * tres.num_comps);
		}

		opj_destroy_cstr_info(&info);
		opj_cleanup(&resources);
		printf("TILES: %d / %d\n",tres.tilesX, tres.tilesY);
		printf("TILE DIMS: %dpx / %dpx\n", tres.tw, tres.th);
		printf("IMAGE DIMS: %dpx / %dpx\n", x1, y1);

		for(n = 0; n < n_thr; n++) {
				args[n].tres = &tres;
				args[n].cachefile = cachefile;
				args[n].tile_index_start = (n * tres.tilesX * tres.tilesY) / n_thr;
				args[n].tile_index_end = ((n+1) * tres.tilesX * tres.tilesY) / n_thr;
//				printf("%d-%d\n", args[n].tile_index_start, args[n].tile_index_end);
				args[n].params = &parameters;
				pthread_create(&t[n], NULL, processTile, &args[n]);
//				processTile(&args[n]);
		}

		y = 0;
		while(tres.done < n_thr) { 
			if(tres.scanlines[y].chunks_done == tres.scanlines[y].n_chunks) {
				printf("line: %d is done\n", y++);
			}

		}

		for(y = 0; y < y1; y++) {
//			printf("(\n");
			for(x = 0; x < x1; x++) {
//				printf("\t(%d, %d, %d)\n", tres.scanlines[y].rgb[x],tres.scanlines[y].rgb[x+1],tres.scanlines[y].rgb[x+2] );
			}
//			printf(")\n");
			free(tres.scanlines[y].rgb);
		}
		free(tres.scanlines);
		free(cachefile);

		pthread_exit(NULL);
		return 0;
}
