/*---------------------------------------------------------------*
 * Project Title : FontToolkit
 * Filename      : duplicates.h
 * Version       : 1.2.0-dev
 * Description   : File duplicate detection interface.
 *---------------------------------------------------------------*/

#ifndef FONTTOOLKIT_DUPLICATES_H
#define FONTTOOLKIT_DUPLICATES_H

#include <stddef.h>
#include <stdint.h>

/*
 * Maximum number of bytes in a SHA-256 digest.
 */
#define DUPLICATES_SHA256_SIZE 32

/*
 * Result of comparing two files.
 */
typedef enum {
    DUPLICATES_ERROR = -1,
    DUPLICATES_DIFFERENT = 0,
    DUPLICATES_IDENTICAL = 1
} DuplicateCompareResult;

/*
 * Calculate the SHA-256 digest of a file.
 *
 * Parameters:
 *   path   : File path.
 *   digest : Output buffer of at least 32 bytes.
 *
 * Returns:
 *   1 on success.
 *   0 on failure.
 *
 * This function never modifies the input file.
 */
int duplicates_sha256_file(
    const char *path,
    uint8_t digest[DUPLICATES_SHA256_SIZE]
);

/*
 * Compare two files by their actual bytes.
 *
 * Returns:
 *   DUPLICATES_IDENTICAL  if the contents are identical.
 *   DUPLICATES_DIFFERENT  if the contents differ.
 *   DUPLICATES_ERROR      if comparison cannot be completed.
 */
DuplicateCompareResult duplicates_compare_bytes(
    const char *path_a,
    const char *path_b
);

/*
 * Compare two files using SHA-256.
 *
 * If the hashes match, the files are also compared byte-by-byte
 * before they are classified as identical.
 *
 * Returns:
 *   DUPLICATES_IDENTICAL  if the contents are identical.
 *   DUPLICATES_DIFFERENT  if the contents differ.
 *   DUPLICATES_ERROR      if hashing or reading fails.
 */
DuplicateCompareResult duplicates_compare_files(
    const char *path_a,
    const char *path_b
);

/* End of file: duplicates.h */

#endif