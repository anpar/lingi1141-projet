#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <stdlib.h>
#include <zlib.h> /* crc32 */
#include <string.h>
#include <fcntl.h> /* open */
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>

#include "../src/read_write_loop.h"
#include "../src/read_loop.h"
#include "../src/wait_for_sender.h"
#include "../src/real_address.h"
#include "../src/create_socket.h"

/* Test Suite setup and cleanup functions: */

int init_suite(void) {
    return 0;
}

int clean_suite(void) {
    return 0;
}

/* Test case functions */


/*--------------------------------------------------------------------------+
| Tests pour les fonctions de packet_implem (encode, decode, etc)           |
+---------------------------------------------------------------------------*/

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

    CU_ASSERT(pkt_set_window(pkt, 4) == PKT_OK);
    CU_ASSERT(pkt_get_window(pkt) == 4);
    CU_ASSERT(pkt_get_window(pkt) == 4);

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

    CU_ASSERT(pkt_set_seqnum(pkt, 4) == PKT_OK);
    CU_ASSERT(pkt_get_seqnum(pkt) == 4);
    CU_ASSERT(pkt_get_seqnum(pkt) == 4);

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

    CU_ASSERT(pkt_set_length(pkt, 220) == PKT_OK);
    CU_ASSERT(pkt_get_length(pkt) == 220);
    CU_ASSERT(pkt_get_length(pkt) == 220);

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
    if(pkt_get_payload(a) != NULL && pkt_get_payload(b) != NULL) {
        CU_ASSERT_STRING_EQUAL(pkt_get_payload(a), pkt_get_payload(b));
    } else {
        CU_ASSERT_PTR_NULL(pkt_get_payload(a));
        CU_ASSERT_PTR_NULL(pkt_get_payload(b));
    }

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
* TODO: Case 9 : data stream is unconsitent
*/
void decode_unconsistent(void) {

}


/*--------------------------------------------------------------------------+
| Tests pour la fonction read_loop (et sous-fonctions) du receiver          |
+---------------------------------------------------------------------------*/

/*
Vérifie que l'initialisation de la fenêtre de réception
se déroule comme prévu.
*/
void test_init_window(void) {
    win * rwin = init_window();
    CU_ASSERT_EQUAL(rwin->last_in_seq, -1);
    CU_ASSERT_EQUAL(rwin->free_space, WIN_SIZE);
    int i;
    for(i = 0; i < WIN_SIZE; i++) {
        CU_ASSERT_PTR_NULL(rwin->buffer[i]);
    }

    free_window(rwin);
}

/*
* Permet de créer simplement des paquets pour les futurs tests.
*/
pkt_t * create_packet(ptypes_t type, uint8_t window, uint8_t seqnum, uint16_t length, char * payload) {
    uint16_t padding = (4 - (length % 4)) % 4;
    size_t len = 8 + length + padding;
    char * buf = (char *) malloc(len * sizeof(char));
    buf[0] = (type << 5) | window;
    buf[1] = seqnum;
    buf[2] = (length >> 8) & 0xFF;
    buf[3] = length & 0xFF;
    uint16_t i;
    for(i = 0; i < length + padding; i++) {
        buf[i+4] = payload[i];
    }
    uint32_t crc = (uint32_t) crc32(0, (Bytef *) buf, 4 + length + padding);
    buf[i+4] = (crc >> 24) & 0xFF;
    buf[i+5] = (crc >> 16) & 0xFF;
    buf[i+6] = (crc >> 8) & 0xFF;
    buf[i+7] = crc & 0xFF;

    pkt_t * p = pkt_new();
    pkt_decode(buf, len, p);
    free(buf);
    return(p);
}

