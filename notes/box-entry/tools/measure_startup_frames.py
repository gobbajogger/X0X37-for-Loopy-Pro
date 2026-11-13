#!/usr/bin/env python3
"""Host measurement of 240x240 RGB565 encodings for the short box-entry clip.
CPU-light: 36 frames, numpy, no device, no JieLi build.
"""
from __future__ import annotations

import hashlib
import json
import struct
import sys
import time
import zlib
from collections import Counter
from pathlib import Path

import numpy as np
from PIL import Image

FRAMES_DIR = Path("/workspace/box-entry/work/short-frames")
HOME240 = Path("/workspace/box-entry/work/HOME-94197fc8-240.png")
OUT_JSON = Path("/workspace/box-entry/check/frame-budget.json")
OUT_TXT = Path("/workspace/box-entry/check/frame-budget.txt")
W = H = 240
RAW = W * H * 2  # 115200
SPI_HZ = 12_000_000  # SMK LCD_BAUD 4 = 12 MHz (lcd.c)


def rgb888_to_565(rgb: np.ndarray) -> np.ndarray:
    r = (rgb[..., 0].astype(np.uint16) >> 3) << 11
    g = (rgb[..., 1].astype(np.uint16) >> 2) << 5
    b = rgb[..., 2].astype(np.uint16) >> 3
    return (r | g | b).astype(np.uint16)


def to_be_bytes(px: np.ndarray) -> bytes:
    """Device stores RGB565 byte-swapped (panel is big-endian)."""
    return px.astype(">u2").tobytes()


def load_frames():
    paths = [FRAMES_DIR / f"{i:04d}.png" for i in range(1, 37)]
    for p in paths:
        if not p.exists():
            raise SystemExit(f"missing {p}")
    frames888 = []
    sha = []
    src_wh = []
    for p in paths:
        im = Image.open(p).convert("RGB")
        src_wh.append(im.size)
        raw = np.asarray(im, dtype=np.uint16)
        if raw.shape[0] != 480 or raw.shape[1] != 480:
            raise SystemExit(f"{p} is {im.size}, expected 480x480")
        # 2x2 area average, then RGB565 (device format)
        avg = (raw[0::2, 0::2] + raw[0::2, 1::2] + raw[1::2, 0::2] + raw[1::2, 1::2] + 2) // 4
        avg = np.clip(avg, 0, 255).astype(np.uint8)
        frames888.append(avg)
        sha.append(hashlib.sha256(p.read_bytes()).hexdigest())
    home = np.asarray(Image.open(HOME240).convert("RGB"), dtype=np.uint8)
    return frames888, sha, home


def unique_colors(px565: np.ndarray) -> int:
    return int(np.unique(px565).size)


# ---- RLE: 16-bit pixels, control byte: 0..127 = (c+1) repeats of next pixel (2 B);
# 128..255 = (c-127) literal pixels follow. ----
def rle_pixels(px: np.ndarray) -> bytes:
    flat = px.ravel()
    n = flat.size
    out = bytearray()
    i = 0
    while i < n:
        # literal run until a repeat of >= 3, or 127 literals
        lit_start = i
        while i < n:
            run = 1
            while i + run < n and flat[i + run] == flat[i] and run < 128:
                run += 1
            if run >= 3 and (i - lit_start) > 0:
                break
            if run >= 3 and (i - lit_start) == 0:
                # start with a repeat
                out.append(run - 1)
                out += struct.pack(">H", int(flat[i]))
                i += run
                lit_start = i
                continue
            i += 1
            if i - lit_start >= 127:
                break
        lit = i - lit_start
        if lit:
            # if the leftover is a short run at the end of the literal window, just emit literals
            out.append(0x80 + lit - 1)  # 128..254 => 1..127 literals? 0x80|(lit-1) = 128+lit-1
            # Wait: 128..255 = (c-127) literals: c=128 => 1, c=254 => 127. So c = 127+lit
            out[-1] = 127 + lit
            out += b"".join(struct.pack(">H", int(v)) for v in flat[lit_start:i])
    return bytes(out)


