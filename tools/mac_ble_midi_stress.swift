// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 M. Geisler (genkigenki/makkuro)
//
// BLE MIDI stress test for the SMK-37 from a Mac, with macOS's own frameworks only (CoreBluetooth,
// CoreMIDI; no installs). One command runs everything:
//
//   swift tools/mac_ble_midi_stress.swift [stress] [--seconds 600] [--rounds 3] [--rate 800]
//         [--name SMK] [--usb X0X] [--out DIR] [--no-reset]
//   swift tools/mac_ble_midi_stress.swift scan     list BLE MIDI devices (10 s)
//   swift tools/mac_ble_midi_stress.swift midi     list CoreMIDI endpoints
//
// Each round (default 3 x 600 s):
//  - DIAG over USB MIDI (editor command 33) before and after: hx_rx, hx_tx, hx_tx_drops, c2h_drops,
//    heap peak and failed allocations, stack free (tasks / run / irq), queue high-water, link MTU.
//  - Dense traffic Mac -> keyboard: note on/off pairs plus APC40 pad LED colour messages (note on,
//    notes 0-39, velocity = colour), packed as many as fit in one write (up to 64 B or the link's
//    maximum). When the link has no room the sender waits for peripheralIsReady (back-pressure);
//    nothing is skipped, the backlog is reported.
//  - Keyboard -> Mac: every notification is parsed and counted (notes, pad/LED, CC, SysEx).
//  - Latency: once a second a Universal Device Inquiry (F0 7E 7F 06 01 F7); the firmware already
//    answers it over BLE (ble.c ble_sx_answer) and over USB (usb.c), so both round trips are timed.
//  - A disconnect in a round is counted and the tool reconnects at once (time to MIDI notify again).
// After the rounds (unless --no-reset): a software reset over USB (editor command 34 op 2) and the
// time until the Mac is connected and subscribed again over BLE.
// Writes DIR/summary.json and DIR/result.txt (PASS / FAIL with reasons); default DIR
// ~/Documents/x0x37/stress-<date>. Needs Bluetooth permission for the app that runs it.
import CoreBluetooth
import CoreMIDI
import Foundation

let midiService = CBUUID(string: "03B80E5A-EDE8-4B33-A751-6CE34EC4C700")
let midiChar = CBUUID(string: "7772E5DB-3868-4112-A1A9-F2669D106BF3")
var args = Array(CommandLine.arguments.dropFirst())
func opt(_ k: String, _ d: String) -> String {
    if let i = args.firstIndex(of: k), i + 1 < args.count { let v = args[i + 1]; args.removeSubrange(i...i + 1); return v }
    return d
}
func flag(_ k: String) -> Bool { if let i = args.firstIndex(of: k) { args.remove(at: i); return true }; return false }
let seconds = Double(opt("--seconds", "600")) ?? 600
let rounds = Int(opt("--rounds", "3")) ?? 3
let rate = Double(opt("--rate", "800")) ?? 800            // messages per second, Mac -> keyboard
let wantName = opt("--name", "")
let connectTimeout = Double(opt("--connect-timeout", "30")) ?? 30   // a55: seconds per connect before it is called stalled
let usbKeys = opt("--usb", "X0X,SMK,FM-1,Felucca").split(separator: ",").map(String.init)
let doReset = !flag("--no-reset")
let stamp: String = { let f = DateFormatter(); f.dateFormat = "yyyyMMdd-HHmmss"; return f.string(from: Date()) }()
let outDir = opt("--out", NSHomeDirectory() + "/Documents/x0x37/stress-" + stamp)
let mode = args.first ?? "stress"

func now() -> Double { ProcessInfo.processInfo.systemUptime }
func log(_ s: String) { print(s); fflush(stdout) }
func emit(_ d: [String: Any]) {
    if let j = try? JSONSerialization.data(withJSONObject: d, options: [.sortedKeys]), let s = String(data: j, encoding: .utf8) { log(s) }
}
func pct(_ a: [Double], _ p: Double) -> Double {
    if a.isEmpty { return -1 }
    let s = a.sorted(); return s[min(s.count - 1, Int(Double(s.count - 1) * p + 0.5))]
}
func name(_ o: MIDIObjectRef) -> String {
    var n: Unmanaged<CFString>?
    MIDIObjectGetStringProperty(o, kMIDIPropertyDisplayName, &n)
    return n?.takeRetainedValue() as String? ?? "?"
}

