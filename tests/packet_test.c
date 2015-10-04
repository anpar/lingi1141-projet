#include "../src/packet_interface.h"
#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <stdlib.h>

/* Test Suite setup and cleanup functions: */

int init_suite(void) {return 0;}
int clean_suite(void) {return 0;}

/* Test case functions */

/*
 * FIRST PART : Getters and setters
 */
void test_pkt_type(void) {
        pkt_t * pkt = pkt_new();
       
        /* 
         * On teste si l'assignation d'un type invalide
         * retourne bien E_TYPE et que la valeur du type
         * après cette assignation invalide est bien
         * différente de celle qu'on a voulu assigné.
         */
        CU_ASSERT(pkt_set_type(pkt ,3) == E_TYPE);
        CU_ASSERT(pkt_get_type(pkt) != 3);

        /* 
         * On teste si l'assignation d'un type valide
         * retourne bien PKT_OK et fonctionne correctement.
         * On vérifie également que pkt_get_type ne modifie
         * pas la valeur du type.
         */
        CU_ASSERT(pkt_set_type(pkt, PTYPE_DATA) == PKT_OK);
        CU_ASSERT(pkt_get_type(pkt) == PTYPE_DATA);
        CU_ASSERT(pkt_set_type(pkt, PTYPE_ACK) == PKT_OK);
        CU_ASSERT(pkt_get_type(pkt) == PTYPE_ACK);
        CU_ASSERT(pkt_set_type(pkt, PTYPE_NACK) == PKT_OK);
        CU_ASSERT(pkt_get_type(pkt) == PTYPE_NACK);
        CU_ASSERT(pkt_get_type(pkt) == PTYPE_NACK);

        pkt_del(pkt);
}

void test_pkt_window(void) {
        pkt_t * pkt = pkt_new();

        /*
         * The first assert returns E_WINDOW only thanks
         * to the definition of MAX_WINDOW_SIZE. Giving -1
         * as an argument to pkt_set_window will produce
         * a implicit cast of this argument by adding
         * UINT8_MAX + 1 (> 31 in this case) and so E_WINDOW
         * will be returned by the function.
         */
        CU_ASSERT(pkt_set_window(pkt, -1) == E_WINDOW);
        CU_ASSERT(pkt_set_window(pkt, MAX_WINDOW_SIZE+1) == E_WINDOW);
        CU_ASSERT(pkt_get_window(pkt) != MAX_WINDOW_SIZE+1);
        
        int i;
        for(i = 0; i <= MAX_WINDOW_SIZE; i++) {
                CU_ASSERT(pkt_set_window(pkt, i) == PKT_OK);
                CU_ASSERT(pkt_get_window(pkt) == i);
        }

        CU_ASSERT(pkt_get_window(pkt) == i-1);
        pkt_del(pkt);
}

void test_pkt_seqnum(void) {
        /*
         * As in the previous case, giving a value
         * outside [0,255] as an argument pkt_set_seqnum
         * will produce an implicit cast... So any value
         * given to pkt_set_value will finally stand
         * in the correct range (E_SEQNUM is never
         * returned).
         *
         * As a consequence, a additionnel check
         * must be performed BEFORE using pkt_set_seqnum.
         */

        pkt_t * pkt = pkt_new();

        int i;
        for(i = 0; i < 256; i++) {
                CU_ASSERT(pkt_set_seqnum(pkt, i) == PKT_OK);
                CU_ASSERT(pkt_get_seqnum(pkt) == i);
        }

        CU_ASSERT(pkt_get_seqnum(pkt) == i-1);
        pkt_del(pkt);
}
 
void test_pkt_length(void) {
        pkt_t * pkt = pkt_new();

        /*
         * Same remark for the third assert...
         */
        CU_ASSERT(pkt_set_length(pkt, MAX_PAYLOAD_SIZE+1) == E_LENGTH);
        CU_ASSERT(pkt_get_length(pkt) != MAX_PAYLOAD_SIZE+1);
        CU_ASSERT(pkt_set_length(pkt, -1) == E_LENGTH);

        int i;
        for(i = 0; i <= MAX_PAYLOAD_SIZE; i++) {
                CU_ASSERT(pkt_set_length(pkt, i) == PKT_OK);
                CU_ASSERT(pkt_get_length(pkt) == i);
        } 

        CU_ASSERT(pkt_get_length(pkt) == i-1);

        pkt_del(pkt);
}

void test_pkt_crc(void) {
        // Nothing to test here...
}

