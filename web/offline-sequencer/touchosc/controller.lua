-- SPDX-License-Identifier: GPL-3.0-only
-- TouchOSC document-root script. Musical MIDI only; no SysEx, flash or project writes.
-- Connection 1 must be explicitly routed by the owner before arming this document.
local channel = 2 -- Human MIDI channel; change to match GLO > MIDI on the keyboard.
local armed = false
local active = {}
local allowed = {[36]=true, [38]=true, [42]=true, [46]=true}
for note=60,71 do allowed[note] = true end
local connection = {true, false, false, false, false, false, false, false, false, false}

local function display(name, text)
  local control = root:findByName(name)
  if control then control.values.text = text end
end

local function routeStatus()
  display("route", (armed and "ARMED" or "DISARMED") .. " / MIDI CH " .. channel .. " / connection 1")
end

local function colour(note, on)
  local control = root:findByName("note_" .. note)
  if control then
    control.color = on and Color(0.55, 0.90, 0.76, 1) or Color(0.16, 0.25, 0.30, 1)
  end
end

local function resetFeedback()
  -- Incoming notes affect presentation, not `active`. Clear both sources of
  -- highlights when disarming so old-route feedback cannot survive a reset.
  for note in pairs(allowed) do colour(note, false) end
  display("feedback", "No incoming MIDI observed")
end

local function release(note)
  local held = active[note]
  if not held then return end
  -- Retire the debt on this send attempt. This is not a transport retry queue.
  active[note] = nil
  local control = root:findByName("note_" .. note)
  if control then control.tag = "" end
  -- Never use the current channel for a note started on another channel.
  sendMIDI({0x80 + held.channel - 1, note, 0}, connection)
  colour(note, false)
end

local function disarm()
  armed = false
  local notes = {}
  for note in pairs(active) do notes[#notes + 1] = note end
  for _,note in ipairs(notes) do release(note) end
  -- A cancelled native pointer can leave x=1 after touch becomes false.
  -- Reset only after retiring active notes, so reentrant callbacks are inert.
  for note in pairs(allowed) do
    local control = root:findByName("note_" .. note)
    if control then control.values.x = 0 end
  end
  resetFeedback()
  routeStatus()
end

function init()
  armed = false
  -- There is no supported pre-stop callback. Recover outstanding local ON
  -- markers retained by native controls on return (requires native qualification).
  -- Release BEFORE editing: no script timeout can run while in editor mode.
  for note in pairs(allowed) do
    local control = root:findByName("note_" .. note)
    local saved = control and type(control.tag) == "string"
      and string.match(control.tag, "^dnbox%-held:(%d+)$")
    local original = saved and tonumber(saved)
    if original and original >= 1 and original <= 16 and tostring(original) == saved then
      active[note] = {channel=original, at=getMillis()}
    end
  end
  disarm()
end

function onReceiveNotify(key, value)
  if key ~= "input" or type(value) ~= "table" or type(value.name) ~= "string" then return end
  if type(value.pressed) ~= "boolean" then return end
  local name, pressed = value.name, value.pressed
  if name == "disarm" then
    if pressed then disarm() end
    return
  end
  if name == "channel_up" or name == "channel_down" then
    if pressed then
      disarm()
      channel = math.max(1, math.min(16, channel + (name == "channel_up" and 1 or -1)))
      routeStatus()
    end
    return
  end
  if name == "arm" then
    if pressed then armed = true; routeStatus() end
    return
  end
  local note = tonumber(string.match(name, "^note_(%d+)$"))
  if not note or not allowed[note] then return end
  if not pressed then release(note); return end
  if not armed or active[note] then return end
  active[note] = {channel=channel, at=getMillis()}
  local control = root:findByName(name)
  if control then control.tag = "dnbox-held:" .. channel end
  sendMIDI({0x90 + channel - 1, note, 100}, connection)
  colour(note, true)
end

function update()
  -- A cancelled/lost touch must not leave a note held forever. Ten seconds is
  -- a deliberate safety limit for this performance sketch, not an engine gate.
  local now = getMillis()
  for _,held in pairs(active) do
    if now < held.at or now - held.at >= 10000 then disarm(); return end
  end
end

function onReceiveMIDI(message, connections)
  if type(message) ~= "table" or #message ~= 3 or type(connections) ~= "table" or connections[1] ~= true then return end
  for i=1,3 do
    local byte = message[i]
    if type(byte) ~= "number" or byte ~= math.floor(byte) or byte < 0 or byte > (i == 1 and 255 or 127) then return end
  end
  local status, note, velocity = message[1], message[2], message[3]
  local kind = math.floor(status / 16)
  if kind ~= 8 and kind ~= 9 then return end
  local receivedChannel = status % 16 + 1
  if receivedChannel ~= channel then return end
  local on = kind == 9 and velocity > 0
  if allowed[note] then colour(note, on) end
  display("feedback", "Received CH " .. receivedChannel .. " note " .. note .. " velocity " .. (on and velocity or 0))
  -- Feedback changes presentation only. It does not change button x values,
  -- call the input handler, echo MIDI, or claim a synchronized pattern snapshot.
end
