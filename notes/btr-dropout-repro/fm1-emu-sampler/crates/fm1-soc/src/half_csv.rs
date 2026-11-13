//! Per-half raw sampler (BTR dropout repro). Guest firmware is not changed.
//!
//! When `x0x_dbg.halves` increments, one CSV row is appended with last_us,
//! late, nested, ui_frames, audio_xruns, audio_peak_pct, the eng_prof[8]
//! delta and eng_prof_frames, eng_job_cost[JB_N] (both cores, wait subtracted),
//! eng_plan, and core-1 busy / job-busy ticks for that half.
//!
//! Addresses and field offsets come from FM1_HALF_LAYOUT (generated from the
//! ELF .symtab and the C structs — this Elite ELF has no DWARF). Nothing in
//! this file hardcodes a RAM address.
//!
//! Env:
//!   FM1_HALF_CSV=PATH      append-only series (created on first row)
//!   FM1_HALF_LAYOUT=PATH   required when FM1_HALF_CSV is set

use crate::{Soc, RAM_BASE};
use std::fs::File;
use std::io::{BufWriter, Write};
use std::sync::Mutex;

struct Layout {
    x0x_dbg: u32,
    field: std::collections::HashMap<String, u32>,
    sym: std::collections::HashMap<String, (u32, u32)>,
    prof_names: Vec<String>,
    job_names: Vec<String>,
    c1_lo: u32,
    c1_hi: u32,
    run_job_lo: u32,
    run_job_hi: u32,
    c1_take_lo: u32,
    c1_take_hi: u32,
}

struct St {
    layout: Layout,
    out: BufWriter<File>,
    last_halves: u32,
    prev_prof: [u32; 8],
    prev_frames: u32,
    have_prev: bool,
    // accumulated since the last emitted half
    c1_busy: u64,
    c1_tot: u64,
    c1_job_busy: u64,
    c1_take_busy: u64,
    c1_job_starts: u32,
    last_in_job: bool,
    rows: u64,
}

static ST: Mutex<Option<St>> = Mutex::new(None);
static INIT: std::sync::Once = std::sync::Once::new();

fn parse_u32(s: &str) -> Option<u32> {
    let s = s.trim();
    if let Some(h) = s.strip_prefix("0x").or_else(|| s.strip_prefix("0X")) {
        u32::from_str_radix(h, 16).ok()
    } else {
        s.parse().ok()
    }
}

fn load_layout(path: &str) -> Result<Layout, String> {
    let text = std::fs::read_to_string(path).map_err(|e| format!("{path}: {e}"))?;
    let mut l = Layout {
        x0x_dbg: 0,
        field: std::collections::HashMap::new(),
        sym: std::collections::HashMap::new(),
        prof_names: Vec::new(),
        job_names: Vec::new(),
        c1_lo: 0,
        c1_hi: 0,
        run_job_lo: 0,
        run_job_hi: 0,
        c1_take_lo: 0,
        c1_take_hi: 0,
    };
    for (n, raw) in text.lines().enumerate() {
        let line = raw.split('#').next().unwrap_or("").trim();
        if line.is_empty() {
            continue;
        }
        let p: Vec<&str> = line.split_whitespace().collect();
        match p[..] {
            ["x0x_dbg", addr, ..] => {
                l.x0x_dbg = parse_u32(addr).ok_or_else(|| format!("line {}: bad x0x_dbg", n + 1))?;
            }
            ["field", name, off] => {
                l.field.insert(
                    name.to_string(),
                    parse_u32(off).ok_or_else(|| format!("line {}: bad field offset", n + 1))?,
                );
            }
            ["sym", name, addr, size] => {
                let a = parse_u32(addr).ok_or_else(|| format!("line {}: bad sym addr", n + 1))?;
                let s = parse_u32(size).ok_or_else(|| format!("line {}: bad sym size", n + 1))?;
                l.sym.insert(name.to_string(), (a, s));
            }
            ["prof", name] => l.prof_names.push(name.to_string()),
            ["job", name] => l.job_names.push(name.to_string()),
            ["c1_text", lo, hi] => {
                l.c1_lo = parse_u32(lo).ok_or_else(|| format!("line {}: bad c1_text", n + 1))?;
                l.c1_hi = parse_u32(hi).ok_or_else(|| format!("line {}: bad c1_text", n + 1))?;
            }
            ["fn", "run_job", addr, size] => {
                l.run_job_lo = parse_u32(addr).unwrap_or(0);
                l.run_job_hi = l.run_job_lo + parse_u32(size).unwrap_or(0);
            }
            ["fn", "c1_take_over", addr, size] => {
                l.c1_take_lo = parse_u32(addr).unwrap_or(0);
                l.c1_take_hi = l.c1_take_lo + parse_u32(size).unwrap_or(0);
            }
            ["elf_sha256", ..] | ["ram_base", ..] => {}
            _ => {}
        }
    }
    if l.x0x_dbg == 0 || !l.field.contains_key("halves") || !l.field.contains_key("last_us") {
        return Err(format!("{path}: missing x0x_dbg / halves / last_us"));
    }
    if l.prof_names.len() != 8 || l.job_names.len() != 5 {
        return Err(format!(
            "{path}: expected 8 prof names and 5 job names, got {} and {}",
            l.prof_names.len(),
            l.job_names.len()
        ));
    }
    Ok(l)
}