/*
Vérifie le bon fonctionnement de la fonction add_in_window.
*/
void test_add_in_window(void) {
    win * rwin;
    pkt_t *p1, *p2;

    /*
    En ajoutant un paquet de seqnum 4 dans la window fraîchement
    initialisé, il doit donc se situer en 4ème position.
    */
    rwin = init_window();
    p1 = create_packet(PTYPE_DATA, 5, 4, 4, "abcd");
    add_in_window(p1, rwin);
    CU_ASSERT_EQUAL(rwin->free_space, WIN_SIZE-1);
    CU_ASSERT_PKT_EQUAL(rwin->buffer[4], p1, 1);
    CU_ASSERT_EQUAL(rwin->last_in_seq, -1);

    /*
    En ajoutant ensuite un deuxième paquet de seqnum 1 dans cette
    même window, et en vérifiant que les deux paquets sont toujours
    à la bonne position.
    */
    p2 = create_packet(PTYPE_DATA, 5, 1, 4, "efgh");
    add_in_window(p2, rwin);
    CU_ASSERT_EQUAL(rwin->free_space, WIN_SIZE-2);
    CU_ASSERT_PKT_EQUAL(rwin->buffer[1], p2, 1);
    CU_ASSERT_PKT_EQUAL(rwin->buffer[4], p1, 1);
    CU_ASSERT_EQUAL(rwin->last_in_seq, -1);

    free_window(rwin); pkt_del(p1);

    /*
    Avec une nouvelle fenêtre dont le dernier paquet en séquence
    était le numéro 5, on ajoute un paquet de seqnum 6, celui-ci doit
    donc se trouver en premier dans le buffer.
    */
    rwin = init_window();
    rwin->last_in_seq = 5;
    p1 = create_packet(PTYPE_DATA, 5, 6, 4, "abcd");
    add_in_window(p1, rwin);
    CU_ASSERT_EQUAL(rwin->free_space, WIN_SIZE-1);
    CU_ASSERT_PKT_EQUAL(rwin->buffer[0], p1, 1);
    CU_ASSERT_EQUAL(rwin->last_in_seq, 5);

    free_window(rwin); pkt_del(p1);

    /*
    Avec une nouvelle fenêtre dont le dernier paquet en séquence
    était le numéro 253, on ajoute un paquet de seqnum 4, celui-ci doit
    donc se trouver en 6ème position dans le buffer.
    */
    rwin = init_window();
    rwin->last_in_seq = 253;
    p1 = create_packet(PTYPE_DATA, 5, 4, 4, "abcd");
    add_in_window(p1, rwin);
    CU_ASSERT_EQUAL(rwin->free_space, WIN_SIZE-1);
    CU_ASSERT_PKT_EQUAL(rwin->buffer[6], p1, 1);
    CU_ASSERT_EQUAL(rwin->last_in_seq, 253);

    free_window(rwin); pkt_del(p1); pkt_del(p2);
}

/*
Fonction permettant, à l'aide d'assert, de vérifier
l'égalité entre deux fenêtres.
*/
void CU_ASSERT_WIN_EQUAL(win * win1, win * win2) {
    CU_ASSERT_EQUAL(win1->last_in_seq, win2->last_in_seq);
    CU_ASSERT_EQUAL(win1->free_space, win2->free_space);

    int i;
    for(i = 0; i < WIN_SIZE; i++) {
        if(win1->buffer[i] != NULL && win2->buffer[i] != NULL) {
            CU_ASSERT_PKT_EQUAL(win1->buffer[i], win2->buffer[i], 1);
        } else {
            CU_ASSERT_PTR_NULL(win1->buffer[i]);
            CU_ASSERT_PTR_NULL(win2->buffer[i]);
        }
    }
}

