/**
    opj_url_stream.c handlers for random access url decoding using libcurl
    Copyright (C) 2013  René van der Ark

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <stdlib.h>
#include <string.h>
#include <curl/curl.h> 
#include <curl/easy.h>
#include <openjpeg.h>
#include "opj_url_stream.h"

OPJ_OFF_T opj_skip_from_url (OPJ_OFF_T p_nb_bytes, struct opj_url_stream_data * p_url) {
	p_url->position += p_nb_bytes;
	if(p_url->position > p_url->size) {
		return -1;
	}
	return p_nb_bytes;
}

OPJ_BOOL opj_seek_from_url (OPJ_OFF_T p_nb_bytes, struct opj_url_stream_data * p_url) {
	p_url->position = p_nb_bytes;
	if(p_url->position > p_url->size) {
		return OPJ_FALSE;
	}
	return OPJ_TRUE;
}

OPJ_SIZE_T opj_read_from_url (void * p_buffer, OPJ_SIZE_T p_nb_bytes, struct opj_url_stream_data * p_url) {
	char header_str[80];
	char *buf;
	size_t size;
	FILE *fp = open_memstream(&buf, &size);
	CURL *ch;
	struct curl_slist *headers = NULL;
	sprintf(header_str, "Range: bytes=%zu-%zu", p_url->position,  p_url->position + p_nb_bytes);

	headers = curl_slist_append(headers, header_str);
	ch = curl_easy_init();
	curl_easy_setopt(ch, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(ch, CURLOPT_URL, p_url->url);
	curl_easy_setopt(ch, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(ch, CURLOPT_WRITEDATA, (void*)fp);
	curl_easy_perform(ch);
	curl_easy_cleanup(ch);
	curl_global_cleanup();

	OPJ_SIZE_T l_nb_read = fread(p_buffer,1,p_nb_bytes, fp);
	p_url->position += l_nb_read;

	fclose(fp);
	free(buf);
	curl_slist_free_all(headers);
	return l_nb_read ? l_nb_read : (OPJ_SIZE_T)-1;
}

OPJ_UINT64 get_url_data_length(char * address) {
	char *buf;
	size_t size;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	OPJ_UINT64 data_length = 0;

	FILE *fp = open_memstream(&buf, &size);
	CURL *ch;
	ch = curl_easy_init();
	curl_easy_setopt(ch, CURLOPT_URL, address);
	curl_easy_setopt(ch, CURLOPT_NOBODY, 1);
	curl_easy_setopt(ch, CURLOPT_WRITEHEADER, (void*) fp);
	curl_easy_setopt(ch, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_perform(ch);
	curl_easy_cleanup(ch);
	curl_global_cleanup();

	while ((read = getline(&line, &len, fp)) != -1) {
		char *hk = strtok(line, ":");
		if(strcmp(hk, "Content-Length") == 0) {
			char *v = strtok(NULL, ":");
			data_length = atoi(v);
		}
	}
	fclose(fp);
	free(buf);
	free(line);
	return data_length;
}

