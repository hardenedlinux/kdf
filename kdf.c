#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sodium.h>

#define ACTION_NONE   0
#define ACTION_DERIVE 1
#define ACTION_MERGE  2

/* XXX: This is a dangerous prototype, DO NOT use this program for any purposes and
 *      never assume it is secure! */

int main(int argc, char **argv)
{
    uint8_t action = ACTION_NONE;
    uint8_t key[crypto_box_SEEDBYTES];
    uint8_t salt[crypto_pwhash_SALTBYTES] = {
	    /* XXX: random bytes from CSPRNG */
	    0xe9, 0x6c, 0xba, 0xbc, 0x67, 0xd4, 0x67, 0x95,
	    0xa9, 0x1d, 0xd8, 0xb9, 0x13, 0xf6, 0x23, 0x14
    };
    uint8_t key1[32];
    uint8_t key2[32];
    uint8_t hash[crypto_auth_hmacsha512_BYTES];

    char str1[128];
    int retval = 0;

    if (sodium_init() < 0) {
	    fprintf(stderr, "libsodium_init() < 0, failed!\n");
	    return 1;
    }
    retval = sodium_mlock(key, crypto_box_SEEDBYTES) == -1 || retval == 1 ? 1 : 0;
    retval = sodium_mlock(salt, crypto_pwhash_SALTBYTES) == -1 || retval == 1 ? 1 : 0;
    retval = sodium_mlock(key1, 32) == -1 || retval == 1 ? 1 : 0;
    retval = sodium_mlock(key2, 32) == -1 || retval == 1 ? 1 : 0;
    retval = sodium_mlock(hash, crypto_auth_hmacsha512_BYTES) == -1 || retval == 1 ? 1 : 0;
    retval = sodium_mlock(str1, 128) == -1 || retval == 1 ? 1 : 0;
    if (retval == 1) {
	    fprintf(stderr, "sodium_mlock() failed!\n");
	    return 1;
    }

    if (argc == 1) {
	    fprintf(stderr, "no arguments!\n");
	    return 1;
    }
    else if (argc > 4) {
            fprintf(stderr, "too many arguments!\n");
	    return 1;
    }

    if (argc == 3 && strcmp(argv[1], "-d") == 0) {
	    /* derive two keys */
	    action = ACTION_DERIVE;
    }
    if (argc == 4 && strcmp(argv[1], "-m") == 0) {
	    /* merge two keys and derive a new one */
	    action = ACTION_MERGE;
    }
    if (action == ACTION_NONE) {
	    fprintf(stderr, "unknown action!\n");
	    return 1;
    }

    fprintf(stderr, "with %d bytes key, %d bytes salt, %d bytes kdf, and %d bytes hashkey\n",
	    crypto_box_SEEDBYTES, crypto_pwhash_SALTBYTES, crypto_kdf_KEYBYTES, crypto_auth_hmacsha512_KEYBYTES);

    if (action == ACTION_DERIVE) {
	    if (
		crypto_pwhash(
		    key, crypto_box_SEEDBYTES,
		    argv[2], strlen(argv[2]), salt,
		    crypto_pwhash_OPSLIMIT_INTERACTIVE,
		    crypto_pwhash_MEMLIMIT_INTERACTIVE,
		    crypto_pwhash_ALG_DEFAULT
		) != 0
	    ) {
		    fprintf(stderr, "pwhash() failed!\n");
	    }
	sodium_bin2hex(str1, 128, key, crypto_box_SEEDBYTES);
	fprintf(stderr, "derived %s\n", str1);

	crypto_kdf_derive_from_key(key1, 32, 1, "9c62faa549c92770926ea4bb", key);
	sodium_bin2hex(str1, 128, key1, 32);
	fprintf(stderr, "derived key1 %s\n", str1);
	printf("%s\n", str1);

	crypto_kdf_derive_from_key(key2, 32, 2, "9c62faa549c92770926ea4bb", key);
	sodium_bin2hex(str1, 128, key2, 32);
	fprintf(stderr, "derived key2 %s\n", str1);
	printf("%s\n", str1);

    }
    else if (action == ACTION_MERGE) {
        if (sodium_hex2bin(key1, 32, argv[2], 64, NULL, NULL, NULL) == -1) {
		fprintf(stderr, "invalid hex!\n");
		return 1;
	}
        if (sodium_hex2bin(key2, 32, argv[3], 64, NULL, NULL, NULL) == -1) {
		fprintf(stderr, "invalid hex!\n");
		return 1;
	}
        crypto_auth_hmacsha512(hash, key1, 32, key2);
	sodium_bin2hex(str1, 128, hash, crypto_auth_hmacsha512_KEYBYTES);
	fprintf(stderr, "derived hash %s\n", str1);
    }

    sodium_memzero(key, crypto_box_SEEDBYTES);
    sodium_memzero(salt, crypto_pwhash_SALTBYTES);
    sodium_memzero(key1, 32);
    sodium_memzero(key2, 32);
    sodium_memzero(hash, crypto_auth_hmacsha512_BYTES);
    sodium_memzero(str1, 128);
    return 0;
}