/*
Vérifie le bon fonctionnement de shift_window pour différents
types de fenêtre. Les différents cas sont représentés en commentaire
sous la forme ----****- ou '-' représente une place vide de la fenêtre
et * une place occupée. On indique la fenêtre avant shift_window à gauche
de '-->' et celle attendue après shift_window à droite de '-->'.
*/
void test_shift_window(void) {
    win *rwin = init_window();      // Fenêtre avant shift window
    win *rwin_e = init_window();    // Fenêtre espérée après shift_window
    pkt_t *p1, *p2, *p3, *p4;

    /* On redirige la sortie de shift_window vers /dev/null pour ne pas polluer stdout */
    int fd = open("/dev/null", O_WRONLY);
    if(fd == -1)
    perror("open() in test_shift_window");

    // ---*--- --> ---*---
    p1 = create_packet(PTYPE_DATA, 5, 3, 4, "abcd");
    add_in_window(p1, rwin);
    add_in_window(p1, rwin_e);
    CU_ASSERT_FALSE(shift_window(rwin, fd));
    CU_ASSERT_WIN_EQUAL(rwin, rwin_e);

    free_window(rwin); free_window(rwin_e);
    pkt_del(p1);

    // *-*---- --> -*-----
    rwin = init_window();
    p1 = create_packet(PTYPE_DATA, 5, 0, 4, "abcd");
    add_in_window(p1, rwin);
    p2 = create_packet(PTYPE_DATA, 5, 2, 4, "efgh");
    add_in_window(p2, rwin);

    rwin_e = init_window();
    rwin_e->last_in_seq = 0;
    p3 = create_packet(PTYPE_DATA, 5, 2, 4, "efgh");
    add_in_window(p3, rwin_e);

    CU_ASSERT_FALSE(shift_window(rwin, fd));
    CU_ASSERT_WIN_EQUAL(rwin, rwin_e);

    free_window(rwin); free_window(rwin_e);
    // p1 est déjà free'd par shift_window
    pkt_del(p2); pkt_del(p3);

    // **-*--- --> -*------
    rwin = init_window();
    p1 = create_packet(PTYPE_DATA, 5, 0, 4, "abcd");
    add_in_window(p1, rwin);
    p2 = create_packet(PTYPE_DATA, 5, 1, 4, "efgh");
    add_in_window(p2, rwin);
    p3 = create_packet(PTYPE_DATA, 5, 3, 3, "ijk");
    add_in_window(p3, rwin);

    rwin_e = init_window();
    rwin_e->last_in_seq = 1;
    p4 = create_packet(PTYPE_DATA, 5, 3, 3, "ijk");
    add_in_window(p4, rwin_e);

    CU_ASSERT_FALSE(shift_window(rwin, fd));
    CU_ASSERT_WIN_EQUAL(rwin, rwin_e);

    free_window(rwin); free_window(rwin_e);
    pkt_del(p3); pkt_del(p4);

    // *----*--- --> ----*---- avec last_in_seq = 254
    rwin = init_window();
    rwin->last_in_seq = 254;
    p1 = create_packet(PTYPE_DATA, 5, 255, 4, "abcd");
    add_in_window(p1, rwin);
    p2 = create_packet(PTYPE_DATA, 5, 1, 4, "efgh");
    add_in_window(p2, rwin);

    rwin_e = init_window();
    rwin_e->last_in_seq = 255;
    p3 = create_packet(PTYPE_DATA, 5, 1, 4, "efgh");
    add_in_window(p3, rwin_e);

    CU_ASSERT_FALSE(shift_window(rwin, fd));
    CU_ASSERT_WIN_EQUAL(rwin, rwin_e);

    free_window(rwin); free_window(rwin_e);
    pkt_del(p2); pkt_del(p3);

    /*
    Un paquet de fin de transfert est présent
    dans la fenêtre mais on doit encore attendre
    des paquets manquants
    */
    rwin = init_window();
    p1 = create_packet(PTYPE_DATA, 5, 0, 4, "abcd");
    add_in_window(p1, rwin);
    p2 = create_packet(PTYPE_DATA, 5, 2, 0, NULL);
    add_in_window(p2, rwin);

    rwin_e = init_window();
    rwin_e->last_in_seq = 0;
    p3 = create_packet(PTYPE_DATA, 5, 2, 0, NULL);
    add_in_window(p3, rwin_e);

    CU_ASSERT_FALSE(shift_window(rwin, fd));
    CU_ASSERT_WIN_EQUAL(rwin, rwin_e);

    free_window(rwin); free_window(rwin_e);
    pkt_del(p2); pkt_del(p3);

    /*
    Un paquet de fin de transfert est présent
    dans la fenêtre et on ne doit plus attendre
    de paquets manquants
    */
    rwin = init_window();
    p1 = create_packet(PTYPE_DATA, 5, 0, 4, "abcd");
    add_in_window(p1, rwin);
    p2 = create_packet(PTYPE_DATA, 5, 1, 0, NULL);
    add_in_window(p2, rwin);

    rwin_e = init_window();
    rwin_e->last_in_seq = 1;

    CU_ASSERT_TRUE(shift_window(rwin, fd));
    CU_ASSERT_WIN_EQUAL(rwin, rwin_e);

    free_window(rwin); free_window(rwin_e);
}

