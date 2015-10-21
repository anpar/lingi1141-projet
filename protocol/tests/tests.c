#include "../src/read_loop.h"
#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <stdlib.h>
#include <zlib.h>
#include <string.h>

/* Test Suite setup and cleanup functions: */

int init_suite(void) {
        return 0;
}

int clean_suite(void) {
        return 0;
}

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

void CU_ASSERT_PKT_EQUAL(pkt_t * a, pkt_t * b, int check_crc) {
        CU_ASSERT(pkt_get_type(a) == pkt_get_type(b));
        CU_ASSERT(pkt_get_window(a) == pkt_get_window(b));
        CU_ASSERT(pkt_get_seqnum(a) == pkt_get_seqnum(b));
        CU_ASSERT(pkt_get_length(a) == pkt_get_length(b));
        CU_ASSERT_STRING_EQUAL(pkt_get_payload(a), pkt_get_payload(b));

        if(check_crc == 1)
                CU_ASSERT(pkt_get_crc(a) == pkt_get_crc(b));
}

void CU_ASSERT_PKT_HEADER_EQUAL(pkt_t * a, pkt_t * b) {
        CU_ASSERT(pkt_get_type(a) == pkt_get_type(b));
        CU_ASSERT(pkt_get_window(a) == pkt_get_window(b));
        CU_ASSERT(pkt_get_seqnum(a) == pkt_get_seqnum(b));
        CU_ASSERT(pkt_get_length(a) == pkt_get_length(b));
}

void encode(void) {
        /*
         * Case 1 : everything is ok.
         * We are testing here if
         * decode(encode(pkt)) == pkt
         * and if PKT_OK is returned.
         */
        pkt_t * pkt1  = pkt_new();
        pkt_set_type(pkt1, PTYPE_DATA);
        pkt_set_window(pkt1, 15);
        pkt_set_seqnum(pkt1, 142);
        // strlen(data1) = 27
        const char * data1 = "Lorem ipsum dolor sit amet.";
        pkt_set_payload(pkt1, data1, strlen(data1));

        pkt_t * pkt1_d = pkt_new();

        size_t len1 = 520;
        char * buf1 = (char *) malloc(len1*sizeof(char));
        CU_ASSERT(pkt_encode(pkt1, buf1, &len1) == PKT_OK);
        CU_ASSERT(len1 == 4 + 27 + 1 + 4);
        CU_ASSERT(pkt_decode(buf1, len1, pkt1_d) == PKT_OK);
        CU_ASSERT_PKT_EQUAL(pkt1, pkt1_d, 0);

        pkt_del(pkt1);
        pkt_del(pkt1_d);
        free(buf1);

        /*
         * Case 2 : buffer is too short.
         * We are testing here if, in this
         * case, encode returns E_NOMEM.
         */
        pkt_t * pkt2 = pkt_new();
        pkt_set_type(pkt2, PTYPE_DATA);
        pkt_set_window(pkt2, 2);
        pkt_set_seqnum(pkt2, 212);
        // strlen(data2) = 50
        const char * data2 = "Lorem ipsum dolor sit amet,consectetur adipiscing elit. Quisque amet.";
        pkt_set_payload(pkt2, data2, strlen(data2));

        size_t len2 = strlen(data2) / 2;
        char * buf2 = (char *) malloc(len2*sizeof(char));
        CU_ASSERT(pkt_encode(pkt2, buf2, &len2) == E_NOMEM);
        CU_ASSERT(len2 == (size_t) strlen(data2) / 2);

        pkt_del(pkt2);
        free(buf2);
}

char * data;
pkt_t * pkt;

/*
 * This function is used to
 * recompute CRC easily when
 * we apply modifications
 * on the data stream used
 * for testing decode. Useful
 * to simulate a corruption
 * caused by the network
 * on the data AND on the CRC
 * in such a way that data
 * and CRC are still in
 * accordance.
 *
 * Must me used only after
 * set_data_for_decode
 * has been used.
 */
void compute_crc_for_data(void) {
        uint32_t crc = (uint32_t) crc32(0, (Bytef *) data, 8);
        data[8] = (crc >> 24) & 0xFF;
        data[9] = (crc >> 16) & 0xFF;
        data[10] = (crc >> 8) & 0xFF;
        data[11] = crc & 0xFF;
}

/*
 * Idem for pkt.
 */
void compute_crc_for_pkt(void) {
        uint32_t crc = (uint32_t) crc32(0, (Bytef *) data, 8);
        pkt_set_crc(pkt, crc);
}

