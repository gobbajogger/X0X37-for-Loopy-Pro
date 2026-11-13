//! Host-only observation for the pinned DnBOX 94197fc8 firmware. No guest writes or clocks.

use crate::{Ram, RAM_BASE};
use std::io::{self, Write};

/// DnBOX94197's package/app prefix. The runner may prepare sample-slot data above
/// this boundary, but restoring flash must not silently replace the pinned firmware.
pub fn validate_flash_fixture(installed: &[u8], restored: &[u8]) -> io::Result<()> {
    if installed.len() != crate::FLASH_SIZE || restored.len() != crate::FLASH_SIZE ||
        installed[..0x9d000] != restored[..0x9d000] {
        return Err(io::Error::new(io::ErrorKind::InvalidData,
            "DnBOX flash fixture has wrong size or changes protected package bytes below0x9D000"));
    }
    Ok(())
}

#[derive(Clone, Copy, Debug)]
pub struct DmaEdge {
    pub seq: u64,
    pub scheduled_cycle: u64,
    pub observed_cycle: u64,
    pub deadline_cycle: u64,
    pub half: u32,
    pub address: u32,
    pub words: u32,
}

struct Watch {
    name: String,
    address: u32,
    bytes: u32,
    scene: bool,
}
impl Watch {
    fn contains(&self, address: u32, width: u32) -> bool {
        address >= self.address && address as u64 + width as u64 <= self.address as u64 + self.bytes as u64
    }
    fn values(&self, ram: &Ram) -> String {
        let offset = self.address - RAM_BASE;
        let stride = self.element_bytes();
        let values: Vec<String> = (0..self.bytes).step_by(stride).map(|i| {
            if stride == 4 { ram.read32(offset + i) } else { ram.read8(offset + i) as u32 }.to_string()
        }).collect();
        format!("[{}]", values.join(","))
    }
    fn element_bytes(&self) -> usize {
        if self.scene || self.bytes % 4 != 0 { 1 } else { 4 }
    }
}

struct Pending {
    seq: u64,
    entry: u64,
    depth: usize,
    last_us_writes: u32,
    stretch_started: bool,
    stage_four: bool,
    fresh_kbuf: [bool; 256],
    mix_writes: u64,
    mix_peak: f64,
    invalid: bool,
}

/// A bounded, buffered host sink. Its timestamps are Periph ticks: in strict mode each
/// CPU cycle has 16 ticks. Core-1 writes share the existing slice model's global clock;
/// they are ordered observations, not independently timed core-1 instructions.
pub struct Observer {
    watches: Vec<Watch>,
    writer: Box<dyn Write + Send>,
    error: Option<io::Error>,
    dma_seq: u64,
    completed: u64,
    resets: u64,
    active: Option<Pending>,
}

impl Observer {
    pub fn new(map: &str, ram_bytes: usize, hz: u64, writer: Box<dyn Write + Send>) -> io::Result<Self> {
        let required = [
            ("x0x_dbg", 52), ("audio_xruns", 4), ("eng_prof", 32),
            ("eng_prof_frames", 4), ("eng_job_cost", 20), ("eng_plan", 1),
            ("eng_c1_mask", 1), ("eng_stage", 4), ("eng_mastered", 4),
            ("eng_do_master", 1), ("eng_job_arg", 4), ("brk_silent_now", 4),
            ("usr_nz", 3), ("nloops", 4), ("pack_n", 4),
            ("kbuf", 1024), ("dry_l", 1024), ("dry_r", 1024),
        ];
        let bad = |s: &str| io::Error::new(io::ErrorKind::InvalidData, s.to_string());
        let mut watches: Vec<Watch> = Vec::new();
        let (mut scene_count, mut scene_bytes) = (0, 0u32);
        for line in map.lines().map(str::trim).filter(|l| !l.is_empty() && !l.starts_with('#')) {
            let fields: Vec<&str> = line.split_whitespace().collect();
            if fields.len() != 3 { return Err(bad("observer map requires: name hex-address decimal-byte-size")); }
            let expected = required.iter().find(|(name, _)| *name == fields[0]).map(|(_, bytes)| *bytes);
            let scene = expected.is_none();
            if scene && !fields[0].strip_prefix("scene_").is_some_and(|suffix| !suffix.is_empty() &&
                suffix.bytes().all(|b| b.is_ascii_lowercase() || b.is_ascii_digit() || b == b'_')) {
                return Err(bad("unknown observer map field"));
            }
            let address = u32::from_str_radix(fields[1].trim_start_matches("0x"), 16).map_err(|_| bad("invalid observer address"))?;
            let bytes: u32 = fields[2].parse().map_err(|_| bad("invalid observer byte size"))?;
            if expected.is_some_and(|expected| bytes != expected) || (scene && !(1..=512).contains(&bytes)) {
                return Err(bad("observer field has wrong byte size"));
            }
            if scene {
                scene_count += 1;
                scene_bytes += bytes;
                if scene_count > 64 || scene_bytes > 4096 { return Err(bad("observer scene ranges exceed count or byte limit")); }
            }
            if address < RAM_BASE || address as u64 + bytes as u64 > RAM_BASE as u64 + ram_bytes as u64 ||
                (!scene && bytes % 4 == 0 && address % 4 != 0) {
                return Err(bad("observer range is outside RAM or misaligned"));
            }
            if watches.iter().any(|w| w.name == fields[0] ||
                ((address as u64) < w.address as u64 + w.bytes as u64 && (w.address as u64) < address as u64 + bytes as u64)) {
                return Err(bad("duplicate or overlapping observer ranges"));
            }
            watches.push(Watch { name: fields[0].to_owned(), address, bytes, scene });
        }
        if required.iter().any(|(name, _)| !watches.iter().any(|w| w.name == *name)) {
            return Err(bad("missing required observer range"));
        }
        let fields = watches.iter().map(|w| format!("\"{}\":{{\"address\":{},\"bytes\":{},\"element_bytes\":{}}}",
            w.name, w.address, w.bytes, w.element_bytes())).collect::<Vec<_>>().join(",");
        let mut result = Self { watches, writer, error: None, dma_seq: 0, completed: 0, resets: 0, active: None };
        result.emit(format!("{{\"event\":\"observer\",\"schema\":1,\"hz\":{hz},\"clock_unit\":\"emulator_tick\",\"cycle_ticks\":16,\"firmware_source\":\"94197fc839b00aff47b98ca02f3c6396e4a6eabf\",\"runner_source\":\"31157d25caf7284080cdb5d86ab7e8e438120fd9\",\"source_fields\":\"expected_profile_not_artifact_attestation\",\"store_time\":\"bus_write_before_remaining_instruction_cost\",\"core1_time\":\"global_slice_clock\",\"eng_prof_scope\":\"guest_counters_with_known_core1_omissions\",\"watches\":{{{fields}}}}}"));
        result.check()?;
        Ok(result)
    }