fn init() {
    INIT.call_once(|| {
        let Ok(csv) = std::env::var("FM1_HALF_CSV") else { return };
        let layout_path = match std::env::var("FM1_HALF_LAYOUT") {
            Ok(p) => p,
            Err(_) => {
                eprintln!("FM1_HALF_CSV is set but FM1_HALF_LAYOUT is not; half sampler off");
                return;
            }
        };
        let layout = match load_layout(&layout_path) {
            Ok(l) => l,
            Err(e) => {
                eprintln!("FM1_HALF_LAYOUT: {e}; half sampler off");
                return;
            }
        };
        match File::create(&csv) {
            Ok(f) => {
                let mut out = BufWriter::new(f);
                // header
                let mut h = String::from(
                    "halves,last_us,max_us,late,nested,ui_frames,in_audio,audio_xruns,audio_peak_pct,audio_cpu_pct,eng_prof_frames,d_eng_prof_frames",
                );
                for n in &layout.prof_names {
                    h.push_str(",d_");
                    h.push_str(n);
                }
                for n in &layout.job_names {
                    h.push(',');
                    h.push_str(n);
                }
                h.push_str(",eng_plan,eng_guard_count,perf_cyc_ok,c1_busy_ticks,c1_ticks,c1_job_busy_ticks,c1_take_busy_ticks,c1_job_starts,emu_cycles\n");
                let _ = out.write_all(h.as_bytes());
                *ST.lock().unwrap() = Some(St {
                    layout,
                    out,
                    last_halves: 0,
                    prev_prof: [0; 8],
                    prev_frames: 0,
                    have_prev: false,
                    c1_busy: 0,
                    c1_tot: 0,
                    c1_job_busy: 0,
                    c1_take_busy: 0,
                    c1_job_starts: 0,
                    last_in_job: false,
                    rows: 0,
                });
                eprintln!("half sampler: writing {csv} (layout {layout_path})");
            }
            Err(e) => eprintln!("FM1_HALF_CSV {csv}: {e}"),
        }
    });
}

fn read_u32(soc: &Soc, addr: u32) -> u32 {
    let o = addr.wrapping_sub(RAM_BASE) as usize;
    if o + 4 <= soc.ram.len() {
        soc.ram.read32(o as u32)
    } else {
        0
    }
}

fn read_u8(soc: &Soc, addr: u32) -> u8 {
    let o = addr.wrapping_sub(RAM_BASE) as usize;
    if o < soc.ram.len() {
        soc.ram.read8(o as u32)
    } else {
        0
    }
}

fn field(st: &St, soc: &Soc, name: &str) -> u32 {
    let off = *st.layout.field.get(name).unwrap_or(&0);
    read_u32(soc, st.layout.x0x_dbg + off)
}

fn sym(st: &St, soc: &Soc, name: &str) -> u32 {
    match st.layout.sym.get(name) {
        Some(&(a, _)) => read_u32(soc, a),
        None => 0,
    }
}

fn in_range(pc: u32, lo: u32, hi: u32) -> bool {
    lo != 0 && hi > lo && pc >= lo && pc < hi
}

