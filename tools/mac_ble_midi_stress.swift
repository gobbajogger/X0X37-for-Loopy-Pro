// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 M. Geisler (genkigenki/makkuro)
//
// BLE MIDI scan / connect / stress test for the SMK-37 from a Mac, with macOS's own tools only
// (CoreBluetooth, CoreMIDI; no installs):
//
//   swift tools/mac_ble_midi_stress.swift scan                 list BLE MIDI devices (10 s)
//   swift tools/mac_ble_midi_stress.swift stress [SECONDS] [NAME] [--rounds N] [--rate HZ]
//       connect, subscribe to MIDI I/O, send note on/off pairs at HZ (default 200) for SECONDS
//       (default 60), count every notification the keyboard sends back, then disconnect; N rounds
//       (default 3) test reconnects. Prints one JSON line per round: connect time, sent, write
//       errors, received packets/bytes, gaps > 50 ms, disconnects (with the reason).
//   swift tools/mac_ble_midi_stress.swift midi                 list CoreMIDI endpoints (does the
//       system's Bluetooth MIDI driver show it?)
// Needs Bluetooth permission for Terminal (System Settings > Privacy & Security > Bluetooth).
import CoreBluetooth
import CoreMIDI
import Foundation

let midiService = CBUUID(string: "03B80E5A-EDE8-4B33-A751-6CE34EC4C700")
let midiChar = CBUUID(string: "7772E5DB-3868-4112-A1A9-F2669D106BF3")
var args = Array(CommandLine.arguments.dropFirst())
func opt(_ k: String, _ d: Double) -> Double {
    if let i = args.firstIndex(of: k), i + 1 < args.count, let v = Double(args[i + 1]) { args.removeSubrange(i...i + 1); return v }
    return d
}
let rounds = Int(opt("--rounds", 3)), rate = opt("--rate", 200)
let mode = args.first ?? "scan"
let seconds = args.count > 1 ? Double(args[1]) ?? 60 : 60
let wantName = args.count > 2 ? args[2] : ""

func now() -> Double { Date().timeIntervalSince1970 }
func emit(_ d: [String: Any]) {
    if let j = try? JSONSerialization.data(withJSONObject: d, options: [.sortedKeys]), let s = String(data: j, encoding: .utf8) { print(s); fflush(stdout) }
}

final class Tester: NSObject, CBCentralManagerDelegate, CBPeripheralDelegate {
    var cm: CBCentralManager!
    var p: CBPeripheral?, ch: CBCharacteristic?
    var seen = Set<UUID>()
    var round = 0, sent = 0, werr = 0, rxPk = 0, rxB = 0, gaps = 0, disc = 0
    var tConnect = 0.0, tStart = 0.0, lastRx = 0.0, maxGap = 0.0, reason = ""
    var timer: Timer?
    override init() { super.init(); cm = CBCentralManager(delegate: self, queue: nil) }

