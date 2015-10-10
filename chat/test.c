#include "real_address.h"
#include "create_socket.h"
#include "read_write_loop.h"
#include "wait_for_client.h"

#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <stdlib.h>

/* Test Suite setup and cleanup functions: */

int init_suite(void) {return 0;}

int clean_suite(void) {return 0;}

/* Test case functions */

void test_real_address(void) {
        struct sockaddr_in6 * rval = (struct sockaddr_in6 *) malloc(sizeof(struct sockaddr_in6));
        const char * address_1 = "google.com";
        const char * s = real_address(address_1, rval);
        
        CU_ASSERT(s == NULL);
        printf("rval->sin6_family : %d.\n", rval->sin6_family);
        printf("rval->sin6_port : %d.\n", rval->sin6_port);
        printf("rval->sin6_flowinfo : %d.\n", rval->sin6_flowinfo);
        printf("rval->sin6_addr : %s.\n", rval->sin6_addr.s6_addr);
        printf("rval->sin6_scope_id : %d.\n", rval->sin6_scope_id);
        
        free(rval);
}

/* Test Runner Code goes here */
int main(void) {
        CU_pSuite basic = NULL;
        
        /* initialize the CUnit test registry */
        if(CUE_SUCCESS != CU_initialize_registry()) {
                return(CU_get_error());
        }

        /* add a suite to the registry */
        basic = CU_add_suite("Complete test suite", init_suite, clean_suite);
        if (basic == NULL) {
                CU_cleanup_registry();
                return(CU_get_error());
        }

        /* add the tests to the suite */
        if      (
                        (NULL == CU_add_test(basic, "test_real_address", test_real_address))
                )
        {
                CU_cleanup_registry();
                return CU_get_error();
        }

        CU_basic_run_tests();
        return CU_get_error();
}
