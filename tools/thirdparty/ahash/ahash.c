/*
 * Average Hash (aHash)
 *
 * Originally described by Dr. Neal Krawetz:
 * http://www.hackerfactor.com/blog/index.php?/archives/432-Looks-Like-It.html
 *
 * Average Hash is a perceptual hashing algorithm that generates a hash
 * based on the visual appearance of an image rather than its raw file
 * contents. This means visually similar images tend to produce similar
 * hashes even when file formats, compression levels, or metadata differ.
 *
 * --------------------------------------------------------------------
 * OVERVIEW
 * --------------------------------------------------------------------
 *
 * The algorithm is specifically designed to work with arbitrary image
 * sizes, from tiny icons to multi-megapixel photographs.
 *
 * The first step is always:
 *
 *      Original Image
 *             ↓
 *      Resize to 8×8 pixels
 *             ↓
 *      Convert to Grayscale
 *             ↓
 *      Compute Average Brightness
 *             ↓
 *      Generate 64-bit Hash
 *
 * Because every image is normalized to the same 8×8 representation,
 * the resulting hash is always exactly 64 bits wide regardless of
 * the original dimensions.
 *
 * Examples:
 *
 *      16×16     → 8×8 → 64-bit hash
 *      128×128   → 8×8 → 64-bit hash
 *      800×600   → 8×8 → 64-bit hash
 *      1920×1080 → 8×8 → 64-bit hash
 *      7680×4320 → 8×8 → 64-bit hash
 *
 * The resize stage intentionally discards most image detail and keeps
 * only the coarse visual structure and brightness distribution.
 *
 * For example, a 4K image containing millions of pixels is reduced to
 * just 64 luminance samples. Fine details, noise, JPEG artifacts,
 * metadata differences, and minor edits are largely discarded while
 * the overall appearance is preserved.
 *
 * This is what makes perceptual hashing useful:
 *
 *      Same image saved as PNG/JPEG      → Similar hash
 *      Minor resize/compression changes  → Similar hash
 *      Small edits                       → Similar hash
 *      Completely different image        → Different hash
 *
 * Note:
 *
 * The exact hash value depends on the image scaling algorithm used.
 * Bilinear, bicubic, Lanczos, nearest-neighbor, and other resampling
 * filters may produce slightly different 8×8 pixel values.
 *
 * Because of this, perceptual hashes are normally compared using
 * Hamming distance rather than exact equality.
 */

#include <stdint.h>
#include <intrin.h>

/*
 * Convert RGB to grayscale.
 *
 * Integer approximation of:
 *
 *      Gray = R×0.30 + G×0.59 + B×0.11
 *
 * Using integer coefficients:
 *
 *      77  / 256 ≈ 0.3008
 *      150 / 256 ≈ 0.5859
 *      29  / 256 ≈ 0.1133
 *
 * This avoids floating point operations and produces deterministic
 * results across compilers and platforms.
 */
FORCEINLINE BYTE RgbToGray(
    _In_ BYTE Red,
    _In_ BYTE Green,
    _In_ BYTE Blue
    )
{
    return (BYTE)((77 * Red + 150 * Green + 29 * Blue) >> 8);
}

/*
 * Compute a traditional 64-bit Average Hash.
 *
 * Input:
 *
 *      RgbPixels[64][3]
 *
 * Represents an image already resized to 8×8.
 *
 * Layout:
 *
 *      Pixel 0   = Top-left
 *      Pixel 63  = Bottom-right
 *
 * RGB Format:
 *
 *      [0] = Red
 *      [1] = Green
 *      [2] = Blue
 *
 * Returns:
 *
 *      64-bit perceptual hash.
 */
ULONG64 AverageHash64(
    const BYTE RgbPixels[64][3]
    )
{
    BYTE gray[64];
    ULONG average = 0;
    ULONG64 hash = 0;

    //
    // Convert pixels to grayscale and accumulate brightness.
    //

    for (ULONG i = 0; i < 64; i++)
    {
        gray[i] = RgbToGray(
            RgbPixels[i][0],
            RgbPixels[i][1],
            RgbPixels[i][2]
            );

        average += gray[i];
    }

    //
    // Calculate average brightness.
    //

    average /= 64;

    //
    // Generate 64 hash bits.
    //
    // Pixel >= average → 1
    // Pixel <  average → 0
    //

    for (ULONG i = 0; i < 64; i++)
    {
        if (gray[i] >= average)
        {
            hash |= (UINT64_C(1) << (63 - i));
        }
    }

    return hash;
}

