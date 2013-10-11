#include <stdio.h>
#include <dirent.h>
#include <getopt.h>
#include <time.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

struct file_node {
	time_t lifetime;
	size_t size;
	char fullpath[255];
	struct file_node *next;
};

static int verbose = 0;

static void print_file_node(struct file_node *f) {
	if(verbose) fprintf(stderr, "[INFO] %s:\tlifetime=%zus\tsize=%zu(MB)\n\n", f->fullpath, f->lifetime, f->size / 1024 / 1024);
}

static void free_list(struct file_node *list) {
	if(list == NULL) { return; }
	if(list->next != NULL) { free_list(list->next); }

	free(list);
}

static struct file_node *insert(char *fullpath, time_t lifetime, size_t filesize, struct file_node *list) {
	struct file_node *new_node = malloc(sizeof(struct file_node));

	memcpy((char*)&new_node->fullpath, fullpath, 255);
	new_node->lifetime = lifetime;
	new_node->size = filesize;
	new_node->next = NULL;

	if(list == NULL) { return new_node; }
	else if(list->lifetime < new_node->lifetime) {
		new_node->next = list;
		return new_node;
	}

	struct file_node *cur_node = list;

	while(cur_node->next != NULL) {
		if(cur_node->next->lifetime < new_node->lifetime) {
			new_node->next = cur_node->next;
			cur_node->next = new_node;
			return list;
		}
		cur_node = cur_node->next;
	}

	cur_node->next = new_node;
	return list;
}

static int apply_cache_rules(char *cachedir, time_t lifetime, size_t max_size) {
	DIR *dirp;
	struct dirent *dp;
	time_t now;
	size_t dir_usage = 0;
	size_t dir_usage_mb = 0;

	struct file_node *list = NULL;

	if((dirp = opendir(cachedir)) == NULL) {
		if(verbose) fprintf(stderr, "[INFO] Could not open cache directory: %s\n", cachedir);
		return 1;
	}

	time (&now);
	while((dp = readdir(dirp)) != NULL) {
		struct stat statbuf;
		char fullpath[255];

		sprintf(fullpath, "%s/%s", cachedir, dp->d_name);

		if(strcmp(".", dp->d_name) == 0 || strcmp("..", dp->d_name) == 0) { continue; }

		if(stat(fullpath, &statbuf) == 0) {
			time_t file_lifetime = now - statbuf.st_atime;
			if(lifetime < file_lifetime) {
				if(verbose) fprintf(stderr, "[INFO] Removing expired file: %s\n", fullpath);
				if(unlink(fullpath) < 0) { 
					fprintf(stderr, "[ERROR] failed to remove file: %s\n", fullpath); 
					free_list(list);
					return 1;
				}
				continue;
			}

			dir_usage += statbuf.st_size;
			list = insert(fullpath, file_lifetime, statbuf.st_size, list);
		}
	}

	if(dir_usage / 1024 / 1024 > max_size) {
		if(verbose) fprintf(stderr, "[INFO] Directory in excess of max usage: %zuMB / %zuMB\n", dir_usage / 1024 / 1024, max_size);

		struct file_node *cur_node = list;
		while(cur_node != NULL) {
			if(verbose) fprintf(stderr, "[INFO] Removing oldest remaining file: %s\n", cur_node->fullpath);

			if(unlink(cur_node->fullpath) < 0) { 
				fprintf(stderr, "[ERROR] failed to remove file: %s\n", cur_node->fullpath); 
				free_list(list);
				return 1;
			}
			dir_usage -= cur_node->size;
			if(dir_usage / 1024 / 1024 < max_size) { break; }
			cur_node = cur_node->next;
		}
	}

	free_list(list);
	closedir(dirp);
	return 0;
}

int main(int argc, char *argv[]) {
	static struct option opts[] = {
		{"verbose", no_argument, NULL, 'v'},
		{"cache-dir", required_argument, NULL, 'd'},
		{"lifetime", required_argument, NULL, 't'},
		{"max-size", required_argument, NULL, 'm'},
		{NULL, 0, NULL, 0}
	};
	char ch;
	char *cachedir = NULL;
	time_t lifetime = 3600;
	size_t max_size = 1024; 
	while((ch = getopt_long(argc, argv, "d:t:m:v:", opts, NULL)) != -1) {
		switch(ch) {
			case 'v':
				verbose = 1;
				break;
			case 'd':
				cachedir = optarg;
				if(cachedir[strlen(cachedir) - 1] == '/') { cachedir[strlen(cachedir) - 1] = '\0'; }
				break;
			case 't':
				lifetime = (time_t) strtol(optarg, NULL, 0);
				break;
			case 'm':
				max_size = (size_t) strtol(optarg, NULL, 0);
		}
	}
	if(cachedir == NULL) { 
		printf("Usage: %s --cache-dir d [--lifetime t] [--max-size m] [--verbose]\n", argv[0]);
		printf("\t--cache-dir d: the cache directory\n");
		printf("\t--lifetime t: max lifetime of cached file in seconds (measuring from latest access time)\n");
		printf("\t--max-size m: maximum total size of files in cache dir expressed in MB\n");

		fprintf(stderr, "[ERROR] No cache dir specified\n", argv[0]); 
		return 1; 
	}

	
	return apply_cache_rules(cachedir, lifetime, max_size);
}