/*
Vérifie le bon fonctionnement de la fonction in_window.
*/
void test_in_window(void) {
    win *rwin = init_window();
    CU_ASSERT(in_window(rwin, 12));
    CU_ASSERT(in_window(rwin, WIN_SIZE-1))
    CU_ASSERT(!in_window(rwin, WIN_SIZE));

    rwin->last_in_seq = 20;
    CU_ASSERT(in_window(rwin, 32));
    CU_ASSERT(in_window(rwin, 51));
    CU_ASSERT(!in_window(rwin, 52));
    CU_ASSERT(!in_window(rwin, 20));
    CU_ASSERT(!in_window(rwin, 12));

    rwin->last_in_seq = 253;
    CU_ASSERT(in_window(rwin, 254));
    CU_ASSERT(in_window(rwin, 12));
    CU_ASSERT(in_window(rwin, 28));
    CU_ASSERT(!in_window(rwin, 29));
    CU_ASSERT(!in_window(rwin, 250));

    free_window(rwin);
}

/*
Vérifie le bon fonctionnement de la fonction build_ack.
*/
void test_build_ack(void) {
    win * rwin = init_window();
    pkt_t *p = pkt_new();
    char ack[4];

    /*
    On envoie un ack pour le premier paquet reçu (connexion)
    */
    pkt_t *p1 = create_packet(PTYPE_DATA, 5, 0, 4, "abcd");
    add_in_window(p1, rwin);
    build_ack(ack, rwin);
    CU_ASSERT_EQUAL(pkt_decode(ack, 4, p), PKT_OK);
    CU_ASSERT_EQUAL(pkt_get_type(p), PTYPE_ACK);
    CU_ASSERT_EQUAL(pkt_get_window(p), rwin->free_space);
    CU_ASSERT_EQUAL(pkt_get_seqnum(p), 1);
    CU_ASSERT_EQUAL(pkt_get_length(p), 0);
    pkt_del(p1); free_window(rwin); rwin = init_window();

    /*
    On envoie un ack pour un paquet reçu quelconque
    */
    rwin->last_in_seq = 10;
    rwin->free_space = 25;
    build_ack(ack, rwin);
    CU_ASSERT_EQUAL(pkt_decode(ack, 4, p), PKT_OK);
    CU_ASSERT_EQUAL(pkt_get_type(p), PTYPE_ACK);
    CU_ASSERT_EQUAL(pkt_get_window(p), rwin->free_space);
    CU_ASSERT_EQUAL(pkt_get_seqnum(p), 11);
    CU_ASSERT_EQUAL(pkt_get_length(p), 0);

    /*
    Le dernier numéro de séquence reçu est 255, on vérifie
    que l'ack correspondant a bien comme seqnum 0.
    */
    rwin->last_in_seq = 255;
    build_ack(ack, rwin);
    CU_ASSERT_EQUAL(pkt_decode(ack, 4, p), PKT_OK);
    CU_ASSERT_EQUAL(pkt_get_type(p), PTYPE_ACK);
    CU_ASSERT_EQUAL(pkt_get_window(p), rwin->free_space);
    CU_ASSERT_EQUAL(pkt_get_seqnum(p), 0);
    CU_ASSERT_EQUAL(pkt_get_length(p), 0);

    free_window(rwin);
    pkt_del(p);
}