// ---- USB MIDI: DIAG, device inquiry, reset ----
final class Usb {
    var client = MIDIClientRef(), inPort = MIDIPortRef(), outPort = MIDIPortRef()
    var src = MIDIEndpointRef(), dst = MIDIEndpointRef()
    var sx: [UInt8] = [], replies: [[UInt8]] = []
    let lock = NSLock()
    init() {
        MIDIClientCreateWithBlock("x0x-stress" as CFString, &client) { _ in }
        MIDIOutputPortCreate(client, "out" as CFString, &outPort)
        MIDIInputPortCreateWithBlock(client, "in" as CFString, &inPort) { [weak self] list, _ in self?.rx(list) }
    }
    func isUsb(_ e: MIDIEndpointRef) -> Bool {
        let n = name(e)
        var ent = MIDIEntityRef(), dev = MIDIDeviceRef(), own: Unmanaged<CFString>?
        MIDIEndpointGetEntity(e, &ent); MIDIEntityGetDevice(ent, &dev)
        MIDIObjectGetStringProperty(dev, kMIDIPropertyDriverOwner, &own)
        let drv = (own?.takeRetainedValue() as String?) ?? ""       // skip the system's Bluetooth MIDI copy
        return usbKeys.contains { n.contains($0) } && !drv.lowercased().contains("bluetooth") && !n.lowercased().contains("bluetooth")
    }
    @discardableResult func attach() -> Bool {
        if src != 0 { MIDIPortDisconnectSource(inPort, src) }
        src = 0; dst = 0
        for i in 0..<MIDIGetNumberOfSources() where isUsb(MIDIGetSource(i)) { src = MIDIGetSource(i); break }
        for i in 0..<MIDIGetNumberOfDestinations() where isUsb(MIDIGetDestination(i)) { dst = MIDIGetDestination(i); break }
        if src != 0 { MIDIPortConnectSource(inPort, src, nil) }
        return src != 0 && dst != 0
    }
    func rx(_ list: UnsafePointer<MIDIPacketList>) {
        var p = list.pointee.packet
        for _ in 0..<list.pointee.numPackets {
            let bytes = withUnsafeBytes(of: p.data) { Array($0.prefix(Int(p.length))) }
            lock.lock()
            for b in bytes {
                if b == 0xF0 { sx = [b] } else if !sx.isEmpty { sx.append(b); if b == 0xF7 { replies.append(sx); sx = [] } }
            }
            lock.unlock()
            p = MIDIPacketNext(&p).pointee
        }
    }
    func send(_ b: [UInt8]) {
        var buf = [UInt8](repeating: 0, count: 1024)
        buf.withUnsafeMutableBytes { raw in
            let pl = raw.baseAddress!.assumingMemoryBound(to: MIDIPacketList.self)
            var pk = MIDIPacketListInit(pl)
            pk = MIDIPacketListAdd(pl, 1024, pk, 0, b.count, b)
            MIDISend(outPort, dst, pl)
        }
    }
    /// one request, the reply whose first bytes match `want`; runs the run loop while it waits
    func ask(_ msg: [UInt8], want: [UInt8], timeout: Double = 3) -> [UInt8]? {
        for _ in 0..<2 {
            if dst == 0 && !attach() { return nil }
            lock.lock(); replies = []; lock.unlock()
            send(msg)
            let t = now()
            while now() - t < timeout {
                RunLoop.main.run(until: Date().addingTimeInterval(0.01))
                lock.lock(); let r = replies.first { Array($0.prefix(want.count)) == want }; lock.unlock()
                if let r = r { return r }
            }
            attach()
        }
        return nil
    }
    static let hdr: [UInt8] = [0xF0, 0x7D, 0x46, 0x4C]
    static let live = ["img_boot_up_dead", "want_host_blocked_c1on", "c1_gave_up", "plat_state", "mb_state_now",
        "mb_err_now", "polls", "poll_max", "h_state", "c1_trace", "reset_now", "h2c", "c2h", "c2h_drops", "h_step",
        "ctl_took", "ctl_gave", "rx0", "rx1", "alive_ms", "alive_polls", "rails0", "rails1", "hx_state", "hx_int_mtu",
        "hx_links", "hx_pairs", "hx_heap", "hx_stacks", "hx_nv", "hx_rx", "hx_tx", "hx_tx_drops",
        "d_state_fr", "d_send_sx", "d_app_flags", "d_mirror_n",
        "d_lat_to", "d_disc0", "d_disc1", "d_disc2", "d_disc3", "d_evlog", "d_smlog", "d_updates",
        "d_oq_drops", "d_drops2"]
    func diag() -> [String: Any]? {
        var w: [UInt32] = []
        while true {
            let req = Usb.hdr + [33] + (w.isEmpty ? [] : [UInt8(w.count)]) + [0xF7]
            guard let r = ask(req, want: Usb.hdr + [33]) else { if w.isEmpty { return nil }; break }
            let d = Array(r.dropFirst(5).dropLast()); guard let n0 = d.first else { break }
            let n = Int(n0)
            for k in 0..<n where 5 * k + 5 < d.count {
                let b = d[(1 + 5 * k)...(5 + 5 * k)].map(UInt32.init)
                w.append(b[0] | b[1] << 7 | b[2] << 14 | b[3] << 21 | (b[4] & 15) << 28)
            }
            if n < 70 || w.count > 127 { break }
        }
        guard w.count >= Usb.live.count else { return ["words": w.count, "error": "short DIAG (older firmware?)"] }
        var o: [String: Any] = [:]
        for (k, v) in zip(Usb.live, w.suffix(Usb.live.count)) { o[k] = v }
        let heap = o["hx_heap"] as! UInt32, st = o["hx_stacks"] as! UInt32, im = o["hx_int_mtu"] as! UInt32
        let states = ["NO IMAGE", "OFF", "RESTART TO TURN ON", "STARTING", "READY", "CONNECTED", "FAILED", "BAD IMAGE"]
        let ps = Int(o["plat_state"] as! UInt32)
        o["state"] = ps < states.count ? states[ps] : "\(ps)"
        o["heap_peak"] = heap & 0xFFFF; o["heap_failed_allocs"] = heap >> 16 & 0xFF
        o["stack_free_tasks"] = (st & 0xFF) * 16; o["stack_free_run"] = (st >> 8 & 0xFF) * 16
        o["stack_free_irq"] = (st >> 16 & 0xFF) * 16; o["queue_high_water"] = st >> 24
        o["conn_interval"] = im & 0xFFFF; o["mtu"] = im >> 16
        o["midi_in_drops"] = (o["rx1"] as! UInt32) & 0xFFFF   // ble_h_link1 low half (raw rx1 kept too)
        return o
    }
    func inquiryMs() -> Double? {
        let t = now()
        return ask([0xF0, 0x7E, 0x7F, 0x06, 0x01, 0xF7], want: [0xF0, 0x7E], timeout: 1).map { _ in (now() - t) * 1000 }
    }
    func reset() { send(Usb.hdr + [34, 2, 0xF7]) }
}