    fn emit(&mut self, line: String) {
        if self.error.is_none() {
            if let Err(e) = writeln!(self.writer, "{line}") { self.error = Some(e); }
        }
    }
    fn check(&mut self) -> io::Result<()> {
        if let Some(e) = self.error.take() { return Err(e); }
        Ok(())
    }
    fn at(&self, name: &str) -> u32 { self.watches.iter().find(|w| w.name == name).unwrap().address - RAM_BASE }
    fn snapshot(&self, ram: &Ram) -> String {
        self.watches.iter().filter(|w| w.bytes < 1024).map(|w| format!("\"{}\":{}", w.name, w.values(ram))).collect::<Vec<_>>().join(",")
    }
    pub fn dma(&mut self, edge: DmaEdge) {
        self.dma_seq = edge.seq;
        self.emit(format!("{{\"event\":\"dma\",\"seq\":{},\"scheduled_cycle\":{},\"observed_cycle\":{},\"deadline_cycle\":{},\"half\":{},\"address\":{},\"words\":{}}}",
            edge.seq, edge.scheduled_cycle, edge.observed_cycle, edge.deadline_cycle, edge.half, edge.address, edge.words));
    }
    pub fn irq_enter(&mut self, ram: &Ram, cycle: u64, depth: usize) {
        if self.active.is_some() {
            self.emit(format!("{{\"event\":\"incomplete\",\"reason\":\"reentered_alnk\",\"cycle\":{cycle}}}"));
        }
        self.active = Some(Pending { seq: self.dma_seq, entry: cycle, depth, last_us_writes: 0,
            stretch_started: false, stage_four: false, fresh_kbuf: [false; 256], mix_writes: 0, mix_peak: 0.0, invalid: false });
        self.emit(format!("{{\"event\":\"irq_enter\",\"seq\":{},\"cycle\":{cycle},\"depth\":{depth},\"snapshot\":{{{}}}}}", self.dma_seq, self.snapshot(ram)));
    }

