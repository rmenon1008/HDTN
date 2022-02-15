/***************************************************************************
 * NASA Glenn Research Center, Cleveland, OH
 * Released under the NASA Open Source Agreement (NOSA)
 * May  2021
 *
 ****************************************************************************
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <cstdlib>
#include "codec/bpv6.h"
#include "codec/bpv6-ext-block.h"
#include <inttypes.h>
#include "Sdnv.h"





uint8_t bpv6_bplib_bib_block::bpv6_bib_decode(const char* buffer, const size_t offset, const size_t bufsz) {
    uint64_t index = offset;
    uint16_t crc16 = 0;
    uint8_t sdnvSize;

    num_targets = SdnvDecodeU64((const uint8_t *)&buffer[index], &sdnvSize, 16); //TODO_SDNV
    if (sdnvSize == 0) {
        return 0; //return 0 on failure TODO
    }
    index += sdnvSize;

    target_type = SdnvDecodeU64((const uint8_t *)&buffer[index], &sdnvSize, 16); //TODO_SDNV
    if (sdnvSize == 0) {
        return 0; //return 0 on failure TODO
    }
    index += sdnvSize;

    target_sequence = SdnvDecodeU64((const uint8_t *)&buffer[index], &sdnvSize, 16); //TODO_SDNV
    if (sdnvSize == 0) {
        return 0; //return 0 on failure TODO
    }
    index += sdnvSize;

    cipher_suite_id = SdnvDecodeU64((const uint8_t *)&buffer[index], &sdnvSize, 16); //TODO_SDNV
    if (sdnvSize == 0) {
        return 0; //return 0 on failure TODO
    }
    index += sdnvSize;

    cipher_suite_flags = SdnvDecodeU64((const uint8_t *)&buffer[index], &sdnvSize, 16); //TODO_SDNV
    if (sdnvSize == 0) {
        return 0; //return 0 on failure TODO
    }
    index += sdnvSize;

    num_security_results = SdnvDecodeU64((const uint8_t *)&buffer[index], &sdnvSize, 16); //TODO_SDNV
    if (sdnvSize == 0) {
        return 0; //return 0 on failure TODO
    }
    index += sdnvSize;

    security_result_type = SdnvDecodeU64((const uint8_t *)&buffer[index], &sdnvSize, 16);  //TODO_SDNV
    if (sdnvSize == 0) {
        return 0; //return 0 on failure TODO
    }
    index += sdnvSize;

    security_result_len = SdnvDecodeU64((const uint8_t *)&buffer[index], &sdnvSize, 16);  //TODO_SDNV
    if (sdnvSize == 0) {
        return 0; //return 0 on failure TODO
    }
    index += sdnvSize;

    //uint8_t valptr[block->security_result_len];
    //memcpy(&valptr, buffer+index,block->security_result_len);
    const uint8_t * const valptr = (const uint8_t *)&buffer[index];
    //crc is big endian, not encoded as sdnv
#define BPLIB_BIB_CRC16_X25                1
    if (cipher_suite_id == BPLIB_BIB_CRC16_X25 && security_result_len == 2) {
        crc16 |= (((uint16_t)valptr[0]) << 8);
        crc16 |= ((uint16_t)valptr[1]);
        security_result = crc16;
    }
    index += 2;
    return 0;
}

void bpv6_bplib_bib_block::bpv6_bib_print() const {
    if (m_blockTypeCode == BPV6_BLOCK_TYPE_CODE::BPLIB_BIB) {
        printf("\nBplib Bundle Integrity Block [type %u]\n", static_cast<unsigned int>(m_blockTypeCode));
        bpv6_block_flags_print();
        printf("Block length: %" PRIu64 " bytes\n", m_blockTypeSpecificDataLength);
        printf("Number of security targets: %" PRIu64 "\n", num_targets);
        printf("Security target type: %" PRIu64 "\n", target_type);
        printf("Security target sequence: %" PRIu64 "\n", target_sequence);
        printf("Cipher suite id: %" PRIu64 "\n", cipher_suite_id);
        printf("Cipher suite flags: %" PRIu64 "\n", cipher_suite_flags);
        printf("Number of security results: %" PRIu64 "\n", num_security_results);
        printf("Security result type: %" PRIu64 "\n", security_result_type);
        printf("Security result length: %" PRIu64 "\n", security_result_len);
        printf("Security result : %" PRIu16 "\n\n", security_result);

    }
    else {
        printf("Block is not bplib Bundle Integrity Block\n\n");
    }
}

uint8_t bpv6_bundle_age_ext_block::bpv6_bundle_age_decode(const char* buffer, const size_t offset, const size_t bufsz)
{
    uint64_t index = offset;
    uint8_t sdnvSize;

    bundle_age = SdnvDecodeU64((const uint8_t *)&buffer[index], &sdnvSize, 16); //TODO_SDNV
    if (sdnvSize == 0) {
        return 0; //return 0 on failure TODO
    }
    index += sdnvSize;

    return 0;
}

void bpv6_bundle_age_ext_block::bpv6_bundle_age_print() const {
    if (m_blockTypeCode == BPV6_BLOCK_TYPE_CODE::BUNDLE_AGE) {
        printf("\n Bundle Age Block [type %u]\n", static_cast<unsigned int>(m_blockTypeCode));
        bpv6_block_flags_print();
        printf("Block length: %" PRIu64 " bytes\n", m_blockTypeSpecificDataLength);
        printf("Bundle Age: %" PRIu64 "\n\n", bundle_age);
    }
    else {
        printf("Block is not bundle age block\n\n");
    }
}


uint8_t bpv6_eid::char_to_bpv6_eid(const char* buffer, const size_t offset, const size_t eid_len, const size_t bufsz) {
    char * pch;
    char* endpch;
    char eid_str[45];
    //uint8_t node_len;
    if (bufsz < offset + eid_len) {
        printf("Bad string length: %" PRIu64 "\n", static_cast<uint64_t>(eid_len));
        return 0;
    }
    if (buffer[offset] != 'i' || buffer[offset + 1] != 'p' || buffer[offset + 2] != 'n' || buffer[offset + 3] != ':') {
        printf("Not IPN EID\n");
        return 0;  // make sure this is ipn scheme eid
    }
    else {
        memcpy(eid_str, &buffer[offset + 4], eid_len);
        pch = strchr(eid_str, '.');
        if (pch != NULL) {
            node = strtoull((const char*)eid_str, &pch, 10);
            pch++;
            service = strtoull((const char*)pch, &endpch, 10);
        }
        return 1;
    }
}
