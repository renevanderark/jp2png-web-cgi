/**
    jp2png-cgi
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h> 
#include <curl/easy.h>
#include "url2cache.h"

static int check_headers(const char *url) {
	CURL *ch;
	long http_code = 0;

	ch = curl_easy_init();
	curl_easy_setopt(ch, CURLOPT_URL, url);
	curl_easy_setopt(ch, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(ch, CURLOPT_NOBODY, 1);
	curl_easy_perform(ch);

	CURLcode curl_code = curl_easy_perform(ch);
	curl_easy_getinfo(ch, CURLINFO_RESPONSE_CODE, &http_code);
	curl_easy_cleanup(ch);
	curl_global_cleanup();

	if(curl_code != CURLE_OK && curl_code != CURLE_ABORTED_BY_CALLBACK) { return 0; }
	if(http_code != 200 ) { return 0; }
	return 1;
}

static char *url2filename(const char *url) {
	unsigned i;
	char *ret = malloc(strlen(url) * sizeof(char));
	for(i = 0; i < strlen(url); i++) {
		ret[i] = url[i];
		switch(url[i]) {
			case ':': 
			case '/': 
			case '&': 
			case '=': 
			case '?': ret[i] = '_';
		}
	}
	return ret;
}

char *download_to_cache(const char *url, const char *cachedir) {
	FILE *fp = NULL;
	char *filename = url2filename(url);
	char *path;
	int len = snprintf(NULL, 0, "%s/%s\n", cachedir, filename);
	path = malloc(len);
	snprintf(path, len, "%s/%s\n", cachedir, filename);
	fp = fopen(path, "r");
	if(fp != NULL) { fclose(fp); return path; }
	fp = fopen(path, "w");
	free(filename);
	if(!check_headers(url) || fp == NULL) { return NULL; }

	CURL *ch;

	ch = curl_easy_init();
	curl_easy_setopt(ch, CURLOPT_URL, url);
	curl_easy_setopt(ch, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(ch, CURLOPT_WRITEDATA, (void*)fp);
	curl_easy_perform(ch);

	curl_easy_cleanup(ch);
	curl_global_cleanup();
	fclose(fp);

	return path;
}

