#include <stdio.h>
#include <stdlib.h>
#include <openjpeg.h>
#include <CUnit/Basic.h>
#include "../lib/opj_res.h"




static void test_opj_init_res(void) {
	struct opj_res res = opj_init_res();
	CU_ASSERT_EQUAL(-1, res.status);
	CU_ASSERT_EQUAL(NULL, res.open_file);
	CU_ASSERT_EQUAL(NULL, res.l_stream);
	CU_ASSERT_EQUAL(NULL, res.l_codec);
	CU_ASSERT_EQUAL(NULL, res.image);
}

static void test_opj_init_from_stream(void) {
	int status; 
	struct opj_res res = opj_init_res();
	opj_dparameters_t *p_p = NULL;
	opj_dparameters_t p;
	status = opj_init_from_stream(p_p, &res);
	CU_ASSERT_EQUAL(2, status);


	opj_set_default_decoder_parameters(&p);

	status = opj_init_from_stream(&p, &res);
	CU_ASSERT_EQUAL(3, status);

	FILE *fptr = fopen("balloon.jp2", "rb");
	res.l_stream = opj_stream_create_default_file_stream(fptr,1);
	status = opj_init_from_stream(&p, &res);
	CU_ASSERT_EQUAL(0, status);

	opj_cleanup(&res);
	fclose(fptr);
	free(p_p);
}

static void test_opj_init(void) {
	opj_dparameters_t p;
	opj_set_default_decoder_parameters(&p);
	struct opj_res res = opj_init("balloon.jp2", &p);
	CU_ASSERT_EQUAL(0, res.status);

	res = opj_init("foobar", &p);
	CU_ASSERT_EQUAL(1, res.status);

	res = opj_init("LICENSE", &p);
	CU_ASSERT_EQUAL(3, res.status);
	opj_cleanup(&res);

}

int main(void) {
	CU_pSuite pSuite = NULL;

	if (CUE_SUCCESS != CU_initialize_registry()) {
		return CU_get_error();
	}

	pSuite = CU_add_suite("Suite_1", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	if(	NULL == CU_add_test(pSuite, "test opj_init_res", test_opj_init_res) ||
		NULL == CU_add_test(pSuite, "test opj_init_from_stream", test_opj_init_from_stream) ||
		NULL == CU_add_test(pSuite, "test opj_init", test_opj_init)) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	CU_cleanup_registry();
	return 0;
}