def rle_pixels_v2(px: np.ndarray) -> bytes:
    """Cleaner pixel RLE as specified above."""
    flat = np.asarray(px, dtype=np.uint16).ravel()
    n = int(flat.size)
    out = bytearray()
    i = 0
    lit = []

    def flush_lit():
        nonlocal lit, out
        k = 0
        while k < len(lit):
            chunk = lit[k : k + 128]
            out.append(127 + len(chunk))  # 128..255 => 1..128 literals
            for v in chunk:
                out += struct.pack(">H", int(v))
            k += len(chunk)
        lit = []

    while i < n:
        run = 1
        v = int(flat[i])
        while i + run < n and int(flat[i + run]) == v and run < 128:
            run += 1
        if run >= 3:
            flush_lit()
            out.append(run - 1)  # 0..127 => 1..128 repeats
            out += struct.pack(">H", v)
            i += run
        else:
            for _ in range(run):
                lit.append(int(flat[i]))
                i += 1
                if len(lit) == 128:
                    flush_lit()
    flush_lit()
    return bytes(out)


def rle_bytes(data: bytes) -> bytes:
    """Byte RLE: same control, 1-byte values."""
    n = len(data)
    out = bytearray()
    i = 0
    lit = bytearray()

    def flush_lit():
        nonlocal lit, out
        k = 0
        while k < len(lit):
            chunk = lit[k : k + 128]
            out.append(127 + len(chunk))
            out += chunk
            k += len(chunk)
        lit = bytearray()

    while i < n:
        run = 1
        v = data[i]
        while i + run < n and data[i + run] == v and run < 128:
            run += 1
        if run >= 3:
            flush_lit()
            out.append(run - 1)
            out.append(v)
            i += run
        else:
            for _ in range(run):
                lit.append(data[i])
                i += 1
                if len(lit) == 128:
                    flush_lit()
    flush_lit()
    return bytes(out)


# ---- Firmware LZ77 (lz.c), exact algorithm ----
LZ_MIN, LZ_MAXLEN, LZ_MAXLIT, LZ_HBITS, LZ_NONE = 3, 0x7F + 3, 128, 11, 0xFFFF


def lz_hash(p: bytes, i: int) -> int:
    v = (p[i] << 16 | p[i + 1] << 8 | p[i + 2]) * 2654435761
    return (v & 0xFFFFFFFF) >> (32 - LZ_HBITS)


def lz_match_len(inp: bytes, n: int, cand: int, i: int) -> int:
    length = 0
    while length < LZ_MAXLEN and i + length < n and inp[cand + length] == inp[i + length]:
        length += 1
    return length


def lz_compress_one(inp: bytes) -> bytes:
    n = len(inp)
    if n >= LZ_NONE:
        raise ValueError("too big")
    head = [LZ_NONE] * (1 << LZ_HBITS)
    out = bytearray()
    i = 0
    lit = 0

    def put_lits(p, nlit):
        while nlit:
            k = nlit if nlit <= LZ_MAXLIT else LZ_MAXLIT
            out.append(k - 1)
            out.extend(p[:k])
            p = p[k:]
            nlit -= k

    while i < n:
        best, dist = 0, 0
        if i + LZ_MIN <= n:
            h = lz_hash(inp, i)
            c = head[h]
            head[h] = i
            if c != LZ_NONE:
                length = lz_match_len(inp, n, c, i)
                if length > best:
                    best, dist = length, i - c
            if best < LZ_MAXLEN and i:
                length = lz_match_len(inp, n, i - 1, i)
                if length > best:
                    best, dist = length, 1
        if best < LZ_MIN:
            i += 1
            continue
        put_lits(inp[lit:i], i - lit)
        out.append(0x80 | (best - LZ_MIN))
        d = dist - 1
        out.append(d & 0xFF)
        out.append((d >> 8) & 0xFF)
        k = 1
        while k < best and i + k + LZ_MIN <= n:
            head[lz_hash(inp, i + k)] = i + k
            k += 1
        i += best
        lit = i
    put_lits(inp[lit:i], i - lit)
    return bytes(out)


def lz_compress(inp: bytes) -> bytes:
    """Firmware LZ rejects inputs >= 64 KiB (u16 distances). Chunk into 120-row
    RGB565 bands (57,600 B) or arbitrary 32 KiB chunks, prefixed by u16 length each.
    The prefix is included in the reported size."""
    if len(inp) < LZ_NONE:
        return lz_compress_one(inp)
    # Prefer 240*120*2 = 57600 if divisible, else 32 KiB
    cap = 240 * 120 * 2 if len(inp) % (240 * 120 * 2) == 0 or len(inp) == 240 * 240 * 2 else 32768
    out = bytearray()
    i = 0
    while i < len(inp):
        chunk = inp[i:i+cap]
        c = lz_compress_one(chunk)
        out += len(c).to_bytes(2, "big")
        out += c
        i += cap
    return bytes(out)


