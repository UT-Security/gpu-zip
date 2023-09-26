// Mostly taken from https://codepen.io/atoliks24/pen/GRRPzQm

const log = s => { };
// const log = console.log;
const SAMPLING_PERIOD_IN_MS = 16;
const SET_SKIPPING_STEP = 1;

/* machine-specific parameters START for i7-8700*/
const CACHE_SIZE = 12 * 1024 * 1024;
const LINE_SIZE = 64;
const CACHE_LINES = CACHE_SIZE / LINE_SIZE;
const CACHE_ASSOC = 16; // check by getconf -a | grep CACHE
const SET_INDEX_POS = Math.log2(LINE_SIZE);
const CACHE_SETS = CACHE_LINES / CACHE_ASSOC;

const SETS_PER_PAGE = 4096 / LINE_SIZE;
/* machine-specific parameters END */

const BYTES_PER_MB = 1024 * 1024;

const COUNT_SWEEPS = false; // if false, measure time. if true, measure how many cache sweeps we get until the clock changes

// Prime and probe object
function PrimeProbe(sets, ways) {
    // length of evictionArray needs to be larger than CACHE_ASSOC * 2^allSetOffset
    this.evictionArray = new Uint32Array(64 * BYTES_PER_MB / Uint32Array.BYTES_PER_ELEMENT); // array that contains offsets into this same array, 3145728
    this.setHeads = new Array(SETS_PER_PAGE);

    this.probeSet = function (setIndex) {
        // var elementsWentOver = 0;
        // Go over all elements in the set
        let pointer = this.setHeads[setIndex];
        let listHead = pointer;
        do {
            // elementsWentOver++;
            pointer = this.evictionArray[pointer];
        } while (pointer != listHead);

        // console.log("Went over " + elementsWentOver + " elements.");
        return pointer;
    }

    this.probeSetLimited = function (setOffset, hops) {
        //var elementsWentOver = 0;
        // Go over all elements in the set
        let pointer = this.setHeads[setOffset];
        let listHead = pointer;
        do {
            // elementsWentOver++;
            pointer = this.evictionArray[pointer];
            hops--;
        } while ((hops != 0) && (pointer != listHead));

        // console.log("Went over " + elementsWentOver + " elements.");
        return pointer;
    }

    this.probeSets = function (sets) {
        // Probe some of the sets in the page
        for (setOffset in sets) {
            this.probeSet(sets[setOffset]);
        }
    }

    // the default result array contains how much time each of this function call takes
    this.probeAllSets = function (worker_id, num_workers) {
        for (let set = (SETS_PER_PAGE / num_workers) * worker_id; set < (SETS_PER_PAGE / num_workers) * (worker_id + 1); set += SET_SKIPPING_STEP) { // 1.1 skip one set when choosing the next set -- zg: it sounds like we don't want to skip
            this.probeSet(set);
        }
    }

    this.shuffle = function (arrayToShuffle) {
        var tmp, current, top = arrayToShuffle.length;
        if (top) while (--top) {
            current = Math.floor(Math.random() * (top + 1));
            tmp = arrayToShuffle[current];
            arrayToShuffle[current] = arrayToShuffle[top];
            arrayToShuffle[top] = tmp;
        }
        return arrayToShuffle;
    }

    // |     4 | 14(8|6) | 4
    this.createSetHeads = function (sets, ways) { // 12288, 16
        // We have 4 set heads
        var unshuffledArray = new Uint32Array(sets / SETS_PER_PAGE); // for each page, store an index of its page index 
        var allSetOffset = Math.ceil(Math.log2(sets)) + 4; // 18

        let i;
        for (i = 0; i < unshuffledArray.length; i++) {
            unshuffledArray[i] = i;
        }

        // Shuffle the array
        var shuffledArray = this.shuffle(unshuffledArray); // 2. shuffle the page index array

        // zg: this comment block is dated
        // Now write into the eviction buffer
        // virtual address is:
        // LLL LEEE EEEE SSSS SS00 00[00] (last 2 bits are because of UINT32 vs BYTE)
        //               ^^^^ ^^ - these 6 bits determine the set index, 64 possible values
        //               ^^^^ ^^^^ ^^ ^^ - these 12 bits (4K) are the same in physical and in virtual
        //      ^^^ ^^^^ we keep the set and change these 6/7 bits to 64/128 different values to cover all 8192=128*64 sets
        // ^^^ ^  - we use 12/16 different values for this to touch each set 12/16 times, once per line

        // zg: for a given set_index, we first only change the page_index (randomly) bits to walk through all pages. Then, switch the more significant line_index to next line, walk through all pages again.
        // Until we walk through all lines in all pages for the given set_index, we move on to the next set_index
        let set_index, element_index, line_index;
        let currentElement, nextElement;
        for (set_index = 0; set_index < SETS_PER_PAGE; set_index++) {
            currentElement = (shuffledArray[0] << 10) + (set_index << (SET_INDEX_POS - 2)); // the virtual address pointer (modulo 4 bytes so an index to uint32array) to the set starting locations on a random page
            this.setHeads[set_index] = currentElement;

            for (line_index = 0; line_index < ways; line_index++) {
                //currentElement = (line_index << 17) + (shuffledArray[0] << 10) + (set_index << 4);
                for (element_index = 0; element_index < sets / SETS_PER_PAGE - 1; element_index++) {
                    // the next element is the set starting location + a line starting location on the next random page
                    // after all pages are walked, go to the next line of the same set_index back from the first random page 
                    nextElement = (line_index << allSetOffset) + (shuffledArray[element_index + 1] << 10) + (set_index << (SET_INDEX_POS - 2));
                    this.evictionArray[currentElement] = nextElement;
                    currentElement = nextElement;
                } // element
                if (line_index == ways - 1) {
                    // In the last line, the last pointer goes to the head of the entire set
                    nextElement = this.setHeads[set_index];
                } else {
                    // Last pointer goes back to the head of the next line
                    nextElement = ((line_index + 1) << allSetOffset) + (shuffledArray[0] << 10) + (set_index << (SET_INDEX_POS - 2));
                }
                this.evictionArray[currentElement] = nextElement;
                currentElement = nextElement;
            } // line



        } // set

    };

    this.createSetHeads(sets, ways);

    // check that this doesn't crash/get stuck
    // this line can get stuck if bad machine-specific parameters
    // this.probeSets([0]);
    this.probeSet(0);
} // PP object.

