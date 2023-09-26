// Inspired by https://codepen.io/atoliks24/pen/GRRPzQm
// memorygrammer (PrimeProbe) but in c
// variable names are kept the same as their JavaScript counterparts
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <immintrin.h>
#include <stdbool.h>

// All parameters here are for Intel i7-8700
/* machine-specific parameters START */
#define CACHE_SIZE (12 * 1024 * 1024) // LLC is 12 MIB
#define CACHE_LINES (CACHE_SIZE / LINE_SIZE)
#define CACHE_ASSOC 16 // check by getconf -a | grep CACHE
#define SET_INDEX_POS 6
#define CACHE_SETS (CACHE_LINES / CACHE_ASSOC)
#define ALL_SET_OFFSET 18
#define LINE_SIZE 64
#define SETS_PER_PAGE (4096 / LINE_SIZE) // 64 on i7-8700
/* machine-specific parameters END */

// Do not skip set. Different from the original JS code.
#define SET_SKIPPING_STEP 1
#define BYTES_PER_MB (1024 * 1024)

uint32_t evictionArray[64 * BYTES_PER_MB / 4];
uint32_t setHeads[SETS_PER_PAGE];
bool setHeadsCreated = false;

uint64_t perfNow()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((uint64_t)(tv.tv_sec) * 1000000 + (uint64_t)(tv.tv_usec));
}

uint32_t *shuffle(uint32_t *arrayToShuffle, uint32_t length)
{
    uint32_t tmp, current, top = length;
    if (top)
        while (--top)
        {
            current = (uint32_t)((double)rand() / (double)RAND_MAX * (top + 1));
            tmp = arrayToShuffle[current];
            arrayToShuffle[current] = arrayToShuffle[top];
            arrayToShuffle[top] = tmp;
        }
    return arrayToShuffle;
}

void createSetHeads()
{
    if (setHeadsCreated)
        return;
    uint32_t sets = CACHE_SETS;
    uint32_t ways = CACHE_ASSOC;
    uint32_t unshuffledArray[CACHE_SETS / SETS_PER_PAGE];
    uint32_t allSetOffset = ALL_SET_OFFSET;

    for (uint32_t i = 0; i < (CACHE_SETS / SETS_PER_PAGE); i++)
    {
        unshuffledArray[i] = i;
    }

    uint32_t *shuffledArray = shuffle(unshuffledArray, CACHE_SETS / SETS_PER_PAGE);

    uint32_t set_index, element_index, line_index;
    uint32_t currentElement, nextElement;
    for (set_index = 0; set_index < SETS_PER_PAGE; set_index++)
    {
        currentElement = (shuffledArray[0] << 10) + (set_index << (SET_INDEX_POS - 2));
        setHeads[set_index] = currentElement;

        for (line_index = 0; line_index < ways; line_index++)
        {
            for (element_index = 0; element_index < sets / SETS_PER_PAGE - 1; element_index++)
            {
                nextElement = (line_index << allSetOffset) + (shuffledArray[element_index + 1] << 10) + (set_index << (SET_INDEX_POS - 2));
                evictionArray[currentElement] = nextElement;
                currentElement = nextElement;
            }
            if (line_index == ways - 1)
            {
                nextElement = setHeads[set_index];
            }
            else
            {
                nextElement = ((line_index + 1) << allSetOffset) + (shuffledArray[0] << 10) + (set_index << (SET_INDEX_POS - 2));
            }
            evictionArray[currentElement] = nextElement;
            currentElement = nextElement;
        }
    }
    setHeadsCreated = true;
}

void probeAllSets()
{
    if (!setHeadsCreated)
        createSetHeads();
    for (uint32_t set = 0; set < SETS_PER_PAGE; set += SET_SKIPPING_STEP)
    {
        uint32_t pointer = setHeads[set];
        uint32_t listHead = pointer;
        do
        {
            pointer = evictionArray[pointer];
        } while (pointer != listHead);
    }
}

uint64_t measureOnce()
{
    if (!setHeadsCreated)
        createSetHeads();

    uint64_t currentTime = perfNow();

    _mm_mfence();

    probeAllSets();

    _mm_mfence();
    uint64_t result = perfNow() - currentTime;

    return result;
}