    /// Called after an actual guest RAM store, including stores of the same value.
    /// `old` was read directly from RAM before that store; no bus reads or guest ticks.
    pub fn store(&mut self, ram: &Ram, cycle: u64, core: u8, address: u32, width: u32, old: u32, value: u32) {
        if self.active.is_none() { return; }
        // A compiler's word store/memset can span several small scene fields or start
        // outside an unaligned one. Emit every intersecting byte from post-write RAM;
        // these are replay events, not a claim that the guest issued byte stores.
        let seq = self.active.as_ref().unwrap().seq;
        let scene_bytes = self.watches.iter().filter(|w| w.scene).flat_map(|w| {
            let lo = (address as u64).max(w.address as u64);
            let hi = (address as u64 + width as u64).min(w.address as u64 + w.bytes as u64);
            (lo..hi).map(move |at| (w.name.clone(), (at - w.address as u64) as u32, ram.read8((at - RAM_BASE as u64) as u32)))
        }).collect::<Vec<_>>();
        for (name, offset, value) in scene_bytes {
            self.emit(format!("{{\"event\":\"store\",\"seq\":{seq},\"cycle\":{cycle},\"core\":{core},\"name\":\"{name}\",\"offset\":{offset},\"width\":1,\"value\":{value}}}"));
        }
        let Some(watch) = self.watches.iter().find(|w| !w.scene && w.contains(address, width)) else { return };
        let (name, offset, bytes) = (watch.name.clone(), address - watch.address, watch.bytes);
        let n_raw = ram.read32(self.at("eng_job_arg")) & 255;
        let n = if n_raw == 0 { 256 } else { n_raw };
        let silent = ram.read32(self.at("brk_silent_now")) != 0;
        let kbuf = self.at("kbuf");
        let pending = self.active.as_mut().unwrap();
        if name == "eng_stage" && offset == 0 && width == 4 {
            if value == 0 {
                pending.stretch_started = true;
                pending.stage_four = false;
                pending.fresh_kbuf.fill(false);
            } else if value == 4 {
                pending.stage_four = pending.stretch_started;
            }
        }
        if name == "kbuf" && pending.stretch_started && width == 4 && offset % 4 == 0 {
            pending.fresh_kbuf[(offset / 4) as usize] = true;
        }
        if name == "x0x_dbg" && offset == 32 && width == 4 { pending.last_us_writes += 1; }
        if (name == "dry_l" || name == "dry_r") && core == 0 && pending.stage_four && !silent {
            if width != 4 || offset % 4 != 0 || offset / 4 >= n || !pending.fresh_kbuf[(offset / 4) as usize] {
                pending.invalid = true;
            } else {
                let (before, after, sample) = (f32::from_bits(old), f32::from_bits(value), f32::from_bits(ram.read32(kbuf + offset)));
                if !before.is_finite() || !after.is_finite() || !sample.is_finite() {
                    pending.invalid = true;
                } else if sample != 0.0 && before != after {
                    pending.mix_writes += 1;
                    pending.mix_peak = pending.mix_peak.max((after as f64 - before as f64).abs());
                }
            }
        }
        if bytes < 1024 {
            let seq = pending.seq;
            self.emit(format!("{{\"event\":\"store\",\"seq\":{seq},\"cycle\":{cycle},\"core\":{core},\"name\":\"{name}\",\"offset\":{offset},\"width\":{width},\"value\":{value}}}"));
        }
    }

