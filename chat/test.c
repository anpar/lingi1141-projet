#include "real_address.h"
#include "create_socket.h"
#include "read_write_loop.h"
#include "wait_for_client.h"

#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <stdlib.h>
#include <arpa/inet.h>

/* Test Suite setup and cleanup functions: */

int init_suite(void) {return 0;}

int clean_suite(void) {return 0;}

/* Test case functions */

void test_real_address(void) {
        struct sockaddr_in6 * rval; 
        const char * address, * t;
        char * dst;
        
        /* Case #1 : rval is NULL */
        rval = NULL;
        address = "example.net";

        CU_ASSERT_STRING_EQUAL(real_address(address, rval), "rval can't be NULL.\n");

        /*  Case #2*/
        rval = (struct sockaddr_in6 *) malloc(sizeof(struct sockaddr_in6));

        CU_ASSERT(real_address(address, rval) == NULL);
        
        dst = (char *) malloc(40 * sizeof(char));
        t = inet_ntop(AF_INET6, (void *) &(rval->sin6_addr.s6_addr), dst, 40 * sizeof(char));
        
        CU_ASSERT(t != NULL);
        CU_ASSERT_STRING_EQUAL(dst, "2606:2800:220:1:248:1893:25c8:1946");
        
        /*
         * FIX : quid des autres informations de rval?
         *       
         * rval->sin6_family : 10.
         * rval->sin6_port : 0.
         * rval->sin6_flowinfo : 0.
         * rval->sin6_addr : 2606:2800:220:1:248:1893:25c8:1946.
         * rval->sin6_scope_id : 0.
         *
         * Est-ce normal que tout soit à 0?
         */

        /*
         * Case #3
         */
        address = "localhost";
        
        CU_ASSERT(real_address(address, rval) == NULL);
        
        dst = (char *) malloc(40 * sizeof(char));
        t = inet_ntop(AF_INET6, (void *) &(rval->sin6_addr.s6_addr), dst, 40 * sizeof(char));
        
        CU_ASSERT(t != NULL);
        CU_ASSERT_STRING_EQUAL(dst, "::1");

        free(dst);
        free(rval);
}

void test_create_socket(void) {
        
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
                        (NULL == CU_add_test(basic, "test_real_address", test_real_address)) ||
                        (NULL == CU_add_test(basic, "test_create_socket", test_create_socket))
                )
        {
                CU_cleanup_registry();
                return CU_get_error();
        }

        CU_basic_run_tests();
        return CU_get_error();
}