def dirty_bbox(a: np.ndarray, b: np.ndarray):
    d = a != b
    if not d.any():
        return 0, 0, 0, 0, 0
    rows = np.any(d, axis=1)
    cols = np.any(d, axis=0)
    y0, y1 = int(np.argmax(rows)), int(H - np.argmax(rows[::-1]))
    x0, x1 = int(np.argmax(cols)), int(W - np.argmax(cols[::-1]))
    return x0, y0, x1 - x0, y1 - y0, int(d.sum())


def dirty_span_encode(prev: np.ndarray, cur: np.ndarray) -> bytes:
    """Per-row changed spans: for each row, u8 nspans, then (u8 x, u8 len, pixels...).
    len 0..240; if a row is empty, nspans=0. 240-wide span uses len=0 as 240? Use u16 len.
    Format: 240 rows × [u8 n] + n × [u8 x][u8 npx][npx × u16 BE]; npx 1..240, npx=0 means 240 if x==0.
    Simpler: [u16 skip_pixels][u16 copy_pixels][pixels] repeating, 0,0 ends frame (after 57600 consumed).
    """
    pf, cf = prev.ravel(), cur.ravel()
    n = pf.size
    out = bytearray()
    i = 0
    while i < n:
        skip = 0
        while i + skip < n and pf[i + skip] == cf[i + skip] and skip < 65535:
            skip += 1
        i += skip
        copy = 0
        while i + copy < n and pf[i + copy] != cf[i + copy] and copy < 65535:
            copy += 1
        out += struct.pack(">HH", skip, copy)
        if copy:
            out += to_be_bytes(cf[i : i + copy])
        i += copy
    return bytes(out)


def dirty_span_rle(prev: np.ndarray, cur: np.ndarray) -> bytes:
    """Same skip/copy but copy payload is pixel-RLE."""
    pf, cf = prev.ravel(), cur.ravel()
    n = pf.size
    out = bytearray()
    i = 0
    while i < n:
        skip = 0
        while i + skip < n and pf[i + skip] == cf[i + skip] and skip < 65535:
            skip += 1
        i += skip
        copy = 0
        while i + copy < n and pf[i + copy] != cf[i + copy] and copy < 65535:
            copy += 1
        payload = rle_pixels_v2(cf[i : i + copy]) if copy else b""
        out += struct.pack(">HH", skip, copy)  # copy is decoded pixel count
        out += struct.pack(">H", len(payload))
        out += payload
        i += copy
    return bytes(out)


def xor_rle(prev: np.ndarray, cur: np.ndarray) -> bytes:
    x = np.bitwise_xor(prev, cur)
    return rle_pixels_v2(x)


def median_cut(px: np.ndarray, ncolors: int) -> tuple[np.ndarray, np.ndarray]:
    """px: (H,W) uint16 RGB565 -> (palette uint16[n], indices uint8)."""
    flat = px.ravel()
    uniq, inv = np.unique(flat, return_inverse=True)
    if uniq.size <= ncolors:
        pal = np.zeros(ncolors, dtype=np.uint16)
        pal[: uniq.size] = uniq
        # remap inv is already 0..uniq-1
        return pal, inv.astype(np.uint8).reshape(px.shape)
    # extract RGB
    def chans(u):
        r = ((u >> 11) & 31).astype(np.int32)
        g = ((u >> 5) & 63).astype(np.int32)
        b = (u & 31).astype(np.int32)
        return np.stack([r, g, b], axis=-1)

    boxes = [np.arange(uniq.size)]
    rgb = chans(uniq)
    while len(boxes) < ncolors:
        # split the box with largest range
        ranges = []
        for b in boxes:
            if b.size <= 1:
                ranges.append(-1)
            else:
                rr = rgb[b].max(0) - rgb[b].min(0)
                ranges.append(int(rr.max()))
        j = int(np.argmax(ranges))
        if ranges[j] < 0:
            break
        b = boxes[j]
        axis = int(np.argmax(rgb[b].max(0) - rgb[b].min(0)))
        order = b[np.argsort(rgb[b][:, axis])]
        mid = order.size // 2
        boxes[j] = order[:mid]
        boxes.append(order[mid:])
    pal = np.zeros(ncolors, dtype=np.uint16)
    # map uniq index -> pal
    u2p = np.zeros(uniq.size, dtype=np.uint8)
    for pi, b in enumerate(boxes):
        # average color
        avg = rgb[b].mean(0)
        r = int(np.clip(round(avg[0]), 0, 31))
        g = int(np.clip(round(avg[1]), 0, 63))
        bl = int(np.clip(round(avg[2]), 0, 31))
        pal[pi] = (r << 11) | (g << 5) | bl
        u2p[b] = pi
    idx = u2p[inv].astype(np.uint8).reshape(px.shape)
    return pal, idx