    /// Called after the existing CPU0 block/step. A TIMER5 return leaves the ALNK depth
    /// present. RTI exits fast execution before any following guest instruction; that
    /// path can prepay one tick of the next instruction (reported as an upper bound).
    pub fn after_block(&mut self, ram: &Ram, cycle: u64, depth: usize, uncertainty: u8) {
        if !self.active.as_ref().is_some_and(|p| depth < p.depth) { return; }
        let p = self.active.take().unwrap();
        let last_us = ram.read32(self.at("x0x_dbg") + 32);
        self.completed += 1;
        self.emit(format!("{{\"event\":\"half\",\"seq\":{},\"entry_cycle\":{},\"completion_cycle\":{cycle},\"completion_uncertainty_cycles\":{uncertainty},\"dma_seq_at_completion\":{},\"last_us_writes\":{},\"last_us\":{last_us},\"snapshot\":{{{}}},\"break_mix\":{{\"nonzero_writes\":{},\"peak_delta\":{},\"invalid\":{}}}}}",
            p.seq, p.entry, self.dma_seq, p.last_us_writes, self.snapshot(ram), p.mix_writes, p.mix_peak, p.invalid));
    }
    pub fn reset(&mut self, cycle: u64) {
        self.resets += 1;
        self.emit(format!("{{\"event\":\"reset\",\"cycle\":{cycle},\"pending\":{}}}", self.active.is_some()));
        self.active = None;
    }
    pub fn finish(&mut self, cycle: u64) -> io::Result<()> {
        let pending = self.active.as_ref().map_or("null".to_string(), |p| p.seq.to_string());
        self.emit(format!("{{\"event\":\"finish\",\"cycle\":{cycle},\"completed\":{},\"dma_edges\":{},\"resets\":{},\"pending_seq\":{pending}}}", self.completed, self.dma_seq, self.resets));
        self.check()?;
        self.writer.flush()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::sync::{Arc, Mutex};

    #[derive(Clone)]
    struct Capture(Arc<Mutex<Vec<u8>>>);
    impl std::io::Write for Capture {
        fn write(&mut self, bytes: &[u8]) -> std::io::Result<usize> {
            self.0.lock().unwrap().extend_from_slice(bytes);
            Ok(bytes.len())
        }
        fn flush(&mut self) -> std::io::Result<()> { Ok(()) }
    }

    fn map() -> String {
        let mut at = RAM_BASE;
        let mut text = String::new();
        for (name, bytes) in [
            ("x0x_dbg", 52), ("audio_xruns", 4), ("eng_prof", 32),
            ("eng_prof_frames", 4), ("eng_job_cost", 20), ("eng_plan", 1),
            ("eng_c1_mask", 1), ("eng_stage", 4), ("eng_mastered", 4),
            ("eng_do_master", 1), ("eng_job_arg", 4), ("brk_silent_now", 4),
            ("usr_nz", 3), ("nloops", 4), ("pack_n", 4),
            ("kbuf", 1024), ("dry_l", 1024), ("dry_r", 1024),
        ] {
            text.push_str(&format!("{name} {at:#x} {bytes}\n"));
            at += (bytes + 3) & !3;
        }
        text
    }

    fn fixture() -> (Observer, Ram, Capture) {
        let capture = Capture(Arc::new(Mutex::new(Vec::new())));
        let observer = Observer::new(&map(), 8192, 5_760_000_000, Box::new(capture.clone())).unwrap();
        (observer, Ram::new(8192), capture)
    }
    fn address(o: &Observer, name: &str) -> u32 { o.watches.iter().find(|w| w.name == name).unwrap().address }
    fn write(o: &mut Observer, ram: &mut Ram, name: &str, offset: u32, value: u32) {
        let addr = address(o, name) + offset;
        let old = ram.read32(addr - RAM_BASE);
        ram.write32(addr - RAM_BASE, value);
        o.store(ram, 1010, 0, addr, 4, old, value);
    }
    fn start(o: &mut Observer, ram: &Ram) {
        o.dma(DmaEdge { seq: 1, scheduled_cycle: 1000, observed_cycle: 1000,
            deadline_cycle: 2000, half: 0, address: RAM_BASE + 5000, words: 1024 });
        o.irq_enter(ram, 1010, 1);
    }
    fn output(capture: &Capture) -> String { String::from_utf8(capture.0.lock().unwrap().clone()).unwrap() }

    #[test]
    fn equal_last_us_stores_are_counted_not_change_detected() {
        let (mut o, mut ram, c) = fixture();
        start(&mut o, &ram);
        write(&mut o, &mut ram, "x0x_dbg", 32, 900);
        write(&mut o, &mut ram, "x0x_dbg", 32, 900);
        o.after_block(&ram, 1900, 0, 1);
        o.finish(1900).unwrap();
        let text = output(&c);
        assert_eq!(text.lines().filter(|l| l.contains("\"name\":\"x0x_dbg\"") && l.contains("\"offset\":32")).count(), 2);
        assert!(text.contains("\"last_us_writes\":2"));
    }

    #[test]
    fn nested_timer_return_does_not_commit_before_late_and_xruns() {
        let (mut o, mut ram, c) = fixture();
        start(&mut o, &ram);
        write(&mut o, &mut ram, "x0x_dbg", 32, 11000);
        o.after_block(&ram, 1800, 1, 0);
        assert!(!output(&c).contains("\"event\":\"half\""));
        write(&mut o, &mut ram, "x0x_dbg", 20, 7);
        write(&mut o, &mut ram, "audio_xruns", 0, 7);
        o.after_block(&ram, 2100, 0, 1);
        o.finish(2100).unwrap();
        let text = output(&c);
        assert_eq!(text.lines().filter(|l| l.contains("\"event\":\"half\"")).count(), 1);
        assert!(text.contains("\"audio_xruns\":[7]"));
        assert!(text.contains("\"completion_uncertainty_cycles\":1"));
    }

    #[test]
    fn dma_sequence_does_not_alias_when_same_half_returns() {
        let (mut o, ram, c) = fixture();
        start(&mut o, &ram);
        for seq in 2..=3 {
            o.dma(DmaEdge { seq, scheduled_cycle: seq * 1000, observed_cycle: seq * 1000,
                deadline_cycle: (seq + 1) * 1000, half: ((seq - 1) & 1) as u32,
                address: RAM_BASE + 5000, words: 1024 });
        }
        o.after_block(&ram, 3500, 0, 0);
        o.finish(3500).unwrap();
        assert!(output(&c).contains("\"dma_seq_at_completion\":3"));
        assert!(output(&c).contains("\"event\":\"half\",\"seq\":1"));
    }

    #[test]
    fn wrapping_guest_counters_are_retained_as_unsigned_raw_values() {
        let (mut o, mut ram, c) = fixture();
        start(&mut o, &ram);
        write(&mut o, &mut ram, "eng_prof", 0, u32::MAX - 2);
        write(&mut o, &mut ram, "eng_prof", 0, 3);
        o.after_block(&ram, 1800, 0, 0);
        o.finish(1800).unwrap();
        let text = output(&c);
        assert!(text.contains("\"value\":4294967293"));
        assert!(text.contains("\"eng_prof\":[3,0,0,0,0,0,0,0]"));
    }

    #[test]
    fn break_mix_requires_current_stage_four_nonzero_sample_and_live_voice() {
        let (mut o, mut ram, c) = fixture();
        start(&mut o, &ram);
        write(&mut o, &mut ram, "eng_job_arg", 0, 64);
        write(&mut o, &mut ram, "eng_stage", 0, 0);
        write(&mut o, &mut ram, "kbuf", 0, 0.5f32.to_bits());
        write(&mut o, &mut ram, "dry_l", 0, 0.25f32.to_bits()); // before stage four
        write(&mut o, &mut ram, "eng_stage", 0, 4);
        write(&mut o, &mut ram, "brk_silent_now", 0, 1);
        write(&mut o, &mut ram, "dry_l", 0, 0.5f32.to_bits()); // silent
        write(&mut o, &mut ram, "brk_silent_now", 0, 0);
        write(&mut o, &mut ram, "dry_l", 0, 0.75f32.to_bits()); // sole valid delta
        write(&mut o, &mut ram, "eng_stage", 0, 0);
        write(&mut o, &mut ram, "dry_l", 0, 0.0f32.to_bits()); // next stretch clear
        o.after_block(&ram, 1800, 0, 0);
        o.finish(1800).unwrap();
        assert!(output(&c).contains("\"nonzero_writes\":1,\"peak_delta\":0.25"));
    }

    #[test]
    fn signed_zero_is_silent_and_nan_invalidates_mix_evidence() {
        let (mut o, mut ram, c) = fixture();
        start(&mut o, &ram);
        write(&mut o, &mut ram, "eng_stage", 0, 0);
        write(&mut o, &mut ram, "eng_job_arg", 0, 64);
        write(&mut o, &mut ram, "eng_stage", 0, 4);
        write(&mut o, &mut ram, "kbuf", 0, (-0.0f32).to_bits());
        write(&mut o, &mut ram, "dry_l", 0, (-0.0f32).to_bits());
        write(&mut o, &mut ram, "kbuf", 0, f32::NAN.to_bits());
        write(&mut o, &mut ram, "dry_l", 0, 0.5f32.to_bits());
        o.after_block(&ram, 1800, 0, 0);
        o.finish(1800).unwrap();
        let text = output(&c);
        assert!(text.contains("\"nonzero_writes\":0"));
        assert!(text.contains("\"invalid\":true"));
        assert!(!text.contains("NaN"));
    }

    #[test]
    fn previous_stretch_kbuf_does_not_qualify_current_break_mix() {
        let (mut o, mut ram, c) = fixture();
        start(&mut o, &ram);
        write(&mut o, &mut ram, "eng_stage", 0, 0);
        write(&mut o, &mut ram, "kbuf", 0, 0.5f32.to_bits());
        write(&mut o, &mut ram, "eng_stage", 0, 0); // a new stretch, no new decoder output
        write(&mut o, &mut ram, "eng_stage", 0, 4);
        write(&mut o, &mut ram, "dry_l", 0, 0.5f32.to_bits());
        o.after_block(&ram, 1800, 0, 0);
        o.finish(1800).unwrap();
        assert!(output(&c).contains("\"nonzero_writes\":0,\"peak_delta\":0,\"invalid\":true"));
    }

    #[test]
    fn missing_overlapping_and_out_of_ram_maps_fail_closed() {
        for text in [String::new(), map() + "eng_stage 0x01c01000 4\n",
            map().replace("x0x_dbg 0x1c00000 52", "x0x_dbg 0x1bfffffc 52")] {
            assert!(Observer::new(&text, 8192, 5_760_000_000, Box::new(Vec::<u8>::new())).is_err());
        }
        let text = map().lines().map(|l| if l.starts_with("dry_r ") { "dry_r 0x1c00000 1024" } else { l }).collect::<Vec<_>>().join("\n");
        assert!(Observer::new(&text, 8192, 5_760_000_000, Box::new(Vec::<u8>::new())).is_err());
    }

    #[test]
    fn truncated_irq_is_reported_as_pending() {
        let (mut o, ram, c) = fixture();
        start(&mut o, &ram);
        o.finish(1500).unwrap();
        assert!(output(&c).contains("\"pending_seq\":1"));
        assert!(!output(&c).contains("\"event\":\"half\""));
    }

    #[test]
    fn optional_scene_ranges_snapshot_raw_bytes_even_when_word_sized_or_unaligned() {
        let capture = Capture(Arc::new(Mutex::new(Vec::new())));
        let text = map() + "scene_chord 0x1c01801 8\nscene_kind 0x1c01820 4\n";
        let result = Observer::new(&text, 8192, 5_760_000_000, Box::new(capture.clone()));
        assert!(result.is_ok(), "valid optional scene ranges must be accepted");
        let mut observer = result.unwrap();
        let mut ram = Ram::new(8192);
        ram.load(0x1801, &[17, 34, 51, 68, 165, 0, 255, 128]);
        ram.load(0x1820, &[1, 2, 3, 4]);
        start(&mut observer, &ram);
        ram.write8(0x1802, 127);
        observer.store(&ram, 1020, 0, RAM_BASE + 0x1802, 1, 34, 127);
        observer.after_block(&ram, 1800, 0, 0);
        observer.finish(1800).unwrap();
        let trace = output(&capture);
        assert!(trace.contains("\"bytes\":8,\"element_bytes\":1"));
        assert!(trace.contains("\"scene_kind\":{\"address\":29366304,\"bytes\":4,\"element_bytes\":1}"));
        assert!(trace.contains("\"scene_chord\":[17,127,51,68,165,0,255,128]"));
        assert!(trace.contains("\"scene_kind\":[1,2,3,4]"));
        assert!(trace.contains("\"name\":\"scene_chord\",\"offset\":1,\"width\":1,\"value\":127"));
        assert!(trace.contains("\"eng_prof\":[0,0,0,0,0,0,0,0]"));
    }

    #[test]
    fn optional_fields_do_not_replace_missing_required_fields() {
        let incomplete = map().lines().filter(|line| !line.starts_with("eng_prof "))
            .collect::<Vec<_>>().join("\n") + "\nscene_kind 0x1c01820 4\n";
        assert!(Observer::new(&incomplete, 8192, 5_760_000_000, Box::new(Vec::<u8>::new())).is_err());
    }

    fn replay_scene_bytes(trace: &str, name: &str, bytes: usize) -> Vec<u8> {
        let mut replay = vec![0; bytes];
        for line in trace.lines().filter(|line| line.contains("\"event\":\"store\"") && line.contains(&format!("\"name\":\"{name}\""))) {
            let number = |key: &str| -> usize {
                line.split(&format!("\"{key}\":")).nth(1).unwrap().split([',', '}']).next().unwrap().parse().unwrap()
            };
            assert_eq!(number("width"), 1, "optional stores must be clipped byte events");
            replay[number("offset")] = u8::try_from(number("value")).unwrap();
        }
        replay
    }

    #[test]
    fn optional_scene_partial_word_store_replays_only_overlapping_bytes() {
        let capture = Capture(Arc::new(Mutex::new(Vec::new())));
        let mut observer = Observer::new(&(map() + "scene_chord 0x1c01801 3\n"), 8192,
            5_760_000_000, Box::new(capture.clone())).unwrap();
        let mut ram = Ram::new(8192);
        start(&mut observer, &ram);
        ram.write32(0x1800, 0x11223344);
        observer.store(&ram, 1020, 0, RAM_BASE + 0x1800, 4, 0, 0x11223344);
        observer.after_block(&ram, 1800, 0, 0);
        observer.finish(1800).unwrap();
        let trace = output(&capture);
        assert_eq!(replay_scene_bytes(&trace, "scene_chord", 3), vec![51, 34, 17]);
        assert!(trace.contains("\"scene_chord\":[51,34,17]"));
        assert_eq!(trace.lines().filter(|line| line.contains("\"name\":\"scene_chord\"")).count(), 3);
    }

    #[test]
    fn optional_scene_word_store_covering_two_fields_replays_both() {
        let capture = Capture(Arc::new(Mutex::new(Vec::new())));
        // Reverse address order in the map also rejects an accidental first-match policy.
        let mut observer = Observer::new(&(map() + "scene_right 0x1c01812 2\nscene_left 0x1c01810 2\n"), 8192,
            5_760_000_000, Box::new(capture.clone())).unwrap();
        let mut ram = Ram::new(8192);
        start(&mut observer, &ram);
        ram.write32(0x1810, 0x89abcdef);
        observer.store(&ram, 1020, 1, RAM_BASE + 0x1810, 4, 0, 0x89abcdef);
        observer.after_block(&ram, 1800, 0, 0);
        observer.finish(1800).unwrap();
        let trace = output(&capture);
        assert_eq!(replay_scene_bytes(&trace, "scene_left", 2), vec![239, 205]);
        assert_eq!(replay_scene_bytes(&trace, "scene_right", 2), vec![171, 137]);
        assert!(trace.contains("\"scene_left\":[239,205]"));
        assert!(trace.contains("\"scene_right\":[171,137]"));
        assert_eq!(trace.lines().filter(|line| line.contains("\"event\":\"store\"") && line.contains("\"name\":\"scene_")).count(), 4);
    }

    #[test]
    fn optional_scene_ranges_enforce_names_bounds_sizes_and_overlap() {
        for extra in [
            "scene_ 0x1c01800 1\n", "scene_Upper 0x1c01800 1\n", "scene_bad-name 0x1c01800 1\n",
            "Scene_kind 0x1c01800 1\n", "scene_kind 0x1c01800 0\n", "scene_kind 0x1c01800 513\n",
            "scene_kind 0x1c01fff 2\n", "scene_kind 0xfffffffe 4\n", "scene_kind 0x1bffffff 1\n",
            "scene_kind 0x1c00001 1\n", "scene_kind 0x1c01800 4\nscene_arp 0x1c01802 4\n",
            "scene_kind 0x1c01800 1\nscene_kind 0x1c01810 1\n",
        ] {
            assert!(Observer::new(&(map() + extra), 8192, 5_760_000_000, Box::new(Vec::<u8>::new())).is_err(), "accepted {extra}");
        }
    }

    #[test]
    fn optional_scene_ranges_accept_limits_and_reject_excess() {
        let mut count_map = map();
        for i in 0..64 { count_map.push_str(&format!("scene_v{i} {:#x} 1\n", RAM_BASE + 0x1800 + i)); }
        assert!(Observer::new(&count_map, 32768, 5_760_000_000, Box::new(Vec::<u8>::new())).is_ok(), "64 optional ranges must fit");
        count_map.push_str("scene_extra 0x1c01840 1\n");
        assert!(Observer::new(&count_map, 32768, 5_760_000_000, Box::new(Vec::<u8>::new())).is_err());
        let mut byte_map = map();
        for i in 0..8 { byte_map.push_str(&format!("scene_v{i} {:#x} 512\n", RAM_BASE + 0x2001 + i * 512)); }
        assert!(Observer::new(&byte_map, 32768, 5_760_000_000, Box::new(Vec::<u8>::new())).is_ok(), "4096 optional bytes must fit");
        byte_map.push_str("scene_extra 0x1c03001 1\n");
        assert!(Observer::new(&byte_map, 32768, 5_760_000_000, Box::new(Vec::<u8>::new())).is_err());
    }

    #[test]
    fn flash_restore_rejects_protected_prefix_changes_and_wrong_size() {
        let installed = vec![0xa5; crate::FLASH_SIZE];
        let mut restored = installed.clone();
        restored[0x9d000] = 0;
        assert!(validate_flash_fixture(&installed, &restored).is_ok());
        restored[0x9cfff] = 0;
        assert!(validate_flash_fixture(&installed, &restored).is_err());
        assert!(validate_flash_fixture(&installed[..100], &restored).is_err());
    }

    #[test]
    fn failed_trace_flush_is_an_error_not_a_success_receipt() {
        struct CannotFlush;
        impl Write for CannotFlush {
            fn write(&mut self, data: &[u8]) -> io::Result<usize> { Ok(data.len()) }
            fn flush(&mut self) -> io::Result<()> { Err(io::Error::new(io::ErrorKind::Other, "test sink failed")) }
        }
        let mut observer = Observer::new(&map(), 8192, 5_760_000_000, Box::new(CannotFlush)).unwrap();
        assert!(observer.finish(123).is_err());
    }

    #[test]
    fn actual_bus_hooks_capture_byte_plan_and_halfword_stores() {
        use fm1_core::Bus;
        let (mut observer, _, capture) = fixture();
        let (plan, cost) = (address(&observer, "eng_plan"), address(&observer, "eng_job_cost"));
        let mut soc = crate::Soc::new(8192, Vec::new());
        start(&mut observer, &soc.ram);
        soc.dnbox_observer = Some(observer);
        soc.write8(plan, 5);
        soc.write16(cost + 2, 0xabcd);
        let observer = soc.dnbox_observer.as_mut().unwrap();
        observer.after_block(&soc.ram, 1800, 0, 0);
        observer.finish(1800).unwrap();
        let text = output(&capture);
        assert!(text.contains("\"name\":\"eng_plan\",\"offset\":0,\"width\":1,\"value\":5"));
        assert!(text.contains("\"name\":\"eng_job_cost\",\"offset\":2,\"width\":2,\"value\":43981"));
        assert!(text.contains("\"eng_plan\":[5]"));
    }

    #[test]
    fn strict_dual_cpu_dma_and_irq_hooks_preserve_ram_pcm_and_clock() {
        use crate::{machine, Soc, VEC_BASE, XIP_BASE};
        use fm1_core::{Bus, Cpu};
        fn immediate(r: u8, value: u32) -> Vec<u8> {
            [vec![0xc0 | r, 0xff], value.to_le_bytes().to_vec()].concat()
        }
        fn store(base: u8, src: u8) -> [u8; 2] { (0x6080u16 | src as u16 | (base as u16) << 4).to_le_bytes() }
        fn setup(enabled: bool) -> (Cpu, Option<Cpu>, Soc, bool, Capture) {
            let (observer, _, capture) = fixture();
            let dbg = address(&observer, "x0x_dbg");
            let xruns = address(&observer, "audio_xruns");
            let prof = address(&observer, "eng_prof");
            // C0 spins until ALNK. Its handler spans multiple unchanged dual-core slices,
            // then stores last_us and final miss counters, acknowledges ALNK and executes RTI.
            let mut code = vec![0xf7, 0x9f];
            code.extend([0x20, 0x00].repeat(2200));
            for (at, value) in [(dbg + 32, 444), (dbg + 20, 7), (xruns, 7), (0x12e08, 8)] {
                code.extend(immediate(1, at)); code.extend(immediate(2, value)); code.extend(store(1, 2));
            }
            code.extend([0x81, 0x00]); // RTI
            let core1_entry = XIP_BASE + code.len() as u32;
            code.extend(immediate(1, prof));
            code.extend(immediate(2, 0));
            code.extend(immediate(6, core1_entry + 18));
            code.extend([0xc2, 0x21]); // r2 += 1
            code.extend(store(1, 2));
            code.extend([0xd6, 0x00]); // goto r6
            let mut soc = Soc::new(1 << 20, code);
            soc.loop_skip.enabled = false;
            soc.strict_align = true;
            soc.xip_cache = Some(crate::XipCache::default());
            soc.periph.cycle_ticks = 16;
            soc.periph.hz = 5_760_000_000;
            let mut cpu = Cpu::new(XIP_BASE);
            cpu.cost_model = true; cpu.jit_on = false;
            cpu.ie = true; cpu.sreg_store[0] |= 0x100;
            soc.take_boot(&cpu);
            soc.write32(0x01c7_fff8, core1_entry);
            soc.write32(0x10008, 8);
            soc.write32(VEC_BASE + 4 * 11, XIP_BASE + 2);
            soc.periph.icfg[1] = 7 << 12; // IRQ11 enabled at priority3
            soc.ram.write32(0x20000, 123 << 8);
            soc.write32(0x12e1c, RAM_BASE + 0x20000);
            soc.write32(0x12e20, 1024);
            soc.write32(0x12e00, 0x800);
            if enabled { soc.dnbox_observer = Some(observer); }
            //512 frames *544 crystal ticks *240 strict model ticks per crystal tick.
            soc.advance(66_846_720);
            (cpu, None, soc, false, capture)
        }
        let (mut a, mut c1a, mut sa, mut ta, capture) = setup(true);
        let (mut b, mut c1b, mut sb, mut tb, _) = setup(false);
        for n in [1024, 1537, 2063, 1024] {
            machine::run_dual(&mut c1a, &mut sa, n, 1 << 20, false, |s, k| machine::run_blocks(&mut a, s, k, &mut ta)).unwrap();
            machine::run_dual(&mut c1b, &mut sb, n, 1 << 20, false, |s, k| machine::run_blocks(&mut b, s, k, &mut tb)).unwrap();
            assert_eq!((a.pc, a.regs, a.insn_count, a.plain_state(), ta), (b.pc, b.regs, b.insn_count, b.plain_state(), tb));
            assert_eq!((a.sp, a.ssp, a.usp, a.flags, a.ie, a.user, a.sreg_store, &a.irq_levels, a.nest_ok),
                (b.sp, b.ssp, b.usp, b.flags, b.ie, b.user, b.sreg_store, &b.irq_levels, b.nest_ok));
            let (ca, cb) = (c1a.as_ref().unwrap(), c1b.as_ref().unwrap());
            assert_eq!((ca.pc, ca.regs, ca.insn_count, ca.plain_state()), (cb.pc, cb.regs, cb.insn_count, cb.plain_state()));
            assert_eq!((sa.periph.cycles, sa.ticks, sa.core1_debt), (sb.periph.cycles, sb.ticks, sb.core1_debt));
            assert_eq!(sa.periph.audio, sb.periph.audio);
            assert_eq!(sa.xip_cache.as_ref().unwrap().misses, sb.xip_cache.as_ref().unwrap().misses);
            for word in (0..1 << 20).step_by(4) { assert_eq!(sa.ram.read32(word), sb.ram.read32(word)); }
        }
        let cycle = sa.periph.cycles;
        sa.dnbox_observer.as_mut().unwrap().finish(cycle).unwrap();
        assert_eq!(sa.periph.audio[0], 123);
        let text = output(&capture);
        assert!(text.contains("\"scheduled_cycle\":66846720"));
        assert!(text.contains("\"deadline_cycle\":133693440"));
        assert!(text.contains("\"core\":1,\"name\":\"eng_prof\""));
        assert!(text.contains("\"last_us\":444"));
        assert!(text.contains("\"audio_xruns\":[7]"));
        assert_eq!(text.lines().filter(|l| l.contains("\"event\":\"half\"")).count(), 1);
    }
}