PP = new PrimeProbe(CACHE_SETS, CACHE_ASSOC);

function performMeasurement(time_collect_ms, worker_id, num_workers) {
    var resultArray = new Array(Math.ceil(time_collect_ms / SAMPLING_PERIOD_IN_MS));

    // For each measurement period
    var measurement_index, busySpins = 0;
    var nextMeasurementStartTime = performance.now(), currentTime;

    nextMeasurementStartTime += SAMPLING_PERIOD_IN_MS;

    // Spin until we're ready for the next measurement
    do {
        currentTime = performance.now();
    }
    while (currentTime < nextMeasurementStartTime);

    // 1. the main loop
    for (measurement_index = 0; measurement_index < resultArray.length; measurement_index++) {

        nextMeasurementStartTime += SAMPLING_PERIOD_IN_MS;
        currentTime = performance.now();

        // if we've run out of time, skip the measurement
        if (currentTime >= nextMeasurementStartTime) {
            if (COUNT_SWEEPS == true) {
                resultArray[measurement_index] = 0;
            } else {
                resultArray[measurement_index] = SAMPLING_PERIOD_IN_MS * 3; // Some arbitrarily high value...
            }
        } else {
            if (COUNT_SWEEPS == true) {
                var sweeps = 0;
                // repeatedly perform the measurement until the clock changes
                do {
                    currentTime = performance.now();
                    sweeps++;
                    PP.probeAllSets(worker_id, num_workers);
                } while (currentTime < nextMeasurementStartTime);
            }
            else {
                // otherwise, perform the measurement
                PP.probeAllSets(worker_id, num_workers);
            }
            // log how many spins it took until the clock ticked again
            if (COUNT_SWEEPS == true) {
                // log how many spins it took until the clock ticked again
                resultArray[measurement_index] = sweeps;
            } else {
                resultArray[measurement_index] = performance.now() - currentTime;
            }
            // Prepare for the next measurement
            do {
                currentTime = performance.now();
            }
            while (currentTime < nextMeasurementStartTime);
        }
    }

    postMessage(resultArray);
}

function measureOnce() {
    let result;

    // we'd better not use performance.now() due to its memory-inefficient implementation
    let currentTime = performance.now();
    PP.probeAllSets(0, 1);

    result = performance.now() - currentTime;

    postMessage(result);
}

onmessage = e => {
    // a directly inlined version of measureOnce()
    // console.log(PP.evictionArray[PP.setHeads[0]]);
    let result;

    // we'd better not use performance.now() due to its memory-inefficient implementation
    let currentTime = performance.now();

    // a directly inlined version of probeAllSets()
    for (let set = 0; set < SETS_PER_PAGE; set += SET_SKIPPING_STEP) { // 1.1 skip one set when choosing the next set -- zg: it sounds like we don't want to skip
        PP.probeSet(set);
    }

    result = performance.now() - currentTime;

    postMessage(result);
}