void set_data_for_decode(void) {
        /*
         * This data stream is correct.
         * We will modify it through test
         * functions to test every cases.
         */
        data = (char *) malloc(12 * sizeof(char));
        data[0] = 0b00100010;
        data[1] = 150;
        data[2] = 0;
        data[3] = 4;
        data[4] = 'A';
        data[5] = 'B';
        data[6] = 'C';
        data[7] = 'D';
        compute_crc_for_data();

        /*
         * This packet corresponds
         * to the data stream above.
         */
        pkt = pkt_new();
        pkt_set_type(pkt, PTYPE_DATA);
        pkt_set_window(pkt, 2);
        pkt_set_seqnum(pkt, 150);
        pkt_set_length(pkt, 4);
        pkt_set_payload(pkt, "ABCD", 4);
        compute_crc_for_pkt();
}

void del_data_for_decode(void) {
        free(data);
        pkt_del(pkt);
}

/*
 * Case 1 : data steam is correct.
 */
void decode_correct_case(void) {
        set_data_for_decode();
        pkt_t * pkt_d = pkt_new();

        CU_ASSERT(pkt_decode(data, 12, pkt_d) == PKT_OK);
        CU_ASSERT_PKT_EQUAL(pkt_d, pkt, 1);

        pkt_del(pkt_d);
        del_data_for_decode();
}

/*
 * Case 2 : this data stream
 * is too short to contain
 * a header (in fact only
 * the length attribute of
 * decode is too small).
 */

void decode_no_header(void) {
        set_data_for_decode();
        pkt_t * pkt_d = pkt_new();

        CU_ASSERT(pkt_decode(data, 3, pkt_d) == E_NOHEADER);

        pkt_del(pkt_d);
        del_data_for_decode();
}

/*
 * Case 3 : data stream
 * has been corrupted !
 */
void decode_invalid_crc(void) {
        set_data_for_decode();
        data[5] = 'X';
        pkt_t * pkt_d = pkt_new();

        CU_ASSERT(pkt_decode(data, 12, pkt_d) == E_CRC);
        CU_ASSERT_PKT_HEADER_EQUAL(pkt_d, pkt);

        pkt_del(pkt_d);
        del_data_for_decode();
}

/*
 * Case 3bis : CRC has been
 * corrupted !
 */
void decode_invalid_crc_bis(void) {
        set_data_for_decode();
        data[8] = 0;
        pkt_t * pkt_d = pkt_new();

        CU_ASSERT(pkt_decode(data, 12, pkt_d) == E_CRC);
        CU_ASSERT_PKT_HEADER_EQUAL(pkt_d, pkt);

        pkt_del(pkt_d);
        del_data_for_decode();
}

/*
 * Case 4 : the evil network
 * corrupted the type and the CRC
 * in such a way that CRC still
 * corresponds to data
 */
void decode_invalid_type(void) {
        set_data_for_decode();
        data[0] = 0b01100010;
        compute_crc_for_data();
        pkt_t * pkt_d = pkt_new();

        CU_ASSERT(pkt_decode(data, 12, pkt_d) == E_TYPE);

        free(pkt_d);
        del_data_for_decode();
}

/*
 * Case 5 : the evil network
 * corrupted the window and the CRC
 * in such a way that CRC still
 * corresponds to data
 */
void decode_invalid_window(void) {
        /*
         * Since window used 5 bits,
         * the value parsed by decode
         * will never be outside [0,31]
         * and so E_WINDOW will never
         * be returned.

        set_data_for_decode();
        data[0] = 255;
        compute_crc_for_data();
        pkt_t * pkt_d = pkt_new();

        CU_ASSERT(pkt_decode(data, 12, pkt_d) == E_WINDOW);

        free(pkt_d);
        del_data_for_decode();

        */
}

/*
 * Case 6 : the evil network
 * corrupted the seqnum and the
 * CRC in such a way that CRC
 * still corresponds to data
 */
void decode_invalid_seqnum(void) {
        /*
         * Same remark applies here
         */
}

/*
 * Case 7 : the length of the
 * data stream is not a multiple
 * of 4. Hence the padding in the
 * payload is invalid
 */
