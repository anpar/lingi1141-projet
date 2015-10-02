#include "../src/packet_interface.h"
#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>

/* Test Suite setup and cleanup functions: */

int init_suite(void) {return 0;}
int clean_suite(void) {return 0;}

/* Test case functions */
void getters(void) {
        CU_ASSERT(42 == 42);
} 

void setters(void) {
        CU_ASSERT(42 == 42);
}

/* Test Runner Code goes here */
int main(void) {
        CU_pSuite pSuite = NULL;

        /* initialize the CUnit test registry */
        if(CUE_SUCCESS != CU_initialize_registry()) {
                return(CU_get_error());
        }

        /* add a suite to the registry */
        pSuite = CU_add_suite( "complete_test_suite", init_suite, clean_suite );
        if (pSuite == NULL) {
                CU_cleanup_registry();
                return(CU_get_error());
        }

        /* add the tests to the suite */
        if      (
                        (NULL == CU_add_test(pSuite, "getters", getters)) ||
                        (NULL == CU_add_test(pSuite, "setters", setters))
                ) 
        {
                CU_cleanup_registry();
                return CU_get_error();
        }

        /* Run all tests using the CUnit Basic interface */
        CU_basic_run_tests();
        return CU_get_error();
}
