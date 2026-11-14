-- SPDX-License-Identifier: GPL-3.0-only
-- Run with Lua 5.1 from the repository root.
local script = arg[1] or "web/offline-sequencer/touchosc/controller.lua"
local sent, now = {}, 0
local controls = {
  route = { values = {} }, feedback = { values = {} },
  note_60 = {}, note_61 = {}, note_36 = {}
}
for _, note in ipairs({36, 38, 42, 46, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71}) do
  controls["note_" .. note] = {values = {x = 0}}
end
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

-- Resetting only `active` leaves receive-only notes and old-channel text
-- visible. Each scenario exercises the real root callbacks at the API boundary.
local neutral = "No incoming MIDI observed"
local function fresh()
  sent, now = {}, 0
  code()
  env.init()
end
local function incoming(status, note, velocity)
  env.onReceiveMIDI({status, note, velocity}, {true})
end
local function isColour(note, on)
  local c = controls["note_" .. note].color
  assert(c and c.r == (on and 0.55 or 0.16) and c.g == (on and 0.90 or 0.25)
    and c.b == (on and 0.76 or 0.30) and c.a == 1,
    "unexpected highlight for note " .. note)
end
local function cleared()
  assert(controls.feedback.values.text == neutral, "feedback must start a new observation window")
  for _, note in ipairs({36, 38, 42, 46, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71}) do
    isColour(note, false)
    assert(controls["note_" .. note].values.x == 0, "feedback reset must not press a button")
  end
end
local failures, scenarios = 0, 0
local function scenario(name, run)
  fresh()
  scenarios = scenarios + 1
  local ok, err = pcall(run)
  if not ok then failures = failures + 1; io.stderr:write(name .. ": " .. tostring(err) .. "\n") end
end
scenario("channel change clears incoming notes and previous-channel text", function()
  incoming(0x91, 60, 99); incoming(0x91, 36, 70); incoming(0x91, 71, 80)
  isColour(60, true)
  input("channel_up", true)
  assert(controls.route.values.text == "DISARMED / MIDI CH 3 / connection 1")
  cleared()
  incoming(0x81, 60, 0)
  cleared() -- A late old-channel OFF is ignored.
  incoming(0x92, 60, 77)
  isColour(60, true)
  assert(controls.feedback.values.text == "Received CH 3 note 60 velocity 77")
  assert(#sent == 0, "receive-only notes must not be released over MIDI")
end)
scenario("RELEASE clears all incoming highlights without echo", function()
  for _, note in ipairs({36, 38, 42, 46, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71}) do
    incoming(0x91, note, 95)
  end
  input("disarm", true)
  cleared()
  input("disarm", true)
  assert(#sent == 0, "RELEASE must not emit OFFs for receive-only notes")
end)
scenario("timeout clears feedback and releases local notes exactly once", function()
  input("arm", true); input("note_61", true)
  incoming(0x91, 60, 95); incoming(0x91, 36, 95)
  now = 10000; env.update()
  cleared()
  assert(controls.route.values.text == "DISARMED / MIDI CH 2 / connection 1")
  expect(1, 0x91, 61, 100); expect(2, 0x81, 61, 0)
  env.update(); input("note_61", false)
  assert(#sent == 2)
end)
scenario("mixed local and incoming state releases on original channel", function()
  input("arm", true); input("note_60", true); input("note_61", true)
  incoming(0x91, 60, 99); incoming(0x91, 36, 99)
  input("channel_down", true)
  cleared()
  assert(#sent == 4)
  local released = {}
  for i=3,4 do
    assert(sent[i][1] == 0x81 and sent[i][3] == 0)
    released[sent[i][2]] = true
  end
  assert(released[60] and released[61], "each held note needs one original-channel OFF")
  input("note_60", false); input("note_61", false)
  assert(#sent == 4, "stale releases must not duplicate OFFs")
end)
scenario("clamped channel controls also reset the observation window", function()
  input("channel_down", true)
  incoming(0x90, 60, 95); input("channel_down", true)
  cleared()
  for _=1,15 do input("channel_up", true) end
  incoming(0x9F, 71, 95); input("channel_up", true)
  cleared()
  assert(controls.route.values.text == "DISARMED / MIDI CH 16 / connection 1")
  assert(#sent == 0)
end)
scenario("init clears presentation left by earlier incoming feedback", function()
  incoming(0x91, 60, 95)
  env.init()
  cleared()
  assert(#sent == 0)
end)
scenario("unlisted-note feedback clears and ARM preserves current feedback", function()
  incoming(0x91, 59, 77)
  input("arm", true)
  assert(controls.feedback.values.text == "Received CH 2 note 59 velocity 77")
  input("disarm", false) -- Release callback alone must not reset feedback.
  assert(controls.feedback.values.text == "Received CH 2 note 59 velocity 77")
  input("disarm", true)
  cleared()
  assert(#sent == 0)
end)
scenario("velocity-zero Note On clears an already highlighted key", function()
  incoming(0x91, 60, 95); isColour(60, true)
  incoming(0x91, 60, 0); isColour(60, false)
  assert(controls.feedback.values.text == "Received CH 2 note 60 velocity 0")
  assert(#sent == 0)
end)
assert(failures == 0, failures .. " of " .. scenarios .. " feedback scenarios failed")
print("TouchOSC feedback lifetime: " .. scenarios .. " scenarios PASS")
print("TouchOSC root controller: PASS")