    func centralManagerDidUpdateState(_ c: CBCentralManager) {
        guard c.state == .poweredOn else { if c.state != .unknown && c.state != .resetting { emit(["error": "bluetooth state \(c.state.rawValue)"]); exit(2) }; return }
        if mode == "scan" {
            c.scanForPeripherals(withServices: [midiService], options: [CBCentralManagerScanOptionAllowDuplicatesKey: false])
            DispatchQueue.main.asyncAfter(deadline: .now() + 10) { exit(self.seen.isEmpty ? 1 : 0) }
        } else { next() }
    }
    func next() {
        round += 1
        if round > rounds { exit(0) }
        sent = 0; werr = 0; rxPk = 0; rxB = 0; gaps = 0; maxGap = 0; reason = ""
        tConnect = now()
        // a device the system already connected (macOS Bluetooth MIDI) does not advertise: try those first
        if let k = cm.retrieveConnectedPeripherals(withServices: [midiService]).first(where: { wantName.isEmpty || ($0.name ?? "").contains(wantName) }) {
            p = k; k.delegate = self; cm.connect(k); return
        }
        cm.scanForPeripherals(withServices: [midiService])
        DispatchQueue.main.asyncAfter(deadline: .now() + 30) { [r = round] in
            if self.round == r && self.ch == nil { emit(["round": r, "error": "not found or not connected in 30 s"]); self.cm.stopScan(); self.next() }
        }
    }
    func centralManager(_ c: CBCentralManager, didDiscover per: CBPeripheral, advertisementData ad: [String: Any], rssi: NSNumber) {
        let name = per.name ?? (ad[CBAdvertisementDataLocalNameKey] as? String) ?? "?"
        if mode == "scan" {
            if seen.insert(per.identifier).inserted { emit(["name": name, "id": per.identifier.uuidString, "rssi": rssi]) }
            return
        }
        if !wantName.isEmpty && !name.contains(wantName) { return }
        c.stopScan(); p = per; per.delegate = self; c.connect(per)
    }
    func centralManager(_ c: CBCentralManager, didConnect per: CBPeripheral) { per.discoverServices([midiService]) }
    func centralManager(_ c: CBCentralManager, didFailToConnect per: CBPeripheral, error: Error?) {
        emit(["round": round, "error": "connect failed: \(error?.localizedDescription ?? "?")"]); next()
    }
    func peripheral(_ per: CBPeripheral, didDiscoverServices e: Error?) {
        guard let s = per.services?.first(where: { $0.uuid == midiService }) else { emit(["round": round, "error": "no MIDI service"]); return }
        per.discoverCharacteristics([midiChar], for: s)
    }
    func peripheral(_ per: CBPeripheral, didDiscoverCharacteristicsFor s: CBService, error e: Error?) {
        guard let c = s.characteristics?.first(where: { $0.uuid == midiChar }) else { emit(["round": round, "error": "no MIDI I/O"]); return }
        ch = c; per.setNotifyValue(true, for: c)
    }
    func peripheral(_ per: CBPeripheral, didUpdateNotificationStateFor c: CBCharacteristic, error e: Error?) {
        let connectMs = Int((now() - tConnect) * 1000)
        tStart = now(); lastRx = tStart
        var n: UInt8 = 36
        timer = Timer.scheduledTimer(withTimeInterval: 1.0 / rate, repeats: true) { _ in
            guard let ch = self.ch, let p = self.p, p.state == .connected else { return }
            let ts = UInt16(Int(now() * 1000) & 0x1FFF)
            let hdr = UInt8(0x80 | (ts >> 7)), tsl = UInt8(0x80 | (ts & 0x7F))
            let on = self.sent % 2 == 0
            let msg: [UInt8] = [hdr, tsl, on ? 0x90 : 0x80, n, on ? 100 : 0]
            if !on { n = n >= 72 ? 36 : n + 1 }
            if p.canSendWriteWithoutResponse { p.writeValue(Data(msg), for: ch, type: .withoutResponse); self.sent += 1 } else { self.werr += 1 }
        }
        DispatchQueue.main.asyncAfter(deadline: .now() + seconds) { [r = round] in
            guard self.round == r else { return }
            self.timer?.invalidate()
            emit(["round": r, "connect_ms": connectMs, "sent": self.sent, "busy_skips": self.werr, "rx_packets": self.rxPk,
                  "rx_bytes": self.rxB, "gaps_over_50ms": self.gaps, "max_gap_ms": Int(self.maxGap * 1000),
                  "disconnects": self.disc, "reason": self.reason])
            self.ch = nil
            if let p = self.p { self.cm.cancelPeripheralConnection(p) }
            DispatchQueue.main.asyncAfter(deadline: .now() + 2) { self.next() }
        }
    }
    func peripheral(_ per: CBPeripheral, didUpdateValueFor c: CBCharacteristic, error e: Error?) {
        let t = now(); rxPk += 1; rxB += c.value?.count ?? 0
        if t - lastRx > 0.05 && rxPk > 1 { gaps += 1 }
        maxGap = max(maxGap, t - lastRx); lastRx = t
    }
    func centralManager(_ c: CBCentralManager, didDisconnectPeripheral per: CBPeripheral, error e: Error?) {
        if ch != nil { disc += 1; reason = e?.localizedDescription ?? "?"; emit(["round": round, "event": "disconnect", "reason": reason]) }
    }
}

if mode == "midi" {
    for i in 0..<MIDIGetNumberOfSources() { var n: Unmanaged<CFString>?; MIDIObjectGetStringProperty(MIDIGetSource(i), kMIDIPropertyDisplayName, &n); print("source:", n?.takeRetainedValue() as String? ?? "?") }
    for i in 0..<MIDIGetNumberOfDestinations() { var n: Unmanaged<CFString>?; MIDIObjectGetStringProperty(MIDIGetDestination(i), kMIDIPropertyDisplayName, &n); print("destination:", n?.takeRetainedValue() as String? ?? "?") }
    exit(0)
}
let tester = Tester()
RunLoop.main.run()
