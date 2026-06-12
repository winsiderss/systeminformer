// LICENSE
//
// This software is dual-licensed to the public domain and under the following
// license: you are granted a perpetual, irrevocable license to copy, modify,
// publish, and distribute this file as you see fit.
//
// VERSION
// 0.2.0 (2017-02-18) Scored matches perform exhaustive search for best score
// 0.1.0 (2016-03-28) Initial release
//
// AUTHOR
// Forrest Smith
//
// NOTES
// Compiling
// You MUST add '#define FTS_FUZZY_MATCH_IMPLEMENTATION' before including this header in ONE source file to create implementation.
//
// fuzzy_match_simple(...)
// Returns true if each character in pattern is found sequentially within str
//
// fuzzy_match(...)
// Returns true if pattern is found AND calculates a score.
// Performs exhaustive search via recursion to find all possible matches and match with highest score.
// Scores values have no intrinsic meaning. Possible score range is not normalized and varies with pattern.
// Recursion is limited internally (default=10) to prevent degenerate cases (pattern="aaaaaa" str="aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa")
// Uses uint8_t for match indices. Therefore patterns are limited to 256 characters.
// Score system should be tuned for YOUR use case. Words, sentences, file names, or method names all prefer different tuning.

#ifndef FTS_FUZZY_MATCH_H
#define FTS_FUZZY_MATCH_H

#include <stdint.h> // uint8_t
#include <ctype.h>  // tolower, toupper
#include <string.h> // memcpy

#ifdef __cplusplus
extern "C" {
#endif

static int fts_fuzzy_match_simple(const char *pattern, const char *str);
static int fts_fuzzy_match(const char *pattern, const char *str, int *outScore);
static int fts_fuzzy_match_with_indices(const char *pattern, const char *str, int *outScore, uint8_t *matches, int maxMatches);

#ifndef FTS_FUZZY_MATCH_IMPLEMENTATION

static int fts_fuzzy_match_simple(const char *pattern, const char *str) {
    while (*pattern && *str) {
        if (tolower((unsigned char)*pattern) == tolower((unsigned char)*str))
            ++pattern;
        ++str;
    }
    return *pattern == '\0';
}

static int fts_fuzzy_match(const char *pattern, const char *str, int *outScore) {
    uint8_t matches[256];
    return fts_fuzzy_match_with_indices(pattern, str, outScore, matches, sizeof(matches));
}

static int recursive_fuzzy_match(const char *pattern, const char *str, int *outScore, const char *strBegin,
    const uint8_t *srcMatches, uint8_t *matches, int maxMatches, int nextMatch,
    int *recursionCount, int recursionLimit) {

    ++(*recursionCount);
    if (*recursionCount >= recursionLimit)
        return 0;

    if (*pattern == '\0' || *str == '\0')
        return 0;

    int recursiveMatch = 0;
    uint8_t bestRecursiveMatches[256];
    int bestRecursiveScore = 0;

    int first_match = 1;
    while (*pattern && *str) {
        if (tolower((unsigned char)*pattern) == tolower((unsigned char)*str)) {
            if (nextMatch >= maxMatches)
                return 0;
            if (first_match && srcMatches) {
                memcpy(matches, srcMatches, nextMatch);
                first_match = 0;
            }
            uint8_t recursiveMatches[256];
            int recursiveScore;
            if (recursive_fuzzy_match(pattern, str + 1, &recursiveScore, strBegin, matches, recursiveMatches, sizeof(recursiveMatches), nextMatch, recursionCount, recursionLimit)) {
                if (!recursiveMatch || recursiveScore > bestRecursiveScore) {
                    memcpy(bestRecursiveMatches, recursiveMatches, 256);
                    bestRecursiveScore = recursiveScore;
                }
                recursiveMatch = 1;
            }
            matches[nextMatch++] = (uint8_t)(str - strBegin);
            ++pattern;
        }
        ++str;
    }

    int matched = *pattern == '\0';
    if (matched) {
        const int sequential_bonus = 15;
        const int separator_bonus = 30;
        const int camel_bonus = 30;
        const int first_letter_bonus = 15;
        const int leading_letter_penalty = -5;
        const int max_leading_letter_penalty = -15;
        const int unmatched_letter_penalty = -1;
        while (*str)
            ++str;
        *outScore = 100;
        int penalty = leading_letter_penalty * matches[0];
        if (penalty < max_leading_letter_penalty)
            penalty = max_leading_letter_penalty;
        *outScore += penalty;
        int unmatched = (int)(str - strBegin) - nextMatch;
        *outScore += unmatched_letter_penalty * unmatched;
        for (int i = 0; i < nextMatch; ++i) {
            uint8_t currIdx = matches[i];
            if (i > 0) {
                uint8_t prevIdx = matches[i - 1];
                if (currIdx == (prevIdx + 1))
                    *outScore += sequential_bonus;
            }
            if (currIdx > 0) {
                char neighbor = strBegin[currIdx - 1];
                char curr = strBegin[currIdx];
                if (islower((unsigned char)neighbor) && isupper((unsigned char)curr))
                    *outScore += camel_bonus;
                int neighborSeparator = neighbor == '_' || neighbor == ' ';
                if (neighborSeparator)
                    *outScore += separator_bonus;
            } else {
                *outScore += first_letter_bonus;
            }
        }
    }
    if (recursiveMatch && (!matched || bestRecursiveScore > *outScore)) {
        memcpy(matches, bestRecursiveMatches, maxMatches);
        *outScore = bestRecursiveScore;
        return 1;
    } else if (matched) {
        return 1;
    } else {
        return 0;
    }
}

static int fts_fuzzy_match_with_indices(const char *pattern, const char *str, int *outScore, uint8_t *matches, int maxMatches) {
    int recursionCount = 0;
    int recursionLimit = 10;
    return recursive_fuzzy_match(pattern, str, outScore, str, NULL, matches, maxMatches, 0, &recursionCount, recursionLimit);
}

#endif // FTS_FUZZY_MATCH_IMPLEMENTATION

#ifdef __cplusplus
}
#endif

#endif // FTS_FUZZY_MATCH_H
