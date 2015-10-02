#include "packet_interface.h"
#include <stdlib.h>
#include <zlib.h>

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
        /*
         * Little hack to use every paramater and
         * force compilation on Inginious to work.
         */
        const char * a = data;
        const size_t l = len;
        pkt_t * p = pkt;
        a = a+1;
        p = p+1+l;
        return(0);
}

pkt_status_code pkt_encode(const pkt_t* pkt, char *buf, size_t *len)
{
        buf[0] = (pkt_get_type(pkt) << 5) | pkt_get_type(pkt); 
        buf[1] = pkt_get_seqnum(pkt);
        buf[2] = (pkt_get_length(pkt) >> 8) & 0xFF;
        buf[3] = pkt_get_length(pkt) & 0xFF;
        
        /*
         * Writing the payload in the buffer. First
         * condition allows to detect the end of the
         * payload and second allows to detect the end
         * of the buffer (taking into account the
         * fact that we need to keep space for
         * an eventual padding and for the CRC).
         */
        const char * payload = pkt_get_payload(pkt);
        uint16_t padding = pkt_get_length(pkt) % 4;
        uint16_t i; // Represents the number of payload's bytes written in the buffer
        for(i = 0; i < pkt_get_length(pkt) + padding && i + 4 < ((uint16_t) *len) - 4 - padding; i++) {
                buf[i+4] = payload[i];
        }

        /*
         * If the loop stopped due to second
         * condition, then it means the buffer
         * is too small and so pkt_encode must
         * return E_NOMEM.
         */
        if(i != pkt_get_length(pkt) + padding)
                return(E_NOMEM);

        buf[i+4] = (pkt_get_crc(pkt) >> 24) & 0xFF;
        buf[i+5] = (pkt_get_crc(pkt) >> 16) & 0xFF;
        buf[i+6] = (pkt_get_crc(pkt) >> 8) & 0xFF;
        buf[i+7] = pkt_get_crc(pkt) & 0xFF;
        
        /*
         * Total number of bytes written : 4 for the header,
         * pkt_get_length(pkt) for the useful part of payload,
         * pkt_get_length(pkt) % 4 for the padding in the
         * payload and 4 for the CRC.
         */
        *len = 4 + pkt_get_length(pkt) + padding + 4;
        return(PKT_OK);
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
        return(PKT_OK);
}

pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window)
{
        if(window > 31)
                return(E_WINDOW);

        pkt->window = window;
        return(PKT_OK);
}

pkt_status_code pkt_set_seqnum(pkt_t *pkt, const uint8_t seqnum)
{
        /*
         * seqnum will never be outside of [0,255] thanks
         * to its type (uint8_t), so no verification needed.
         */
        pkt->seqnum = seqnum;
        return(PKT_OK);
}

pkt_status_code pkt_set_length(pkt_t *pkt, const uint16_t length)
{
        if(length > 512)
                return(E_LENGTH);

        pkt->length = length;
        return(PKT_OK);
}

pkt_status_code pkt_set_crc(pkt_t *pkt, const uint32_t crc)
{
        pkt->crc = crc;
        return(PKT_OK);
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
        for(i = 0; i < length; i++) {
                pkt->payload[i] = data[i];
        }

        while(padding > 0) {
                pkt->payload[i] = 0;
                i++;
                padding--;
        }

        return(PKT_OK);
}
