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
         * Step 1 : validate the CRC32
         */
        if(len < 4)
                return(E_UNCONSISTENT);

        uint32_t received_crc = (data[len-1] << 24) | (data[len-2] << 16) | (data[len-3] << 8) | data[len-4];
        if(len < 8)
                return(E_NOHEADER);

        uint32_t computed_crc = crc32(0, (Bytef *) data, len-4);
        if(received_crc != computed_crc)
                return(E_CRC);

        pkt_status_code c = pkt_set_crc(pkt, received_crc);

        /*
         * Step 2 : validate the type of packet
         */

        c = pkt_set_type(pkt, data[0] >> 5);
        if(c == E_TYPE)
                return(E_TYPE);

        c = pkt_set_window(pkt, data[0] & 0b00011111);
        if(c == E_WINDOW)
                return(E_WINDOW);

        c = pkt_set_seqnum(pkt, data[1]);
        
        /*
         * Step 3 : validate the length of the packet
         */
        c = pkt_set_length(pkt, (data[2] << 8) | data[3]);
        if(c == E_LENGTH)
                return(E_LENGTH);

        if(len == 8)
                return(E_NOPAYLOAD);

        c = pkt_set_payload(pkt, data+4, len - 8);
        if(c == E_NOMEM)
                return(E_NOMEM);

        return(PKT_OK);
}

pkt_status_code pkt_encode(const pkt_t* pkt, char *buf, size_t *len)
{
        buf[0] = (pkt_get_type(pkt) << 5) | pkt_get_window(pkt); 
        buf[1] = pkt_get_seqnum(pkt);
        buf[2] = (pkt_get_length(pkt) >> 8) & 0xFF;
        buf[3] = pkt_get_length(pkt) & 0xFF;
        
        /*
         * Writing the payload in the buffer. First
         * condition allows to detect the end of the
         * payload and second allows to detect the end
         * of the buffer (taking into account the
         * fact that we need to keep space for the CRC).
         */
        const char * payload = pkt_get_payload(pkt);
        uint16_t padding = (4 - (pkt_get_length(pkt) % 4)) % 4;
        uint16_t i; // Represents the number of payload's bytes written in the buffer
        for(i = 0; i < pkt_get_length(pkt) + padding && i + 4 < ((uint16_t) *len) - 4; i++) {
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

        /*
         * Compute the CRC and add it at the end
         * of buf.
         */
        uint32_t crc = crc32(0, (Bytef *) buf, 4 + pkt_get_length(pkt) + padding);
        // TODO : add crc into pkt to avoid recomputing crc later
        buf[i+4] = (crc  >> 24) & 0xFF;
        buf[i+5] = (crc >> 16) & 0xFF;
        buf[i+6] = (crc >> 8) & 0xFF;
        buf[i+7] = crc & 0xFF;
        
        /*
         * Total number of bytes written : 4 for the header,
         * pkt_get_length(pkt) for the useful part of payload,
         * padding for the padding in the
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
        int padding = (4 - (length % 4)) % 4;

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

        pkt_set_length(pkt, length);
        return(PKT_OK);
}