/*
 * Compute Hamming distance between two 64-bit hashes.
 *
 * XOR identifies differing bits.
 * POPCNT counts the number of set bits.
 *
 * Distance Range:
 *
 *      0..64
 */
static inline int
HashHammingDistance64(
    ULONG64 Hash1,
    ULONG64 Hash2
    )
{
    return (int)__popcnt64(Hash1 ^ Hash2);
}

/*
 * Human-readable similarity classification for 64-bit hashes.
 *
 * These thresholds are commonly used as rough guidelines for aHash.
 */
const char*
HashSimilarity64(
    ULONG64 Hash1,
    ULONG64 Hash2
    )
{
    int distance = HashHammingDistance64(Hash1, Hash2);

    if (distance == 0)
        return "Identical";

    if (distance <= 5)
        return "Very similar";

    if (distance <= 10)
        return "Slight modifications";

    if (distance <= 20)
        return "Potentially different";

    return "Usually unrelated";
}

/*
 * --------------------------------------------------------------------
 * 256-BIT VARIANT
 * --------------------------------------------------------------------
 *
 * A 256-bit perceptual hash preserves substantially more information
 * than the original 64-bit hash.
 *
 * Typical implementations use:
 *
 *      16×16 grayscale image
 *
 * resulting in:
 *
 *      256 comparison bits
 *
 * Advantages:
 *
 *      • Lower collision rate
 *      • Better differentiation
 *      • More resistant to false positives
 *      • Still extremely fast to compare
 *
 * The hash is stored as four 64-bit blocks:
 */

typedef struct _AHASH256
{
    ULONG64 Block[4];
} AHASH256;

/*
 * Compute Hamming distance between two 256-bit hashes.
 *
 * Each 64-bit block is XOR'ed independently, and POPCNT counts
 * the differing bits within that block.
 *
 * A 256-bit perceptual hash preserves significantly more image
 * detail than a traditional 64-bit hash, reducing the likelihood
 * that unrelated images produce similar hashes.
 *
 * Modern x64 processors implement POPCNT in hardware, making
 * this comparison extremely fast.
 */
ULONG HashHammingDistance256(
    const AHASH256* Hash1,
    const AHASH256* Hash2
    )
{
    ULONG distance;

    distance =
        __popcnt64(Hash1->Block[0] ^ Hash2->Block[0]) + /* First 64-bit block.  */
        __popcnt64(Hash1->Block[1] ^ Hash2->Block[1]) + /* Second 64-bit block. */
        __popcnt64(Hash1->Block[2] ^ Hash2->Block[2]) + /* Third 64-bit block.  */
        __popcnt64(Hash1->Block[3] ^ Hash2->Block[3]);  /* Fourth 64-bit block. */

    return distance;
}

/*
 * Similarity classification for 256-bit hashes.
 *
 * NOTE:
 *
 * Thresholds used for 64-bit hashes should not be reused directly for
 * 256-bit hashes. Because there are four times as many bits, distances
 * are naturally larger.
 *
 * The values below are merely example starting points and should be
 * tuned using real-world image samples.
 */
const char* HashSimilarity256(
    const AHASH256* Hash1,
    const AHASH256* Hash2
    )
{
    ULONG distance = HashHammingDistance256(Hash1, Hash2);

    if (distance == 0)
        return "Identical";

    if (distance <= 20)
        return "Very similar";

    if (distance <= 40)
        return "Slight modifications";

    if (distance <= 80)
        return "Potentially different";

    return "Usually unrelated";
}

/*
 * --------------------------------------------------------------------
 * 64-BIT SIMILARITY GUIDELINES
 * --------------------------------------------------------------------
 *
 * Distance    Meaning
 * --------    ---------------------
 * 0           Identical
 * 1-5         Very similar
 * 6-10        Slight modifications
 * 11-20       Potentially different
 * 21+         Usually unrelated
 *
 * Example:
 *
 *      uint64_t hash1 = AverageHash64(Image1);
 *      uint64_t hash2 = AverageHash64(Image2);
 *
 *      int distance =
 *          HashHammingDistance64(hash1, hash2);
 *
 *      printf(
 *          "Distance: %d\n"
 *          "Similarity: %s\n",
 *          distance,
 *          HashSimilarity64(hash1, hash2)
 *          );
 */