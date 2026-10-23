/*---------------------------------------------------------------*
 * Project Title : FontToolkit
 * Filename      : duplicates.c
 * Version       : 1.2.0-dev
 * Description   : SHA-256 and binary file comparison.
 *---------------------------------------------------------------*/

#include "duplicates.h"

#include <stdio.h>
#include <string.h>

#include <openssl/evp.h>

#define DUPLICATES_BUFFER_SIZE 16384

/*
 * Calculate the SHA-256 digest of a file.
 */
int duplicates_sha256_file(
    const char *path,
    uint8_t digest[DUPLICATES_SHA256_SIZE]
)
{
    FILE *file = NULL;
    EVP_MD_CTX *context = NULL;

    unsigned char buffer[DUPLICATES_BUFFER_SIZE];
    unsigned int digest_length = 0;

    size_t bytes_read;
    int success = 0;

    if (path == NULL || digest == NULL) {
        return 0;
    }

    file = fopen(path, "rb");

    if (file == NULL) {
        return 0;
    }

    context = EVP_MD_CTX_new();

    if (context == NULL) {
        fclose(file);
        return 0;
    }

    if (EVP_DigestInit_ex(
            context,
            EVP_sha256(),
            NULL
        ) != 1) {
        goto cleanup;
    }

    while ((bytes_read = fread(
                buffer,
                1,
                sizeof(buffer),
                file
            )) > 0) {

        if (EVP_DigestUpdate(
                context,
                buffer,
                bytes_read
            ) != 1) {
            goto cleanup;
        }
    }

    if (ferror(file)) {
        goto cleanup;
    }

    if (EVP_DigestFinal_ex(
            context,
            digest,
            &digest_length
        ) != 1) {
        goto cleanup;
    }

    if (digest_length != DUPLICATES_SHA256_SIZE) {
        goto cleanup;
    }

    success = 1;

cleanup:

    EVP_MD_CTX_free(context);
    fclose(file);

    return success;
}


/*
 * Compare file contents in blocks.
 *
 * Hashes are not needed here because this function compares
 * the actual bytes directly.
 */
DuplicateCompareResult duplicates_compare_bytes(
    const char *path_a,
    const char *path_b
)
{
    FILE *file_a = NULL;
    FILE *file_b = NULL;

    unsigned char buffer_a[DUPLICATES_BUFFER_SIZE];
    unsigned char buffer_b[DUPLICATES_BUFFER_SIZE];

    size_t count_a;
    size_t count_b;

    DuplicateCompareResult result = DUPLICATES_ERROR;

    if (path_a == NULL || path_b == NULL) {
        return DUPLICATES_ERROR;
    }

    file_a = fopen(path_a, "rb");

    if (file_a == NULL) {
        goto cleanup;
    }

    file_b = fopen(path_b, "rb");

    if (file_b == NULL) {
        goto cleanup;
    }

    for (;;) {

        count_a = fread(
            buffer_a,
            1,
            sizeof(buffer_a),
            file_a
        );

        count_b = fread(
            buffer_b,
            1,
            sizeof(buffer_b),
            file_b
        );

        if (count_a != count_b) {
            result = DUPLICATES_DIFFERENT;
            break;
        }

        if (count_a > 0 &&
            memcmp(buffer_a, buffer_b, count_a) != 0) {

            result = DUPLICATES_DIFFERENT;
            break;
        }

        if (count_a == 0) {

            if (ferror(file_a) || ferror(file_b)) {
                result = DUPLICATES_ERROR;
            } else {
                result = DUPLICATES_IDENTICAL;
            }

            break;
        }
    }

cleanup:

    if (file_a != NULL) {
        fclose(file_a);
    }

    if (file_b != NULL) {
        fclose(file_b);
    }

    return result;
}


/*
 * Compare files using SHA-256 first.
 *
 * A byte-by-byte comparison is performed when the hashes match.
 */
DuplicateCompareResult duplicates_compare_files(
    const char *path_a,
    const char *path_b
)
{
    uint8_t digest_a[DUPLICATES_SHA256_SIZE];
    uint8_t digest_b[DUPLICATES_SHA256_SIZE];

    if (path_a == NULL || path_b == NULL) {
        return DUPLICATES_ERROR;
    }

    if (!duplicates_sha256_file(path_a, digest_a)) {
        return DUPLICATES_ERROR;
    }

    if (!duplicates_sha256_file(path_b, digest_b)) {
        return DUPLICATES_ERROR;
    }

    if (memcmp(
            digest_a,
            digest_b,
            DUPLICATES_SHA256_SIZE
        ) != 0) {

        return DUPLICATES_DIFFERENT;
    }

    return duplicates_compare_bytes(path_a, path_b);
}

/* End of file: duplicates.c */