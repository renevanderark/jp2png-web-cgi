#include <stdlib.h>
#include <string.h>
#include <curl/curl.h> 
#include <curl/easy.h>

struct chunk {
	char *filename;
	unsigned idx;
};

static size_t write_to_cache(void *ptr, size_t size, size_t nmb, struct chunk *chunk) {
	char filename[255];
	sprintf(filename, "%s-%05u-%zu-%zu", chunk->filename, chunk->idx++, nmb, size);
	fprintf(stderr, "%s\n", filename);
	FILE *f = fopen(filename, "w");
	fwrite(ptr, size, size*nmb, f);
	fclose(f);
	return size*nmb;
}

int main(int argc, char **argv) {
	struct chunk *chunk = malloc(sizeof(chunk));
	CURL *ch;
	chunk->idx = 0;
	chunk->filename = "output/testfile";
	ch = curl_easy_init();
	curl_easy_setopt(ch, CURLOPT_URL, argv[1]);
	curl_easy_setopt(ch, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(ch, CURLOPT_WRITEFUNCTION, (void*)write_to_cache);
	curl_easy_setopt(ch, CURLOPT_WRITEDATA, (void*)chunk);
	curl_easy_perform(ch);
	curl_easy_cleanup(ch);
	curl_global_cleanup();
	free(chunk);
	return 0;
}