/*
Vérifie le bon fonctionnement de la fonction build_nack.
*/
void test_build_nack(void) {
    win * rwin = init_window();
    pkt_t *p1, *p2;
    char nack[4];

    p1 = create_packet(PTYPE_DATA, 5, 15, 18, "");
    build_nack(p1, nack, rwin);
    p2 = pkt_new();
    CU_ASSERT_EQUAL(pkt_decode(nack, 4, p2), PKT_OK);
    CU_ASSERT_EQUAL(pkt_get_type(p2), PTYPE_NACK);
    CU_ASSERT_EQUAL(pkt_get_window(p2), rwin->free_space);
    CU_ASSERT_EQUAL(pkt_get_seqnum(p2), 15);
    CU_ASSERT_EQUAL(pkt_get_length(p2), 0);

    free_window(rwin);
    pkt_del(p1); pkt_del(p2);
}

/* Utiliser pour tester read_loop juste en-dessous */
bool receiver_ready = false;

int get_receiver_socket() {
    struct sockaddr_in6 addr;
    const char *err = real_address("::", &addr);
    if (err) {
        fprintf(stderr, "Could not resolve hostname %s: %s\n", "::", err);
        return -1;
    }

    int sfd = create_socket(&addr, 1200, NULL, -1); /* Bound */
    receiver_ready = true;
    if (sfd > 0 && wait_for_sender(sfd) < 0) { /* Connected */
        fprintf(stderr,"Could not connect the socket after the first packet.\n");
        close(sfd);
        return -1;
    }

    return sfd;
}

int get_sender_socket() {
    struct sockaddr_in6 addr;
    const char *err = real_address("::", &addr);
    if (err) {
        fprintf(stderr, "Could not resolve hostname %s: %s\n", "::", err);
        return -1;
    }

    int sfd = create_socket(NULL, -1, &addr, 1200); /* Connected */
    return(sfd);
}

void * thread_receiver(void * filename) {
    int sfd = get_receiver_socket();
    if(sfd == -1) {
        fprintf(stderr, "Failed to get_receiver_socket() failed.\n");
        pthread_exit(NULL);
    }

    char *file = (char *) filename;
    read_loop(sfd, file);
    pthread_exit(NULL);
}