func u(_ d: [String: Any]?, _ k: String) -> Int { Int((d?[k] as? UInt32) ?? 0) }

// ---- BLE ----
final class Tester: NSObject, CBCentralManagerDelegate, CBPeripheralDelegate {
    var cm: CBCentralManager!
    let usb = Usb()
    var p: CBPeripheral?, ch: CBCharacteristic?
    var seen = Set<UUID>()
    var onReady: (() -> Void)?
    var tLink = 0.0
    // round state
    var round = 0, running = false, inRound = false
    var q: [[UInt8]] = [], qMax = 0, genAcc = 0.0, tGen = 0.0, noteI = 0, padI = 0
    var writes = 0, msgsSent = 0, bytesSent = 0, waits = 0, maxPayload = 0
    var rxPk = 0, rxB = 0, rxNotes = 0, rxPads = 0, rxCC = 0, rxSx = 0, rxOther = 0, rxSxBuf: [UInt8] = []
    var gapMax = 0.0, lastRx = 0.0
    var disc: [[String: Any]] = [], tDisc = 0.0
    var inqSent: Double = 0, inqOut = 0, inqLost = 0, pendingInq = false, bleRtt: [Double] = [], usbRtt: [Double] = []
    var results: [[String: Any]] = []
    var genTimer: Timer?, inqTimer: Timer?
    // a55: connect step tracing + watchdog (the a52 run hung silently after the connect)
    var step = "idle", tStep = 0.0, linkGen = 0, steps: [[String: Any]] = []
    func mark(_ s: String) {
        let t = now(); step = s; tStep = t
        let ms = Int((t - tLink) * 1000)
        steps.append(["step": s, "ms": ms]); log("[link] +\(ms) ms \(s)")
    }
    func arm() {
        linkGen += 1; let g = linkGen
        DispatchQueue.main.asyncAfter(deadline: .now() + connectTimeout) {
            guard g == self.linkGen, self.step != "notify on" else { return }
            let d = self.usb.diag()
            log("[link] TIMEOUT: stuck at '\(self.step)' for \(Int((now() - self.tStep) * 1000)) ms; USB DIAG: \(d.map { "\($0)" } ?? "n/a")")
            if let k = self.p { self.cm.cancelPeripheralConnection(k) }   // drop the link so the keyboard can advertise again
            self.finish(fatal: "connect stalled at '\(self.step)' after \(Int(connectTimeout)) s")
        }
    }