void test_pkt_payload(void) {
        pkt_t * pkt = pkt_new();

        /*
         * First case : data is empty.
         */
        CU_ASSERT(pkt_set_payload(pkt, NULL, 0) == PKT_OK);
        CU_ASSERT(pkt_get_length(pkt) == 0);        
        /*
         * Second case : data doesn't need a padding
         */
        CU_ASSERT(pkt_set_payload(pkt, "ABCD", 4) == PKT_OK);
        CU_ASSERT(pkt_get_length(pkt) == 4);
        CU_ASSERT_STRING_EQUAL(pkt_get_payload(pkt), "ABCD");
        CU_ASSERT_STRING_EQUAL(pkt_get_payload(pkt), "ABCD");
        
        /*
         * Third case : data needs a padding
         */
        CU_ASSERT(pkt_set_payload(pkt, "123", 3) == PKT_OK);
        CU_ASSERT(pkt_get_length(pkt) == 3);
        // FIX : adding a 0 after 123 causes a seg fault.. wtf?
        CU_ASSERT_STRING_EQUAL(pkt_get_payload(pkt), "1230");

        /*
         * Fourth case : data is too long
         */
        const char * data = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Fusce sed nibh porttitor, aliquet elit id, cursus nulla. Curabitur in risus molestie, ornare nulla nec, condimentum velit. Aenean placerat ante iaculis, ultrices metus et, tristique enim. Curabitur vitae lorem enim. Donec id mauris quis augue convallis dictum. Nam lacinia in ligula ac rhoncus. Nam fringilla justo ut dui fermentum vehicula. In scelerisque neque sed posuere pulvinar. Ut tincidunt ac nunc eget consectetur. Nam tempor tellus at placerat mollis. Etiam eget ultricies eros. In elementum mauris eu scelerisque efficitur volutpat."; 
        CU_ASSERT(pkt_set_payload(pkt, data, strlen(data)) == E_LENGTH);
        CU_ASSERT(pkt_get_length(pkt) != strlen(data));
        CU_ASSERT_STRING_NOT_EQUAL(pkt_get_payload(pkt), data);

        pkt_del(pkt);
}

/*
 * SECOND PART : Encode and decode
 */

void CU_ASSERT_PKT_EQUAL(pkt_t * a, pkt_t * b) {
        CU_ASSERT(pkt_get_type(a) == pkt_get_type(b));
        CU_ASSERT(pkt_get_window(a) == pkt_get_window(b));
        CU_ASSERT(pkt_get_seqnum(a) == pkt_get_seqnum(b));
        CU_ASSERT(pkt_get_length(a) == pkt_get_length(b));
        CU_ASSERT_STRING_EQUAL(pkt_get_payload(a), pkt_get_payload(b));
        /*
         * This line is not usefull since the packet before
         * encoding doesn't contain the crc.
         * CU_ASSERT(pkt_get_crc(a) == pkt_get_crc(b));
         */
}

void encode(void) {
        /*
         * Case 1 : everything is ok
         */
        pkt_t * pkt1  = pkt_new();
        pkt_set_type(pkt1, PTYPE_DATA);
        pkt_set_window(pkt1, 15);
        pkt_set_seqnum(pkt1, 142);
        const char * data1 = "Lorem ipsum dolor sit amet.";
        pkt_set_payload(pkt1, data1, strlen(data1));

        pkt_t * pkt1_d = pkt_new();

        size_t len1 = 520;
        char * buf1 = (char *) malloc(len1*sizeof(char));
        CU_ASSERT(pkt_encode(pkt1, buf1, &len1) == PKT_OK);
        CU_ASSERT(len1 == 36);
        CU_ASSERT(pkt_decode(buf1, len1, pkt1_d) == PKT_OK);
        CU_ASSERT_PKT_EQUAL(pkt1, pkt1_d);
        
        pkt_del(pkt1);
        pkt_del(pkt1_d);
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
                        (NULL == CU_add_test(basic, "type", test_pkt_type)) ||
                        (NULL == CU_add_test(basic, "window", test_pkt_window)) ||
                        (NULL == CU_add_test(basic, "seqnum", test_pkt_seqnum)) ||
                        (NULL == CU_add_test(basic, "length", test_pkt_length)) ||
                        (NULL == CU_add_test(basic, "crc", test_pkt_crc)) ||
                        (NULL == CU_add_test(basic, "payload", test_pkt_payload)) ||
                        (NULL == CU_add_test(basic, "encode", encode))
                ) 
        {
                CU_cleanup_registry();
                return CU_get_error();
        }

        CU_basic_run_tests();
        return CU_get_error();
}
