# iGPU Graphical Data Compression in Chrome

This folder contains two Chrome PoCs that do cross-origin pixel stealing via the iGPU graphical data compression side-channel.

`chrome-pp` relies on direct rendering time to infer cross-origin pixel. It is also hosted at <https://www.cs.utexas.edu/~yingchen/chrome-pp/chrome.html>.

`chrome-cache` relies on LLC walk time to infer cross-origin pixel. It is also hosted at <https://www.cs.utexas.edu/~yingchen/chrome-cache/chrome.html>.

## To set up the server

```bash
python3 -m http.server
```

## PoC details

The PoCs embed a cross-origin checkerboard as an iframe.
It isolates and expands a single cross-origin pixel.
Then, it cooks up an SVG filter such that if the cross-origin pixel is black, all intermediate surfaces created by this filter is Black (compressible), and if the cross-origin pixel is white, all intermediate surfaces created by this filter is turbulence (non-compressible) (See Algorithm 2 in the paper).
It renders this SVG filter and infer the binarized cross-origin pixel by the direct rendering time of this filter (`chrome-pp`), or the LLC walk time after this filter renders (`chrome-cache`).

## How to run PoC `chrome-pp`

First, set up the server and visit <http://0.0.0.0:8000/chrome-pp/chrome.html>
The parameters are:

1. `Time`: The time that the SVG filter renders when targeting a single pixel. The smaller the value, the higher the PoC throughput, but also the lower the reconstruction accuracy.
2. `repetition`: The parameter for finding appropriate threshold of the direct rendering time when targeting a black vs white cross-origin pixel. The amount of time spend on finding threshold = `Time` *`repetition`* 2. The higher the repetition, the more accurate the threshold.
3. `stress`: If the PoC is not enough to saturate the iGPU memory subsystem, extra memory contention is needed to trigger rendering time difference between black and white (See `03-llc`). Set `stress` = 1 to add extra memory contention.
4. `warmup time`: For warming up the iGPU.
5. `Div Size`: The width of all intermediate surfaces.
6. `Layer`: The number of layer of the SVG filter.
7. `threshold low/high`: Suppose `BR` denotes the direct rendering time of a cross-origin black pixel and `BW` denotes the direct rendering time of a cross-origin white pixel (determined in thresholding).
    1. If measured rendering time is smaller than (`BR` + (`BW`-`BR`)* `threshold low`), we conclude the target pixel is black.
    2. If measured rendering time is larger than (`BR` + (`BW`-`BR`)* `threshold high`), we conclude the target pixel is white.
    3. Otherwise, we redo the attack.
8. `number of workers`: If `stress` = 1, the PoC will launch `number of workers` memory stressor each doing bignum computations of `BigInt digits`.
9. `Test mode`: If Test mode is clicked, the PoC will only do the threshold finding phase but not the real attack, and display the direct rendering time of targeting a black vs white cross-origin pixel on top of the screen.

Hit Run.

## How to run PoC `chrome-cache`

First, set up the server and visit <http://0.0.0.0:8000/chrome-cache/chrome.html>

Parameters follow the same logic as above. Except we rely on LLC walk time rather than rendering time.

## Note

1. For machines with a powerful memory controller, we recommend to set `stress` = 1 when running the `chrome-pp` PoC.
2. Although we use the checkerboard as our cross-origin target, one can switch the target by modifying the `frameinner` src in `chrome-embed-zoom-scroll.html`.

## How to reproduce Figure 14 in the paper

1. Collecting *DRAM traffic per frame* of Chrome rendering `chrome-pp` PoC requires one to synchronize frame updates in the browser and hardware measurements. We do so by attaching a uprobe on the `Math.sqrt` function in V8 to expose the CPU time-stamp counter to Chrome. See `../uprobe-chromium` for how to set up the uprobe.
2. Call `Math.sqrt()` in front of `requestAnimationFrame` in `foreverRender()` function of `chrome-pp/chrome.js`.
3. Use a script similar to the one in `01-leakage-channel/scripts/exp2` to monitor *DRAM traffic per frame*, and *DRAM bandwidth* of Chrome rendering.
4. Collect CPU time-stamps reported by `Math.sqrt()` to get *Rendering time per frame*.
5. Use a ploting script similar to the one in `01-leakage-channel/scripts/exp2` to produce Figure 14.

## How to reproduce Figure 15 in the paper

1. Run the `chrome-cache` PoC, and collect *LLC walk time* (print `currentTimeArray` after Line 196 in `chrome-cache/chrome.js`).
2. Use a script similar to the one in `../03-llc` to produce Figure 15.