def index_rle(idx: np.ndarray) -> bytes:
    """RLE of 8-bit indices, same control codes, 1-byte values."""
    return rle_bytes(idx.tobytes())


def psnr565(orig: np.ndarray, recon: np.ndarray) -> float:
    # expand to 8-bit approx
    def to8(p):
        r = ((p >> 11) & 31) * 255 // 31
        g = ((p >> 5) & 63) * 255 // 63
        b = (p & 31) * 255 // 31
        return np.stack([r, g, b], axis=-1).astype(np.float64)

    a, b = to8(orig), to8(recon)
    mse = np.mean((a - b) ** 2)
    if mse == 0:
        return 99.0
    return float(10 * np.log10(255.0**2 / mse))


def spi_ms(nbytes: int) -> float:
    return nbytes * 8 / SPI_HZ * 1000.0


def main():
    t0 = time.time()
    frames888, src_sha, home888 = load_frames()
    frames = [rgb888_to_565(f) for f in frames888]
    home565 = rgb888_to_565(home888)

    # Frame 36 vs exact HOME-240 after downscale
    f36 = frames[35]
    n_diff_home = int(np.sum(f36 != home565))
    # area-average of exact 2x HOME should match; report anyway
    ident = []
    groups = []
    i = 0
    while i < 36:
        j = i + 1
        while j < 36 and np.array_equal(frames[j], frames[i]):
            j += 1
        groups.append((i + 1, j, j - i))  # 1-based inclusive? frames i..j-1
        i = j

    per = []
    zero = np.zeros_like(frames[0])
    for i, px in enumerate(frames):
        prev = frames[i - 1] if i else zero
        x0, y0, bw, bh, ndirty = dirty_bbox(prev, px) if i else (0, 0, W, H, W * H)
        raw = to_be_bytes(px)
        xor = to_be_bytes(np.bitwise_xor(prev, px)) if i else raw
        rec = {
            "i": i + 1,
            "src_sha256": src_sha[i],
            "unique_rgb565": unique_colors(px),
            "raw": RAW,
            "rle_px": len(rle_pixels_v2(px)),
            "rle_bytes": len(rle_bytes(raw)),
            "lz": len(lz_compress(raw)),
            "zlib": len(zlib.compress(raw, 9)),
            "dirty_px": ndirty if i else W * H,
            "bbox": [x0, y0, bw, bh],
            "bbox_raw": (bw * bh * 2) if i else RAW,
            "xor_rle_px": len(xor_rle(prev, px)) if i else len(rle_pixels_v2(px)),
            "xor_lz": len(lz_compress(xor)) if i else len(lz_compress(raw)),
            "xor_zlib": len(zlib.compress(xor, 9)) if i else len(zlib.compress(raw, 9)),
            "dirty_span": len(dirty_span_encode(prev, px)) if i else RAW,
            "dirty_span_rle": len(dirty_span_rle(prev, px)) if i else len(rle_pixels_v2(px)),
            "identical_to_prev": bool(i and ndirty == 0),
        }
        # bbox+RLE of the bbox region only (plus 4-byte header x,y,w,h)
        if i and ndirty:
            sub = px[y0 : y0 + bh, x0 : x0 + bw]
            rec["bbox_rle"] = 8 + len(rle_pixels_v2(sub))
            rec["bbox_lz"] = 8 + len(lz_compress(to_be_bytes(sub)))
        elif i:
            rec["bbox_rle"] = 2  # empty marker
            rec["bbox_lz"] = 2
        else:
            rec["bbox_rle"] = len(rle_pixels_v2(px))
            rec["bbox_lz"] = len(lz_compress(raw))
        rec["spi_full_ms"] = round(spi_ms(RAW), 2)
        rec["spi_dirty_bbox_ms"] = round(spi_ms(rec["bbox_raw"] if i else RAW), 2)
        rec["spi_dirty_px_ms"] = round(spi_ms((ndirty if i else W * H) * 2), 2)
        per.append(rec)

    def sum_field(field, indices):
        return int(sum(per[i][field] for i in indices))

    all_i = list(range(36))
    no_last = list(range(35))  # 1..35 stored, 36 is live UI
    # 18 frames at 35 fps: 1,3,5,...,35 (odd), drop 36; or 1,3,...,33 + 35
    half = list(range(0, 35, 2))  # indices 0,2,...,34 = frames 1,3,...,35 (18 frames)
    # unique-hold: first of each identical group, excluding frame 36 if it's the last of a group
    uniq_idx = []
    for start0, end1based, cnt in groups:
        a = start0 - 1
        b = end1based  # exclusive 0-based? groups stored (i+1, j, j-i) with frames i..j-1
        # start0 is 1-based i+1, end is j (exclusive 1-based? I used (i+1, j, j-i) where j is exclusive 0-based+? )
        # i was 0-based, groups.append((i+1, j, j-i)) with j exclusive 0-based.
        # frames 0-based [start0-1, j)
        j = end1based
        s = start0 - 1
        if s <= 34:
            uniq_idx.append(s)  # store first, even if group extends to 35 (frame 36)

    # Recompute half-step encodings properly (delta vs previous KEPT frame, not skipped)
    def chain(indices, kind):
        total = 0
        prev = zero
        parts = []
        for i in indices:
            px = frames[i]
            raw = to_be_bytes(px)
            xor = to_be_bytes(np.bitwise_xor(prev, px))
            if kind == "rle":
                sz = len(rle_pixels_v2(px)) if (prev is zero or i == indices[0]) else len(xor_rle(prev, px))
            elif kind == "xor_rle":
                sz = len(xor_rle(prev, px)) if i != indices[0] else len(rle_pixels_v2(px))
            elif kind == "dirty_span_rle":
                sz = len(dirty_span_rle(prev, px)) if i != indices[0] else len(rle_pixels_v2(px))
            elif kind == "lz":
                sz = len(lz_compress(raw if i == indices[0] else xor))
            elif kind == "bbox_rle":
                if i == indices[0]:
                    sz = len(rle_pixels_v2(px))
                else:
                    x0, y0, bw, bh, nd = dirty_bbox(prev, px)
                    if nd == 0:
                        sz = 2
                    else:
                        sz = 8 + len(rle_pixels_v2(px[y0 : y0 + bh, x0 : x0 + bw]))
            elif kind == "fw_lz_raw_concat":
                sz = 0  # handled later
            else:
                sz = 0
            parts.append(sz)
            total += sz
            prev = px
        return total, parts

    # Palette encodings
    pal_stats = {}
    for ncolors in (16, 32, 256):
        # global palette from all stored frames 1-35
        stacked = np.stack(frames[:35], axis=0)
        # sample to speed median cut: all unique across 35 frames may be huge
        pal, _ = median_cut(stacked[0], ncolors)  # placeholder
        # better: unique colors of a subsample
        sample = stacked[:, ::4, ::4].reshape(-1)
        # build a fake image of unique sample colors
        uniq = np.unique(sample)
        canvas = uniq.reshape(1, -1)
        pal, _ = median_cut(np.broadcast_to(canvas, (1, uniq.size)).copy(), ncolors)

        def apply_pal(px, pal=pal):
            # nearest in RGB565 expanded
            def ch(u):
                return np.stack([((u >> 11) & 31) * 2, (u >> 5) & 63, (u & 31) * 2], axis=-1).astype(np.int32)

            pc = ch(pal)
            # for speed, map unique colors of this frame
            u, inv = np.unique(px, return_inverse=True)
            uc = ch(u)
            # dist to each pal color
            # uc: Ux3, pc: Px3
            d = ((uc[:, None, :] - pc[None, :, :]) ** 2).sum(-1)
            assign = d.argmin(1).astype(np.uint8)
            idx = assign[inv].reshape(px.shape)
            recon = pal[idx]
            return idx, recon

        sizes_rle = []
        sizes_xor = []
        psnrs = []
        prev_idx = None
        pal_bytes = ncolors * 2
        for i in range(35):
            idx, recon = apply_pal(frames[i])
            psnrs.append(psnr565(frames[i], recon))
            sizes_rle.append(len(index_rle(idx)))
            if prev_idx is None:
                sizes_xor.append(sizes_rle[-1])
            else:
                sizes_xor.append(len(rle_bytes(np.bitwise_xor(prev_idx, idx).tobytes())))
            prev_idx = idx
        pal_stats[ncolors] = {
            "palette_bytes": pal_bytes,
            "frames_1_to_35_index_rle": int(sum(sizes_rle) + pal_bytes),
            "frames_1_to_35_index_xor_rle": int(sum(sizes_xor) + pal_bytes),
            "psnr_min": round(min(psnrs), 2),
            "psnr_mean": round(float(np.mean(psnrs)), 2),
            "first_frame_rle": sizes_rle[0],
            "lossy": True,
        }
        # 18-frame subset
        half_rle = pal_bytes
        prev_idx = None
        for i in half:
            idx, recon = apply_pal(frames[i])
            if prev_idx is None:
                half_rle += len(index_rle(idx))
            else:
                half_rle += len(rle_bytes(np.bitwise_xor(prev_idx, idx).tobytes()))
            prev_idx = idx
        pal_stats[ncolors]["frames_18_index_xor_rle"] = int(half_rle)

    # Lossless global palette? only if unique of 1-35 <= 256
    uniq35 = np.unique(np.stack(frames[:35]))
    lossless_pal = {
        "unique_colors_frames_1_35": int(uniq35.size),
        "unique_colors_all_36": int(np.unique(np.stack(frames)).size),
        "fits_256": bool(uniq35.size <= 256),
    }
    if uniq35.size <= 256:
        pal = np.zeros(256, dtype=np.uint16)
        pal[: uniq35.size] = uniq35
        invmap = {int(c): i for i, c in enumerate(uniq35)}
        total = 2 * int(uniq35.size)
        prev = None
        for i in range(35):
            idx = np.vectorize(invmap.get)(frames[i]).astype(np.uint8)
            if prev is None:
                total += len(index_rle(idx))
            else:
                total += len(rle_bytes(np.bitwise_xor(prev, idx).tobytes()))
            prev = idx
        lossless_pal["frames_1_35_xor_rle"] = total

    # Concatenate encodings for totals (keyframe frame 1, then XOR-RLE)
    totals = {}
    for name, idxs in (("35f_70fps_drop_last", no_last), ("18f_35fps_drop_last", half), ("unique_holds_drop_last", uniq_idx)):
        totals[name] = {}
        for kind in ("rle", "xor_rle", "dirty_span_rle", "lz", "bbox_rle"):
            tot, parts = chain(idxs, kind)
            totals[name][kind] = {"bytes": tot, "per_frame": parts, "n": len(idxs)}
        # also zlib and raw
        raw_sum = RAW * len(idxs)
        totals[name]["raw"] = {"bytes": raw_sum, "n": len(idxs)}
        # keyframe every 8 of xor stream: already xor vs previous kept
        # firmware LZ of concatenated xor frames (one blob)
        blob = b""
        prev = zero
        for i in idxs:
            raw = to_be_bytes(frames[i])
            if i == idxs[0]:
                blob += raw
            else:
                blob += to_be_bytes(np.bitwise_xor(prev, frames[i]))
            prev = frames[i]
        totals[name]["concat_xor_lz"] = {"bytes": len(lz_compress(blob)), "raw_concat": len(blob)}
        totals[name]["concat_xor_zlib"] = {"bytes": len(zlib.compress(blob, 9)), "note": "HOST reference only; zlib is not the firmware LZ"}
        # SPI time if we blit dirty bbox vs previous displayed
        spi = 0.0
        prev = zero
        max_spi = 0.0
        for i in idxs:
            if i == idxs[0]:
                ms = spi_ms(RAW)
            else:
                *_, nd = dirty_bbox(prev, frames[i])
                x0, y0, bw, bh, nd = dirty_bbox(prev, frames[i])
                ms = spi_ms(bw * bh * 2) if nd else 0.0
            spi += ms
            max_spi = max(max_spi, ms)
            prev = frames[i]
        totals[name]["spi_bbox_total_ms"] = round(spi, 1)
        totals[name]["spi_bbox_max_ms"] = round(max_spi, 1)
        totals[name]["spi_full_each_total_ms"] = round(spi_ms(RAW) * len(idxs), 1)

    # Header / identity
    report = {
        "label": "HOST MEASURED",
        "device_pixel_format": "RGB565 big-endian (byte-swapped), 240x240, ST7789 COLMOD 0x55",
        "downsample": "2x2 area average from 480x480 RGB888, then RGB565",
        "source_frames": str(FRAMES_DIR),
        "n_source_png": 36,
        "frame_36_vs_HOME240_diff_pixels": n_diff_home,
        "identical_groups_1based": [{"first": a, "end_exclusive_0based_j": b, "count": c} for a, b, c in groups],
        "identical_groups_human": [f"frames {a}-{b} ({c} copies)" if c > 1 else f"frame {a}" for a, b, c in groups],
        "per_frame": per,
        "totals": totals,
        "palette": pal_stats,
        "lossless_palette": lossless_pal,
        "unique_hold_indices_1based": [i + 1 for i in uniq_idx],
        "half_indices_1based": [i + 1 for i in half],
        "spi": {
            "rate_Hz": SPI_HZ,
            "source": "firmware/src/lcd.c LCD_BAUD 4u = 12 MHz on BOARD_SMK37; comment: 12 MHz took 23 ms for a 240x72 strip",
            "full_frame_bytes": RAW,
            "full_frame_ms_HOST_computed": round(spi_ms(RAW), 2),
            "comment_check_240x72_ms": round(spi_ms(240 * 72 * 2), 2),
        },
        "elapsed_s": round(time.time() - t0, 2),
    }
    OUT_JSON.write_text(json.dumps(report, indent=2))
    # human text
    lines = []
    def p(s=""):
        lines.append(s)
    p("HOST MEASURED box-entry frame budget")
    p(f"elapsed {report['elapsed_s']}s")
    p(f"frame 36 vs HOME-240 differing pixels: {n_diff_home}")
    p(f"identical groups: {report['identical_groups_human']}")
    p(f"unique RGB565 colors frames 1-35: {lossless_pal['unique_colors_frames_1_35']}")
    p(f"unique RGB565 colors all 36: {lossless_pal['unique_colors_all_36']}")
    p("")
    p("Per-frame (1..36): i uniq dirty bbox(x,y,w,h) rle xor_rle dirty_span_rle bbox_rle lz xor_lz spi_bbox_ms")
    for r in per:
        p(
            f"  {r['i']:2d} {r['unique_rgb565']:6d} {r['dirty_px']:6d} {r['bbox']} "
            f"rle={r['rle_px']:6d} xor={r['xor_rle_px']:6d} dsr={r['dirty_span_rle']:6d} "
            f"bb={r['bbox_rle']:6d} lz={r['lz']:6d} xorlz={r['xor_lz']:6d} spi={r['spi_dirty_bbox_ms']:.1f}ms"
        )
    p("")
    for name, block in totals.items():
        p(f"=== {name} ===")
        for k, v in block.items():
            if isinstance(v, dict) and "bytes" in v:
                p(f"  {k}: {v['bytes']:,} B  ({v['bytes']/1024:.1f} KiB) n={v.get('n','')}")
            else:
                p(f"  {k}: {v}")
    p("")
    p("Palette (LOSSY unless noted):")
    for nc, st in pal_stats.items():
        p(f"  {nc} colors: {st}")
    p(f"  lossless: {lossless_pal}")
    p("")
    p(f"SPI full frame {report['spi']['full_frame_ms_HOST_computed']} ms at 12 MHz; 240x72 check {report['spi']['comment_check_240x72_ms']} ms (comment said 23 ms)")
    OUT_TXT.write_text("\n".join(lines) + "\n")
    print(OUT_TXT.read_text())
    print(f"wrote {OUT_JSON} and {OUT_TXT}", file=sys.stderr)


if __name__ == "__main__":
    main()
