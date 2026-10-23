#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Source-extracted canvas rectangle correctness and optional x86 host throughput.

Run: python3 tests/host/gfx_rect_perf_test.py --source firmware/src/gfx.c
     python3 tests/host/gfx_rect_perf_test.py --source firmware/src/gfx.c --bench
Benchmarks are native HOST PROXIES, never LCD DMA/device/audio measurements.
"""
import argparse
import pathlib
import subprocess
import tempfile


def extract(source):
    marker = "static void cv_rect("
    at = source.index(marker)
    opening = source.index("{", at)
    nesting = 0
    for i in range(opening, len(source)):
        if source[i] == "{":
            nesting += 1
        elif source[i] == "}":
            nesting -= 1
            if nesting == 0:
                return source[at : i + 1]
    raise ValueError("Unterminated cv_rect function")


HEADER = r'''
#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define CV_MAX (240u * 144u)
static uint16_t cv_px[CV_MAX];
static uint16_t expect_px[CV_MAX];
static uint32_t cv_w = 240u, cv_h = 144u;
static int32_t cv_oy, cv_y0, cv_y1;
static inline uint16_t swap16(uint32_t c) {
    return (uint16_t)(((c >> 8) & 0xFFu) | ((c & 0xFFu) << 8));
}
'''

TAIL = r'''
static uint32_t rndstate = 0xC0FFEE37u;
static uint32_t rnd(void) {
    rndstate ^= rndstate << 13;
    rndstate ^= rndstate >> 17;
    rndstate ^= rndstate << 5;
    return rndstate;
}
typedef struct { int32_t x,y,w,h; uint16_t c; } rect_op;
static rect_op ops[512];
static volatile uint64_t sink;
static double ms(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec * 1000.0 + (double)t.tv_nsec / 1000000.0;
}
static void init_canvas(unsigned band, uint32_t width) {
    cv_w = width;
    cv_oy = cv_y0 = band ? 72 : 0;
    cv_y1 = (int32_t)cv_h;
    for (unsigned i=0; i<CV_MAX; i++)
        cv_px[i] = expect_px[i] = (uint16_t)((i * 67u) ^ (i >> 7));
}
static void oracle(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c) {
    int32_t start = y + cv_oy, end = start + h;
    uint16_t sc = swap16(c);
    for (int32_t yy=cv_y0; yy<cv_y1; yy++)
        for (int32_t xx=0; xx<(int32_t)cv_w; xx++)
            if (xx >= x && xx < x+w && yy >= start && yy < end)
                expect_px[(uint32_t)yy * cv_w + (uint32_t)xx] = sc;
}
static void setup_ops(unsigned shape) {
    for (unsigned i=0; i<512u; i++) {
        rect_op *o=&ops[i];
        if (shape == 0u) {
            o->x=(int32_t)(rnd()%280u)-20;
            o->y=(int32_t)(rnd()%180u)-20;
            o->w=(int32_t)(rnd()%260u)-4;
            o->h=(int32_t)(rnd()%130u)-4;
        } else if (shape == 1u) {
            o->x=(int32_t)(rnd()%260u)-10;
            o->y=(int32_t)(rnd()%165u)-10;
            o->w=(int32_t)(rnd()%24u)-2;
            o->h=(int32_t)(rnd()%15u)-2;
        } else {
            o->x=0; o->y=0; o->w=240; o->h=144;
        }
        o->c=(uint16_t)rnd();
    }
}
static int check(void) {
    unsigned checked=0;
    const uint32_t widths[] = {1u, 64u, 240u};
    for (unsigned wi=0; wi<3u; wi++)
    for (unsigned band=0; band<2u; band++) {
        for (unsigned shape=0; shape<3u; shape++) {
            init_canvas(band, widths[wi]);
            setup_ops(shape);
            for (unsigned i=0; i<512u; i++) {
                rect_op o=ops[i];
                cv_rect(o.x,o.y,o.w,o.h,o.c);
                oracle(o.x,o.y,o.w,o.h,o.c);
                if (memcmp(cv_px,expect_px,sizeof cv_px)) {
                    fprintf(stderr,"FAIL band=%u shape=%u case=%u [%d,%d,%d,%d]\n",
                            band,shape,i,o.x,o.y,o.w,o.h);
                    return 1;
                }
                checked++;
            }
        }
    }
    printf("PASS: %u exact-source rectangles, oracle pixel-equivalent; full and band canvases\n",checked);
    return 0;
}
static double speed(unsigned shape) {
    unsigned rounds = shape == 2u ? 80u : 650u;
    init_canvas(0, 240u);
    double start=ms();
    for (unsigned k=0; k<rounds; k++)
        for (unsigned j=0; j<512u; j++) {
            rect_op o=ops[j];
            cv_rect(o.x,o.y,o.w,o.h,o.c);
        }
    double elapsed=ms()-start;
    uint64_t check=0;
    for (unsigned i=0; i<CV_MAX; i+=53u) check+=cv_px[i];
    sink=check;
    return elapsed * 1000000.0 / ((double)rounds*512u);
}
int main(int argc, char **argv) {
    if (check()) return 1;
    if (argc < 2 || strcmp(argv[1],"--bench")) return 0;
    for (unsigned shape=0; shape<3u; shape++) {
        double times[5];
        setup_ops(shape);
        for (unsigned i=0; i<5u; i++) times[i]=speed(shape);
        for (unsigned i=0; i<5u; i++)
            for (unsigned j=i+1; j<5u; j++)
                if (times[i] > times[j]) {double t=times[i];times[i]=times[j];times[j]=t;}
        printf("HOST PROXY %s median_ns_per_rect=%.3f\n",
            shape==0u?"mixed":shape==1u?"small":"full",times[2]);
    }
    return 0;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", default="firmware/src/gfx.c", type=pathlib.Path)
    parser.add_argument("--compiler", default="gcc", choices=("gcc", "clang"))
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--bench", action="store_true")
    opts = parser.parse_args()
    if opts.sanitize and opts.bench:
        parser.error("--bench with --sanitize would measure sanitizer overhead")
    function = extract(opts.source.read_text(encoding="utf-8"))
    with tempfile.TemporaryDirectory(prefix="x0x-gfx-rect-") as td:
        src = pathlib.Path(td) / "source.c"
        exe = pathlib.Path(td) / "test"
        src.write_text(HEADER + "\n" + function + "\n" + TAIL, encoding="utf-8")
        cmd = [opts.compiler, "-std=c11", "-O1" if opts.sanitize else "-O2",
               "-Wall", "-Wextra", "-Werror"]
        if opts.sanitize:
            cmd += ["-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                    "-fno-omit-frame-pointer"]
        cmd += [str(src), "-o", str(exe)]
        print("COMPILE:", " ".join(cmd), flush=True)
        subprocess.run(cmd, check=True)
        subprocess.run([str(exe)] + (["--bench"] if opts.bench else []), check=True)
    print("COMPILE_EXIT=0 RUN_EXIT=0", flush=True)


if __name__ == "__main__":
    main()