    override init() { super.init(); cm = CBCentralManager(delegate: self, queue: nil) }

    func centralManagerDidUpdateState(_ c: CBCentralManager) {
        if c.state == .unauthorized { finish(fatal: "Bluetooth permission missing: allow it for this app in System Settings > Privacy & Security > Bluetooth"); return }
        guard c.state == .poweredOn else { if c.state != .unknown && c.state != .resetting { finish(fatal: "Bluetooth state \(c.state.rawValue)") }; return }
        if mode == "scan" {
            c.scanForPeripherals(withServices: [midiService], options: nil)
            DispatchQueue.main.asyncAfter(deadline: .now() + 10) { exit(self.seen.isEmpty ? 1 : 0) }
            return
        }
        if !usb.attach() { log("warning: no USB MIDI port of the keyboard; DIAG and the USB latency are skipped") }
        connect { self.startRound() }
    }
    func connect(_ then: @escaping () -> Void) {
        onReady = then; tLink = now(); steps = []; mark("connect requested"); arm()
        if let k = p { cm.connect(k); return }
        if let k = cm.retrieveConnectedPeripherals(withServices: [midiService]).first(where: { wantName.isEmpty || ($0.name ?? "").contains(wantName) }) {
            p = k; k.delegate = self; cm.connect(k); return
        }
        cm.scanForPeripherals(withServices: [midiService], options: nil)
        DispatchQueue.main.asyncAfter(deadline: .now() + 60) { if self.ch == nil && self.p == nil { self.finish(fatal: "keyboard not found over BLE in 60 s (not advertising?)") } }
    }
    func centralManager(_ c: CBCentralManager, didDiscover per: CBPeripheral, advertisementData ad: [String: Any], rssi: NSNumber) {
        let nm = per.name ?? (ad[CBAdvertisementDataLocalNameKey] as? String) ?? "?"
        if mode == "scan" { if seen.insert(per.identifier).inserted { emit(["name": nm, "id": per.identifier.uuidString, "rssi": rssi]) }; return }
        if !wantName.isEmpty && !nm.contains(wantName) { return }
        c.stopScan(); p = per; per.delegate = self; mark("found \(nm) rssi \(rssi)"); c.connect(per)
    }
    func centralManager(_ c: CBCentralManager, didConnect per: CBPeripheral) { mark("connected (LL up)"); per.discoverServices([midiService]); mark("discovering services") }
    func centralManager(_ c: CBCentralManager, didFailToConnect per: CBPeripheral, error: Error?) {
        mark("connect failed"); log("connect failed: \(error?.localizedDescription ?? "?"); retrying"); c.connect(per)
    }
    func peripheral(_ per: CBPeripheral, didDiscoverServices e: Error?) {
        mark("services \(per.services?.count ?? 0) err \(e?.localizedDescription ?? "none")")
        guard let s = per.services?.first(where: { $0.uuid == midiService }) else { finish(fatal: "no MIDI service"); return }
        per.discoverCharacteristics([midiChar], for: s); mark("discovering characteristic")
    }
    func peripheral(_ per: CBPeripheral, didDiscoverCharacteristicsFor s: CBService, error e: Error?) {
        mark("characteristics \(s.characteristics?.count ?? 0) err \(e?.localizedDescription ?? "none")")
        guard let c = s.characteristics?.first(where: { $0.uuid == midiChar }) else { finish(fatal: "no MIDI I/O characteristic"); return }
        ch = c; per.setNotifyValue(true, for: c); mark("enabling notify (CCCD write)")
    }
    func peripheral(_ per: CBPeripheral, didUpdateNotificationStateFor c: CBCharacteristic, error e: Error?) {
        if e == nil && c.isNotifying { mark("notify on") } else { mark("notify failed \(e?.localizedDescription ?? "?")") }
        guard e == nil, c.isNotifying else { finish(fatal: "subscribe failed: \(e?.localizedDescription ?? "?")"); return }
        let ms = Int((now() - tLink) * 1000)
        if inRound, tDisc > 0 {                       // reconnected inside a round
            disc[disc.count - 1]["reconnect_ms"] = Int((now() - tDisc) * 1000); tDisc = 0; pump(); return
        }
        log("BLE MIDI ready in \(ms) ms (max write \(per.maximumWriteValueLength(for: .withoutResponse)) B)")
        let f = onReady; onReady = nil; lastLinkMs = ms; f?()
    }
    var lastLinkMs = 0
    func centralManager(_ c: CBCentralManager, didDisconnectPeripheral per: CBPeripheral, error e: Error?) {
        ch = nil; mark("disconnected: \(e?.localizedDescription ?? "no error")")
        if resetting { arm(); c.connect(per); return }
        if inRound {
            tDisc = now()
            disc.append(["at_s": Int(now() - tGen), "reason": e?.localizedDescription ?? "?"])
            log("round \(round): disconnect (\(e?.localizedDescription ?? "?")); reconnecting")
            arm(); c.connect(per)
        }
    }