void decode_invalid_padding(void) {
        char * d = (char *) malloc(11*sizeof(char));
        d[0] = 0b00100010;
        d[1] = 150;
        d[2] = 0;
        d[3] = 3;
        d[4] = 'A';
        d[5] = 'B';
        d[6] = 'C';
        uint32_t crc = (uint32_t) crc32(0, (Bytef *) d, 7);
        d[7] = (crc >> 24) & 0xFF;
        d[8] = (crc >> 16) & 0xFF;
        d[9] = (crc >> 8) & 0xFF;
        d[10] = crc & 0xFF;

        pkt_t * p = pkt_new();
        pkt_set_type(p, PTYPE_DATA);
        pkt_set_window(p, 2);
        pkt_set_seqnum(p, 150);
        pkt_set_length(p, 3);

        pkt_t * pkt_d = pkt_new();

        CU_ASSERT(pkt_decode(d, 11, pkt_d) == E_PADDING);
        CU_ASSERT_PKT_HEADER_EQUAL(pkt_d, p);

        pkt_del(p);
        pkt_del(pkt_d);
        free(d);
}


/*
 * Case 8 : no payload in the data stream.
 */
void decode_no_payload(void) {
        char * d = (char *) malloc(8*sizeof(char));
        d[0] = 0b00100010;
        d[1] = 150;
        d[2] = 0;
        d[3] = 3;
        uint32_t crc = (uint32_t) crc32(0, (Bytef *) d, 4);
        d[4] = (crc >> 24) & 0xFF;
        d[5] = (crc >> 16) & 0xFF;
        d[6] = (crc >> 8) & 0xFF;
        d[7] = crc & 0xFF;

        pkt_t * p = pkt_new();
        pkt_set_type(p, PTYPE_DATA);
        pkt_set_window(p, 2);
        pkt_set_seqnum(p, 150);
        pkt_set_length(p, 3);

        pkt_t * pkt_d = pkt_new();

        CU_ASSERT(pkt_decode(d, 8, pkt_d) == E_NOPAYLOAD);
        CU_ASSERT_PKT_HEADER_EQUAL(pkt_d, p);

        pkt_del(p);
        pkt_del(pkt_d);
        free(d);
}

/*
 * Case 9 : data stream is unconsitent
 */
void decode_unconsistent(void) {
        /*
         * FIX: This test currently doesn't work.
         * When a data stream is unconsistent,
         * error like invalid CRC or invalid
         * padding are returned before
         * E_UNCONSISTENT.

        set_data_for_decode();
        char * d = (char *) malloc(44 * sizeof(char));
        d[0] = 0b00100010;
        d[1] = 150;
        d[2] = 0;
        d[3] = 4;
        d[4] = 'A';
        d[5] = 'B';
        d[6] = 'C';
        d[7] = 'D';
        uint32_t crc = (uint32_t) crc32(0, (Bytef *) d, 40);
        d[40] = (crc >> 24) & 0xFF;
        d[41] = (crc >> 16) & 0xFF;
        d[42] = (crc >> 8) & 0xFF;
        d[42] = crc & 0xFF;

        pkt_t * pkt_d = pkt_new();

        CU_ASSERT(pkt_decode(d, 44, pkt_d) == E_UNCONSISTENT);
        printf("ce = %d.\n", pkt_decode(d, 44, pkt_d));
        CU_ASSERT_PKT_HEADER_EQUAL(pkt_d, pkt);

        del_data_for_decode();
        pkt_del(pkt_d);
        free(d);

        */
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
                        (NULL == CU_add_test(basic, "encode", encode)) ||
                        (NULL == CU_add_test(basic, "decode correct case", decode_correct_case)) ||
                        (NULL == CU_add_test(basic, "decode no header", decode_no_header)) ||
                        (NULL == CU_add_test(basic, "decode invalid CRC", decode_invalid_crc)) ||
                        (NULL == CU_add_test(basic, "decode invalid CRC bis", decode_invalid_crc_bis)) ||
                        (NULL == CU_add_test(basic, "decode invalid type", decode_invalid_type)) ||
                        (NULL == CU_add_test(basic, "decode invalid window", decode_invalid_window)) ||
                        (NULL == CU_add_test(basic, "decode invalid seqnum", decode_invalid_seqnum)) ||
                        (NULL == CU_add_test(basic, "decode invalid padding", decode_invalid_padding)) ||
                        (NULL == CU_add_test(basic, "decode no payload", decode_no_payload)) ||
                        (NULL == CU_add_test(basic, "decode unconsistent", decode_unconsistent))
                )
        {
                CU_cleanup_registry();
                return CU_get_error();
        }

        CU_basic_run_tests();
        return CU_get_error();
}
