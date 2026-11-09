
#include <catch2/catch_test_macros.hpp>
#include "crypt/aes.h"
#include <stdio.h>
#include <stdlib.h>

/*
 *  This test case is pulled from the NIST AES specification to test a standard AES ECB test case for the OpenRTX project
 *  https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.197.pdf
 *
 *  Initial thoughts are to only define AES256 in aes.h, no sense supporting different key sizes on an embedded device when 256 is the most future proof and has the highest bits of security
 */

TEST_CASE("Test NIST AES ECB Vector With 256 Bit Key")
{
    uint8_t key[] = {
        0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe, 0x2b, 0x73, 0xae,
        0xf0, 0x85, 0x7d, 0x77, 0x81, 0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61,
        0x08, 0xd7, 0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4,
    };

    uint8_t original_plaintext[] = {
        0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
        0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
    };

    uint8_t plaintext[] = {
        0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
        0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
    };

    uint8_t expected_ciphertext[] = {
        0xf3, 0xee, 0xd1, 0xbd, 0xb5, 0xd2, 0xa0, 0x3c,
        0x06, 0x4b, 0x5a, 0x7e, 0x3d, 0xb1, 0x81, 0xf8,
    };

    struct AES_ctx context;

    AES_init_ctx(&context, key);

    AES_ECB_encrypt(&context, plaintext);

    /*
     *  Check that plaintext now contains expected expected_ciphertext as per NIST test
     */
    REQUIRE(std::memcmp(plaintext, expected_ciphertext, sizeof(plaintext))
            == 0);

    AES_init_ctx(&context, key);

    AES_ECB_decrypt(&context, plaintext);

    /*
     *  Check that a decryption operation brings us back to the original plaintext value
     */

    REQUIRE(std::memcmp(plaintext, original_plaintext, sizeof(plaintext)) == 0);
}