    // ---- a round ----
    func startRound() {
        round += 1
        if round > rounds { return doReset ? resetTest() : finish(fatal: nil) }
        let before = usb.diag()
        if let u0 = usb.inquiryMs() { usbRtt.append(u0) }
        q = []; qMax = 0; writes = 0; msgsSent = 0; bytesSent = 0; waits = 0; maxPayload = 0
        rxPk = 0; rxB = 0; rxNotes = 0; rxPads = 0; rxCC = 0; rxSx = 0; rxOther = 0; gapMax = 0
        disc = []; tDisc = 0; inqOut = 0; inqLost = 0; bleRtt = []; usbRtt = []
        tGen = now(); lastRx = tGen; genAcc = 0; inRound = true
        log("round \(round)/\(rounds): \(Int(seconds)) s at \(Int(rate)) messages/s")
        genTimer = Timer.scheduledTimer(withTimeInterval: 0.002, repeats: true) { _ in self.generate() }
        inqTimer = Timer.scheduledTimer(withTimeInterval: 1.0, repeats: true) { _ in self.inquiry() }
        DispatchQueue.main.asyncAfter(deadline: .now() + seconds) { self.endRound(before) }
    }
    var lastGen = 0.0
    func generate() {
        let t = now(); if lastGen == 0 { lastGen = t }
        genAcc += (t - lastGen) * rate; lastGen = t
        while genAcc >= 1 {
            genAcc -= 1
            let k = msgsQueued
            if k % 4 == 3 {                           // every 4th: an APC40 pad LED colour (note on, notes 0-39)
                q.append([0x90, UInt8(padI % 40), UInt8((padI * 7 + 1) % 128)]); padI += 1
            } else if k % 4 == 0 || k % 4 == 1 {
                q.append([0x90, UInt8(48 + noteI % 37), 100])
            } else {
                q.append([0x80, UInt8(48 + noteI % 37), 0]); noteI += 1
            }
            msgsQueued += 1
        }
        qMax = max(qMax, q.count)
        pump()
    }
    var msgsQueued = 0
    func ts() -> (UInt8, UInt8) { let t = UInt16(Int(Date().timeIntervalSince1970 * 1000) & 0x1FFF); return (UInt8(0x80 | (t >> 7)), UInt8(0x80 | (t & 0x7F))) }
    /// pack as many queued messages as fit in one write; when the link is busy, wait (peripheralIsReady)
    func pump() {
        guard let p = p, let ch = ch, p.state == .connected else { return }
        if pendingInq { _ = sendInquiry() }   // a60: probe goes first, but notes still flow if the link is momentarily full
        let cap = min(64, p.maximumWriteValueLength(for: .withoutResponse))
        while !q.isEmpty {
            guard p.canSendWriteWithoutResponse else { waits += 1; return }
            let (h, l) = ts(); var pk: [UInt8] = [h]; var n = 0
            while let m = q.first, pk.count + 1 + m.count <= cap { pk.append(l); pk += m; q.removeFirst(); n += 1 }
            if n == 0 { break }
            p.writeValue(Data(pk), for: ch, type: .withoutResponse)
            writes += 1; msgsSent += n; bytesSent += pk.count; maxPayload = max(maxPayload, pk.count)
        }
    }
    func peripheralIsReady(toSendWriteWithoutResponse per: CBPeripheral) { pump() }
    func inquiry() {
        if inqOut > 0 { inqLost += 1 }
        pendingInq = true          // a60: pump() sends it with priority, before the note backlog
        pump()
    }
    // a60: send the latency probe ahead of queued notes so the flood can't starve it
    func sendInquiry() -> Bool {
        guard let p = p, let ch = ch, p.state == .connected, p.canSendWriteWithoutResponse else { return false }
        let (h, l) = ts()
        p.writeValue(Data([h, l, 0xF0, 0x7E, 0x7F, 0x06, 0x01, l, 0xF7]), for: ch, type: .withoutResponse)  // a60: ts byte before F0 (BLE-MIDI)
        inqSent = now(); inqOut = 1; pendingInq = false
        return true
    }
    func peripheral(_ per: CBPeripheral, didUpdateValueFor c: CBCharacteristic, error e: Error?) {
        guard let v = c.value, v.count > 1 else { return }
        let t = now(); rxPk += 1; rxB += v.count
        if rxPk > 1 { gapMax = max(gapMax, t - lastRx) }; lastRx = t
        // BLE MIDI: header, then (timestamp, message) or SysEx continuation bytes
        let b = [UInt8](v); var i = 1; var status: UInt8 = 0
        while i < b.count {
            let x = b[i]
            if !rxSxBuf.isEmpty {                     // inside a SysEx: data until a timestamp + F7
                if x & 0x80 != 0 { if i + 1 < b.count && b[i + 1] == 0xF7 { sysexDone(); i += 2; continue }; if x == 0xF7 { sysexDone(); i += 1; continue }; i += 1; continue }
                rxSxBuf.append(x); i += 1; continue
            }
            if x & 0x80 != 0 && i + 1 < b.count && b[i + 1] & 0x80 != 0 { i += 1; status = b[i] }   // timestamp, status
            else if x & 0x80 != 0 { i += 1; continue }                                              // lone timestamp
            if status == 0xF0 { rxSxBuf = [0xF0]; i += 1; continue }
            let len = (status & 0xF0 == 0xC0 || status & 0xF0 == 0xD0) ? 1 : (status >= 0xF8 ? 0 : 2)
            let hi = status & 0xF0
            if hi == 0x90 || hi == 0x80 { if i + 1 < b.count && b[i + 1] < 40 && status & 0x0F <= 8 { rxPads += 1 } else { rxNotes += 1 } }
            else if hi == 0xB0 { rxCC += 1 } else { rxOther += 1 }
            i += 1 + len
            while i + len <= b.count - 1 && b[i] & 0x80 == 0 { i += len }                       // running status
        }
    }
    func sysexDone() {
        rxSx += 1
        if rxSxBuf.count >= 5 && rxSxBuf[1] == 0x7E && rxSxBuf[3] == 0x06 && rxSxBuf[4] == 0x02 && inqOut > 0 {
            bleRtt.append((now() - inqSent) * 1000); inqOut = 0
        }
        rxSxBuf = []
    }
    func endRound(_ before: [String: Any]?) {
        genTimer?.invalidate(); inqTimer?.invalidate(); lastGen = 0
        let drainT = now()
        func done() {
            inRound = false
            if let u1 = usb.inquiryMs() { usbRtt.append(u1) }
            let after = usb.diag()
            var r: [String: Any] = [
                "round": round, "seconds": seconds, "rate_msgs_per_s": rate,
                "msgs_sent": msgsSent, "writes": writes, "bytes_sent": bytesSent, "max_payload": maxPayload,
                "avg_msgs_per_write": writes > 0 ? Double(msgsSent) / Double(writes) : 0,
                "busy_waits": waits, "backlog_max": qMax, "backlog_left": q.count, "drain_ms": Int((now() - drainT) * 1000),
                "rx_packets": rxPk, "rx_bytes": rxB, "rx_notes": rxNotes, "rx_pad_led": rxPads, "rx_cc": rxCC, "rx_sysex": rxSx,
                "rx_other": rxOther, "rx_max_gap_ms": Int(gapMax * 1000),
                "disconnects": disc, "inquiries_lost": inqLost,
                "ble_rtt_ms": ["n": bleRtt.count, "p50": pct(bleRtt, 0.5), "p95": pct(bleRtt, 0.95), "max": bleRtt.max() ?? -1],
                "usb_rtt_ms": ["n": usbRtt.count, "p50": pct(usbRtt, 0.5), "max": usbRtt.max() ?? -1],
                "diag_before": before ?? [:], "diag_after": after ?? [:]]
            var fails: [String] = []
            if before == nil || after == nil { fails.append("no DIAG over USB") }
            let dRx = u(after, "hx_rx") - u(before, "hx_rx")
            r["device_rx_delta"] = dRx
            r["device_tx_delta"] = u(after, "hx_tx") - u(before, "hx_tx")
            if before != nil && after != nil {
                if dRx < writes { fails.append("keyboard got \(dRx) of \(writes) writes") }
                for k in ["hx_tx_drops", "c2h_drops", "heap_failed_allocs", "midi_in_drops"] where u(after, k) > u(before, k) {
                    fails.append("\(k) +\(u(after, k) - u(before, k))")
                }
                if u(after, "reset_now") != u(before, "reset_now") { fails.append("the keyboard restarted") }
                if (after?["state"] as? String) != "CONNECTED" && (after?["state"] as? String) != "READY" { fails.append("state \(after?["state"] ?? "?")") }
            }
            if !disc.isEmpty { fails.append("\(disc.count) disconnect(s)") }
            if !q.isEmpty { fails.append("\(q.count) messages not sent") }
            if inqLost > 1 { fails.append("\(inqLost) latency probes unanswered") }
            r["pass"] = fails.isEmpty; r["fails"] = fails
            emit(r); results.append(r)
            DispatchQueue.main.asyncAfter(deadline: .now() + 1) { self.startRound() }
        }
        func wait() { if q.isEmpty || now() - drainT > 10 { done() } else { pump(); DispatchQueue.main.asyncAfter(deadline: .now() + 0.05, execute: wait) } }
        wait()
    }