/*
Vérifie le bon fonctionnement de la foncion read_loop (qui
constitue le coeur du receiver). Il s'agit en gros de vérifier
si les ack/nack corrects sont écrit sur le socket lorsque des
paquets sont envoyés sur ce même socket.
*/
void test_readloop(void) {
    // On crée un socket pour le sender
    int sfd_s = get_sender_socket();
    if(sfd_s == -1) {
        fprintf(stderr, "Failed to get_sender_socket() failed.\n");
        return;
    }

    // On lance le receiver dans un thread
    pthread_t t;
    int err = pthread_create(&t, NULL, thread_receiver, NULL);
    if(err != 0) {
        fprintf(stderr, "pthread_create() failed.\n");
        return;
    }

    // On doit attendre que le receiver soit prêt pour envoyer le premier paquet
    while(!receiver_ready) {}
    char buf_to_send[12]; size_t len_to_send = 12;
    char buf_to_receive[4]; size_t len_to_receive = 4;

    // Envoi du premier paquet
    pkt_t *p_s = create_packet(PTYPE_DATA, 5, 0, 4, "abcd");
    CU_ASSERT_EQUAL(pkt_encode(p_s, buf_to_send, &len_to_send), PKT_OK);
    write_on_socket(sfd_s, buf_to_send, len_to_send);

    pkt_del(p_s);

    /*
    La connexion a été correctement établie à partir d'ici, vérifions qu'un
    ack de seqnum 1 a bien été renvoyé sur le socket du sender.
    */

    // Lecture de l'ack reçu en échange du premier paquet
    ssize_t num_read = read(sfd_s, buf_to_receive, len_to_receive);
    if(num_read == -1) {
        fprintf(stderr, "read() failed.\n");
    }

    // Comparaison de l'ack reçu et de l'ack attendu
    pkt_t *p_r = pkt_new();
    CU_ASSERT_EQUAL(pkt_decode(buf_to_receive, num_read, p_r), PKT_OK);
    pkt_t *p_e = create_packet(PTYPE_ACK, 30, 1, 0, NULL);
    CU_ASSERT_PKT_HEADER_EQUAL(p_r, p_e);

    pkt_del(p_r); pkt_del(p_e);

    // Envoi d'un deuxième paquet pas en séquence
    p_s = create_packet(PTYPE_DATA, 5, 2, 4, "ijkl");
    CU_ASSERT_EQUAL(pkt_encode(p_s, buf_to_send, &len_to_send), PKT_OK);
    write_on_socket(sfd_s, buf_to_send, len_to_send);

    pkt_del(p_s);

    // Lecture de l'ack reçu en échange du deuxième paquet
    num_read = read(sfd_s, buf_to_receive, len_to_receive);
    if(num_read == -1) {
        fprintf(stderr, "read() failed.\n");
    }

    // Comparaison de l'ack reçu et de l'ack attendu
    p_r = pkt_new();
    CU_ASSERT_EQUAL(pkt_decode(buf_to_receive, num_read, p_r), PKT_OK);
    p_e = create_packet(PTYPE_ACK, 30, 1, 0, NULL);
    CU_ASSERT_PKT_HEADER_EQUAL(p_r, p_e);

    pkt_del(p_r); pkt_del(p_e);

    // Envoi d'un troisème paquet qui vient combler le trou dans la window
    p_s = create_packet(PTYPE_DATA, 5, 1, 4, "efgh");
    CU_ASSERT_EQUAL(pkt_encode(p_s, buf_to_send, &len_to_send), PKT_OK);
    write_on_socket(sfd_s, buf_to_send, len_to_send);

    pkt_del(p_s);

    // Lecture de l'ack reçu en échange du troisème paquet
    num_read = read(sfd_s, buf_to_receive, len_to_receive);
    if(num_read == -1) {
        fprintf(stderr, "read() failed.\n");
    }

    // Comparaison de l'ack reçu et de l'ack attendu
    p_r = pkt_new();
    CU_ASSERT_EQUAL(pkt_decode(buf_to_receive, num_read, p_r), PKT_OK);
    p_e = create_packet(PTYPE_ACK, 29, 3, 0, NULL);
    CU_ASSERT_PKT_HEADER_EQUAL(p_r, p_e);

    pkt_del(p_r); pkt_del(p_e);

    // Envoi du paquet indiquant la fin du transfert
    p_s = create_packet(PTYPE_DATA, 5, 3, 0, NULL);
    CU_ASSERT_EQUAL(pkt_encode(p_s, buf_to_send, &len_to_send), PKT_OK);
    write_on_socket(sfd_s, buf_to_send, len_to_send);

    // Lecture de l'ack reçu en échange du paquet final
    num_read = read(sfd_s, buf_to_receive, len_to_receive);
    if(num_read == -1) {
        fprintf(stderr, "read() failed.\n");
    }

    // Comparaison de l'ack reçu et de l'ack attendu
    p_r = pkt_new();
    CU_ASSERT_EQUAL(pkt_decode(buf_to_receive, num_read, p_r), PKT_OK);
    p_e = create_packet(PTYPE_ACK, 30, 4, 0, NULL);
    CU_ASSERT_PKT_HEADER_EQUAL(p_r, p_e);

    err = pthread_join(t, NULL);
    if(err != 0) {
        fprintf(stderr, "pthread_join() failed.\n");
        return;
    }
}




/*--------------------------------------------------------------------------+
| Tests pour la fonction read_write_loop (et sous-fonctions) du sender      |
+---------------------------------------------------------------------------*/

