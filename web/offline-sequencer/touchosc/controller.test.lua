-- SPDX-License-Identifier: GPL-3.0-only
-- Run with Lua 5.1 on the Orchestrator's box; this session does not execute it.
local script = arg[1] or "web/offline-sequencer/touchosc/controller.lua"
local sent, now = {}, 0
local controls = {
  route = { values = {} }, feedback = { values = {} },
  note_60 = {}, note_61 = {}, note_36 = {}
}
local env = {
  root = {findByName = function(_, name) return controls[name] end},
  sendMIDI = function(message, connections)
    assert(connections[1] == true and connections[2] == false)
    table.insert(sent, message)
  end,
  Color = function(r, g, b, a) return {r=r, g=g, b=b, a=a} end,
  getMillis = function() return now end
}
setmetatable(env, {__index = _G})
local code = assert(loadfile(script))
setfenv(code, env)
code()
local function input(name, pressed)
  env.onReceiveNotify("input", {name=name, pressed=pressed})
end
local function expect(index, a, b, c)
  assert(sent[index] and sent[index][1] == a and sent[index][2] == b and sent[index][3] == c)
end
env.init()
input("note_60", true)
assert(#sent == 0, "disarmed layout must not send")
input("arm", true)
input("note_60", true)
input("note_60", true)
assert(#sent == 1, "repeated held callback must not retrigger")
expect(1, 0x91, 60, 100) -- UI channel 2 uses MIDI status 0x91.
input("note_60", false)
expect(2, 0x81, 60, 0)
env.onReceiveMIDI({0x91, 60, 95}, {true})
assert(#sent == 2, "incoming feedback must never echo")
assert(controls.feedback.values.text == "Received CH 2 note 60 velocity 95")
local before = controls.feedback.values.text
env.onReceiveMIDI({0x91, 60, 95}, {false, true})
assert(controls.feedback.values.text == before, "other connections are ignored")
env.onReceiveMIDI({0x91, 60, 300}, {true})
assert(controls.feedback.values.text == before, "invalid bytes are rejected")
input("note_61", true)
expect(3, 0x91, 61, 100)
input("channel_up", true)
expect(4, 0x81, 61, 0) -- release uses the original channel.
input("note_61", false)
input("note_60", true)
assert(#sent == 4, "changing channel disarms and stale release sends nothing")
input("arm", true)
input("note_36", true)
expect(5, 0x92, 36, 100)
input("disarm", true)
expect(6, 0x82, 36, 0)
input("arm", true)
input("note_60", true)
now = 11000
env.update()
expect(8, 0x82, 60, 0)
input("note_60", true)
assert(#sent == 8, "hold timeout disarms before another press")
input("unrecognized", true)
input("note_999", true)
assert(#sent == 8, "unknown controls never emit arbitrary MIDI")
print("TouchOSC root controller: PASS")