    // ---- reconnect after a software reset ----
    var resetResult: [String: Any] = [:], resetting = false
    func resetTest() {
        guard usb.dst != 0 || usb.attach() else { resetResult = ["skipped": "no USB MIDI port"]; return finish(fatal: nil) }
        log("reset test: software reset over USB, then waiting for the BLE link")
        let t0 = now(); var linkDown = 0.0
        resetting = true
        usb.reset()
        func poll() {
            if linkDown == 0, p?.state != .connected { linkDown = now() }
            if now() - t0 > 90 { resetResult = ["pass": false, "error": "not reconnected in 90 s"]; return finish(fatal: nil) }
            if linkDown > 0 && ch != nil {
                let ms = Int((now() - t0) * 1000)
                Thread.sleep(forTimeInterval: 2); usb.attach()
                let d = usb.diag()
                resetResult = ["pass": ms < 30000, "reconnect_ms": ms, "link_lost_after_ms": Int((linkDown - t0) * 1000),
                               "diag_after": d ?? [:]]
                emit(["reset_test": resetResult]); return finish(fatal: nil)
            }
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.05, execute: poll)
        }
        onReady = {}                                  // the reconnect lands in didUpdateNotificationState
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.05, execute: poll)
    }
    // keep reconnecting after the reset (macOS keeps a pending connect with no timeout)
    func finish(fatal: String?) {
        var fails: [String] = []
        if let f = fatal { fails.append(f) }
        for r in results where (r["pass"] as? Bool) != true { fails.append("round \(r["round"]!): " + ((r["fails"] as? [String]) ?? []).joined(separator: "; ")) }
        if doReset && fatal == nil && (resetResult["pass"] as? Bool) != true && resetResult["skipped"] == nil { fails.append("reset test: \(resetResult["error"] ?? "reconnect too slow")") }
        if results.count < rounds && fatal == nil { fails.append("only \(results.count) of \(rounds) rounds ran") }
        let pass = fails.isEmpty
        let sum: [String: Any] = ["pass": pass, "fails": fails, "rounds": results, "reset_test": resetResult,
                                  "settings": ["seconds": seconds, "rounds": rounds, "rate": rate, "name": wantName, "reset": doReset],
                                  "first_link_ms": lastLinkMs, "link_steps": steps, "date": stamp]
        try? FileManager.default.createDirectory(atPath: outDir, withIntermediateDirectories: true)
        if let j = try? JSONSerialization.data(withJSONObject: sum, options: [.sortedKeys, .prettyPrinted]) {
            try? j.write(to: URL(fileURLWithPath: outDir + "/summary.json"))
        }
        var txt = pass ? "PASS\n" : "FAIL\n"
        for f in fails { txt += "- \(f)\n" }
        for r in results {
            let b = r["ble_rtt_ms"] as! [String: Any]
            txt += "round \(r["round"]!): \(r["msgs_sent"]!) messages in \(r["writes"]!) writes (max \(r["max_payload"]!) B), "
                + "keyboard got \(r["device_rx_delta"]!), disconnects \((r["disconnects"] as! [Any]).count), "
                + "BLE round trip p50 \(b["p50"]!) ms p95 \(b["p95"]!) ms, \(((r["pass"] as? Bool) == true) ? "pass" : "FAIL")\n"
        }
        if let ms = resetResult["reconnect_ms"] { txt += "reconnect after a reset: \(ms) ms\n" }
        try? txt.write(toFile: outDir + "/result.txt", atomically: true, encoding: .utf8)
        log(txt + "files: \(outDir)/summary.json, result.txt")
        exit(pass ? 0 : 1)
    }
}

if mode == "midi" {
    for i in 0..<MIDIGetNumberOfSources() { print("source:", name(MIDIGetSource(i))) }
    for i in 0..<MIDIGetNumberOfDestinations() { print("destination:", name(MIDIGetDestination(i))) }
    exit(0)
}
let tester = Tester()
RunLoop.main.run()