/*
Vérifie que l'initialisation de la fenêtre de réception
se déroule comme prévu.
*/
void test_initWindowTimer(void){
    initWindow();


    struct timespec value;
    struct timespec interval;
    value.tv_sec = 5;
    value.tv_nsec = 0;
    interval.tv_sec = 0;
    interval.tv_nsec = 0;

    //    initTimer(value, interval, itimerspec);
    CU_ASSERT_EQUAL(value.tv_sec, 5);
    CU_ASSERT_EQUAL(value.tv_nsec, 0);
    CU_ASSERT_EQUAL(interval.tv_sec, 0);
    CU_ASSERT_EQUAL(interval.tv_nsec, 0);
    int i;
    for(i = 0; i < WINDOW_SIZE; i++) {
        CU_ASSERT_EQUAL(windowTab[i].ack, 0);
    }

    return;
}

void test_write_on_socket2(void){
    return;
}


/* Fonction de capture du signal SIGALRM */
void hdl2()
{
    struct itimerspec timerspec;
    if(timer_gettime(windowTab[1].timerid, &timerspec)!=0)
    {
        perror("timer_gettime()");
        return;
    }
    if(timerspec.it_value.tv_sec==0)
    {
        windowTab[1].ack = 0;
    }
}


/* Le test ne réussira que s'il sort de sa boucle, càd si la fonction qui
capte le signal SIGALRM fonctionne et change la valeur de l'ack */
void test_hdl(void){
    signal(SIGALRM, hdl2);
    windowTab[1].ack = -1;

    if(timer_create(CLOCK_REALTIME,NULL,&windowTab[1].timerid)!=0)
    {
        perror("timer_create()");
        return;
    }

    struct timespec value;
    value.tv_sec = 3;
    value.tv_nsec = 0;
    struct timespec interval;
    interval.tv_sec = 0;
    interval.tv_nsec = 0;

    struct itimerspec timerspec;
    timerspec.it_interval = interval;
    timerspec.it_value = value;

    if(timer_settime(windowTab[1].timerid,0,&timerspec,NULL)!=0)
    {
        perror("timer_settime()");
        return;
    }

    while(windowTab[1].ack == -1)
    {
        //nothing
    }

    return;
}


void timer_helper(void)
{
    int i;
    for(i=0; i<32; i++)
    {
        windowTab[i].timerid = 0;
        CU_ASSERT_EQUAL(timer_create(CLOCK_REALTIME,NULL,&windowTab[i].timerid),0);
    }
}

void test_seqnum_valid(void){
    // case [253|254|255|0...|28]
    lastack = 253;
    int i = 253;
    while(i<29 || i>252)
    {
        CU_ASSERT_EQUAL(seqnum_valid(i), true);
        i=((i+1)%256);
    }
    while(i>28 && i<253)
    {
        CU_ASSERT_EQUAL(seqnum_valid(i), false);
        i++;
    }
    CU_ASSERT_EQUAL(seqnum_valid(29), false);

    return;
}

void test_slideWindowTab(void){
    lastack = 253;
    int b = 3 - lastack;
    timer_helper();
    slideWindowTab(b);
    int i;
    for(i=25; i<32; i++)
    {
        CU_ASSERT_EQUAL(windowTab[i].ack, 0);
    }
    return;
}


