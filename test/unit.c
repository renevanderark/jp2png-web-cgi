#include <stdio.h>
#include <stdlib.h>
#include <openjpeg.h>
#include <CUnit/Basic.h>
#include "../lib/opj_res.h"

struct opj_res res;


static void test_opj_init_res(void) {
	res = opj_init_res();
	CU_ASSERT_EQUAL(-1, res.status);
	CU_ASSERT_EQUAL(NULL, res.open_file);
	CU_ASSERT_EQUAL(NULL, res.l_stream);
	CU_ASSERT_EQUAL(NULL, res.l_codec);
	CU_ASSERT_EQUAL(NULL, res.image);
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

	if((NULL == CU_add_test(pSuite, "test opj_init_res", test_opj_init_res)) ||
		(NULL == CU_add_test(pSuite, "test opj_init_res", test_opj_init_res))) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	CU_cleanup_registry();
	return 0;
}
