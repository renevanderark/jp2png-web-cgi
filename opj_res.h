#define READ_FAILURE 0
#define READ_SUCCESS 1
#define JP2_RFC3745_MAGIC "\x00\x00\x00\x0c\x6a\x50\x20\x20\x0d\x0a\x87\x0a"
#define JP2_MAGIC "\x0d\x0a\x87\x0a"

struct opj_res {
	int status;
	opj_stream_t *l_stream;
	opj_codec_t *l_codec;
	opj_image_t *image;
	FILE *open_file;
};
void opj_cleanup(struct opj_res *resources);
int is_jp2(FILE *fptr);
void error_callback(const char *msg, void *client_data);
void warning_callback(const char *msg, void *client_data);
void info_callback(const char *msg, void *client_data);
struct opj_res opj_init(const char *fname, opj_dparameters_t *parameters);