/// Called after each core-1 slice, next to `core_load::note`. Accumulates
/// core-1 busy; when guest `halves` increments, appends one raw CSV row.
pub fn note(soc: &Soc, pc: u32, from: u64, to: u64) {
    init();
    let mut g = ST.lock().unwrap();
    let Some(st) = g.as_mut() else { return };

    let dt = to.saturating_sub(from);
    if dt > 0 {
        st.c1_tot += dt;
        let waiting = in_range(pc, st.layout.c1_lo, st.layout.c1_hi);
        if !waiting && st.layout.c1_lo != 0 {
            st.c1_busy += dt;
        }
        let in_job = in_range(pc, st.layout.run_job_lo, st.layout.run_job_hi);
        if in_job {
            st.c1_job_busy += dt;
            if !st.last_in_job {
                st.c1_job_starts = st.c1_job_starts.saturating_add(1);
            }
        }
        st.last_in_job = in_job;
        if in_range(pc, st.layout.c1_take_lo, st.layout.c1_take_hi) {
            st.c1_take_busy += dt;
        }
    }

    let halves = field(st, soc, "halves");
    if halves == 0 || halves == st.last_halves {
        return;
    }
    // First observation: remember the baseline without emitting a boot-zero row.
    if st.last_halves == 0 && !st.have_prev {
        st.last_halves = halves;
        for i in 0..8 {
            st.prev_prof[i] = read_u32(
                soc,
                st.layout.sym.get("eng_prof").map(|p| p.0).unwrap_or(0) + (i as u32) * 4,
            );
        }
        st.prev_frames = sym(st, soc, "eng_prof_frames");
        st.have_prev = true;
        st.c1_busy = 0;
        st.c1_tot = 0;
        st.c1_job_busy = 0;
        st.c1_take_busy = 0;
        st.c1_job_starts = 0;
        return;
    }
    st.last_halves = halves;

    let last_us = field(st, soc, "last_us");
    let max_us = field(st, soc, "max_us");
    let late = field(st, soc, "late");
    let nested = field(st, soc, "nested");
    let ui_frames = field(st, soc, "ui_frames");
    let in_audio = field(st, soc, "in_audio");
    let xruns = sym(st, soc, "audio_xruns");
    let peak = sym(st, soc, "audio_peak_pct");
    let cpu = sym(st, soc, "audio_cpu_pct");
    let frames = sym(st, soc, "eng_prof_frames");
    let d_frames = frames.wrapping_sub(st.prev_frames);
    let prof_base = st.layout.sym.get("eng_prof").map(|p| p.0).unwrap_or(0);
    let mut d_prof = [0u32; 8];
    let mut now_prof = [0u32; 8];
    for i in 0..8 {
        now_prof[i] = read_u32(soc, prof_base + (i as u32) * 4);
        d_prof[i] = now_prof[i].wrapping_sub(st.prev_prof[i]);
    }
    st.prev_prof = now_prof;
    st.prev_frames = frames;

    let job_base = st.layout.sym.get("eng_job_cost").map(|p| p.0).unwrap_or(0);
    let mut jobs = [0u32; 5];
    for i in 0..5 {
        jobs[i] = read_u32(soc, job_base + (i as u32) * 4);
    }
    let plan = match st.layout.sym.get("eng_plan") {
        Some(&(a, _)) => read_u8(soc, a) as u32,
        None => 0,
    };
    let guard = sym(st, soc, "eng_guard_count");
    let cyc_ok = sym(st, soc, "perf_cyc_ok");

    let c1_busy = st.c1_busy;
    let c1_tot = st.c1_tot;
    let c1_job = st.c1_job_busy;
    let c1_take = st.c1_take_busy;
    let c1_starts = st.c1_job_starts;
    st.c1_busy = 0;
    st.c1_tot = 0;
    st.c1_job_busy = 0;
    st.c1_take_busy = 0;
    st.c1_job_starts = 0;

    let mut line = format!(
        "{halves},{last_us},{max_us},{late},{nested},{ui_frames},{in_audio},{xruns},{peak},{cpu},{frames},{d_frames}"
    );
    for v in &d_prof { line.push(','); line.push_str(&v.to_string()); }
    for v in &jobs { line.push(','); line.push_str(&v.to_string()); }
    line.push_str(&format!(
        ",{plan},{guard},{cyc_ok},{c1_busy},{c1_tot},{c1_job},{c1_take},{c1_starts},{}\n",
        soc.periph.cycles
    ));
    let _ = st.out.write_all(line.as_bytes());
    st.rows += 1;
    if st.rows % 512 == 0 {
        let _ = st.out.flush();
    }
}

/// Flush the series at the end of a --shot / --bench.
pub fn finish() {
    if let Ok(mut g) = ST.lock() {
        if let Some(st) = g.as_mut() {
            let _ = st.out.flush();
            eprintln!("half sampler: {} rows (last halves {})", st.rows, st.last_halves);
        }
    }
}
