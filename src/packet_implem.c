#include "packet_interface.h"
#include <stdlib.h>

/*
 * Structure reprensenting a packet.
 * Types of the different fields are
 * deduced from the getters/setters
 * defined in the interface.
 */
struct __attribute__((__packed__)) pkt {
	ptypes_t type : 3;
        uint8_t window : 5;
        uint8_t seqnum;
        uint16_t length;
        char * payload;
        uint32_t crc;
};


pkt_t* pkt_new()
{
        pkt_t * pkt = (pkt_t *) (malloc(sizeof(pkt_t)));
        if(pkt == NULL)
                return(NULL);

        return(pkt);
}

void pkt_del(pkt_t *pkt)
{
        free(pkt->payload);
        free(pkt);    
}

pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt)
{
	
}

pkt_status_code pkt_encode(const pkt_t* pkt, char *buf, size_t *len)
{
	
}

ptypes_t pkt_get_type  (const pkt_t* pkt)
{
        return(pkt->type);	
}

uint8_t  pkt_get_window(const pkt_t* pkt)
{
        return(pkt->window);
}

uint8_t  pkt_get_seqnum(const pkt_t* pkt)
{
	return(pkt->seqnum);
}

uint16_t pkt_get_length(const pkt_t* pkt)
{
	return(pkt->length);
}

uint32_t pkt_get_crc   (const pkt_t* pkt)
{
	return(pkt->crc);
}

const char* pkt_get_payload(const pkt_t* pkt)
{
       return(pkt->payload);
}


pkt_status_code pkt_set_type(pkt_t *pkt, const ptypes_t type)
{
        if(type != PTYPE_DATA && type != PTYPE_ACK && type != PTYPE_NACK)
                return(E_TYPE);

        pkt->type = type;
}

pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window)
{
        if(window < 0 || window > 31)
                return(E_WINDOW);

        pkt->window = window;
}

pkt_status_code pkt_set_seqnum(pkt_t *pkt, const uint8_t seqnum)
{
        if(seqnum < 0 || seqnum > 255)
                return(E_SEQNUM);

        pkt->seqnum = seqnum;
}

pkt_status_code pkt_set_length(pkt_t *pkt, const uint16_t length)
{
        if(length < 0 || length > 512)
                return(E_LENGTH);

        pkt->length = length;
}

pkt_status_code pkt_set_crc(pkt_t *pkt, const uint32_t crc)
{
        pkt->crc = crc;
}

pkt_status_code pkt_set_payload(pkt_t *pkt, const char *data,
                                const uint16_t length)
{
        int padding = length % 4;
        pkt_set_length(pkt, length);

        pkt->payload = (char *) malloc(sizeof(length + padding));
        if(pkt->payload == NULL)
                return(E_NOMEM);

        int i;
        for(i = 0; i < length-1; i++) {
                pkt->payload[i] = data[i];
        }

        while(padding > 0) {
                pkt->payload[i] = 0;
                padding--;
        }
}