/*
Vérifie le bon fonctionnement de la foncion read_loop (qui
constitue le coeur du receiver). Il s'agit en gros de vérifier
si les ack/nack corrects sont écrit sur le socket lorsque des
paquets sont envoyés sur ce même socket.
*/
void test_read_write_loop(void) {

    // On crée un socket pour le sender
    struct sockaddr_in6 addr;
    const char *err = real_address("::", &addr);
    if (err) {
        fprintf(stderr, "Could not resolve hostname %s: %s\n", "::", err);
        return;
    }
    int sfd_sender = create_socket(NULL, -1, &addr, 1200); /* Connected */
    printf("test1 \n");

    int sfd_receiver = create_socket(&addr, 1200, NULL, -1); /* Bound */
    if(connect(sfd_receiver, (const struct sockaddr *) &addr, sizeof(addr)) == 0) {
        fprintf(stderr, "Receiver is connected to the sender.\n");
    }


    printf("test2 \n");

    // On crée un fichier dans lequel on écrit qlqchose
    FILE * fp; // si fopen
//    int fp; // si open
    char buf[MAX_PKT_SIZE];
    char buf_to_send[12];
    size_t len_to_send = 12;
    printf("test3 \n");
//    fp = open("filetxt",O_CREAT,S_IRUSR|S_IWUSR);
    fp=fopen("filetxt","r+");
//    printf("%d fp \n", fp);
    if(fp == NULL)
    {
        perror("open()");
        return;
    }
    printf("test5 \n");

    char x[] = "Motde13bytes";
    printf("fwrite %d et %d \n", (int) fwrite(x, sizeof(x[0]), sizeof(x), fp), (int) sizeof(x));
//    ssize_t ret = write(fp, x, 10);
/*    if (ret == -1) {
        printf("rrno %d \n", errno);
        perror("write() mot12");
        return;
    }
    printf("test6 %d \n", (int) ret);
    */
    printf("test6 \n");
    read_write_loop(sfd_sender,"../filetxt");


    /* On lit les données sur le socket */
    ssize_t read_on_socket = read(sfd_sender, (void *) buf, MAX_PKT_SIZE);
    if (read_on_socket <= 0) {
        perror("read()");
        return;
    }
    printf("test7 lu sur le socket %d bytes \n", (int) read_on_socket);
    /* On décode les données reçues dans d_pkt */
    pkt_t * p_r = pkt_new();
    pkt_status_code c = pkt_decode((const char *) buf, (const size_t) read_on_socket, p_r);
    printf("test8 \n");
    // Comparaison des données reçues et attendues
    CU_ASSERT_EQUAL(c, PKT_OK);
    pkt_t * p_e = create_packet(PTYPE_DATA, 31, 1, 12, "Motde12bytes");
    CU_ASSERT_PKT_EQUAL(p_r, p_e, 0);

    printf("test9 \n");
    pkt_del(p_r); pkt_del(p_e);

    /*
    La connexion a été correctement établie à partir d'ici, vérifions qu'un
    ack de seqnum 1 a bien été renvoyé sur le socket du sender.
    */

    // Lecture de l'ack reçu en échange du premier paquet
    p_e = create_packet(PTYPE_ACK, 30, 1, 0, NULL);
    CU_ASSERT_EQUAL(pkt_encode(p_e, buf_to_send, &len_to_send), PKT_OK);
    write_on_socket(sfd_sender, buf_to_send, len_to_send);

    // Vérifions qu'il a bien pris les mesures nécessaires à la réception d'un ack
    CU_ASSERT_EQUAL(windowTab[0].ack, 0);
    CU_ASSERT_EQUAL(windowFree, 32);

    pkt_del(p_e);


//    fclose(fp);
    return;

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
        (NULL == CU_add_test(basic, "decode unconsistent", decode_unconsistent)) ||
        (NULL == CU_add_test(basic, "init_window", test_init_window)) ||
        (NULL == CU_add_test(basic, "add_in_window", test_add_in_window)) ||
        (NULL == CU_add_test(basic, "shift_window", test_shift_window)) ||
        (NULL == CU_add_test(basic, "in_window", test_in_window)) ||
        (NULL == CU_add_test(basic, "build_ack", test_build_ack)) ||
        (NULL == CU_add_test(basic, "build_nack", test_build_nack)) ||
        (NULL == CU_add_test(basic, "initWindowTimer", test_initWindowTimer)) ||
        (NULL == CU_add_test(basic, "slideWindowTab", test_slideWindowTab)) ||
        (NULL == CU_add_test(basic, "hdl", test_hdl)) ||
        (NULL == CU_add_test(basic, "seqnum_valid", test_seqnum_valid)) ||
        //(NULL == CU_add_test(basic, "read_write_loop", test_read_write_loop)) ||
        (NULL == CU_add_test(basic, "read_loop", test_readloop))
    )
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_run_tests();
    return CU_get_error();
}
