-- SPDX-License-Identifier: GPL-3.0-only
-- Native-derived regressions: VM re-entry with outstanding ON; focus timeout x=1.
-- This API-boundary test cannot prove native tag persistence or MIDI delivery.
local script = arg[1] or 'web/offline-sequencer/touchosc/controller.lua'
local failures, count = 0, 0
local function scenario(name, run)
  count = count + 1
  local sent, controls, now, env = {}, {}, 0, nil
  for _, n in ipairs({36,38,42,46,60,61,62,63,64,65,66,67,68,69,70,71}) do
    local c = {name='note_'..n, tag=''}
    -- Assignments made by root can synchronously re-enter button notifications.
    local values = {x=0, touch=false}
    c.values = setmetatable({}, {
      __index=values,
      __newindex=function(_, k, v)
        local old=values[k]; values[k]=v
        if k=='x' and old~=v and env then
          env.onReceiveNotify('input',{name=c.name,pressed=v>0})
        end
      end})
    controls[c.name]=c
  end
  controls.route={values={}};controls.feedback={values={}}
  local function loadVM()
    env={root={findByName=function(_,n)return controls[n]end},
      sendMIDI=function(m,conn)assert(conn[1] and not conn[2]);sent[#sent+1]=m end,
      Color=function(r,g,b,a)return {r=r,g=g,b=b,a=a}end,getMillis=function()return now end}
    setmetatable(env,{__index=_G})
    local code=assert(loadfile(script));setfenv(code,env);code();env.init()
  end
  local function input(n,v)env.onReceiveNotify('input',{name=n,pressed=v})end
  local function press(n)controls['note_'..n].values.x=1 end
  local function off(index,ch,n)
    local m=sent[index];assert(m and m[1]==0x80+ch-1 and m[2]==n and m[3]==0,'missing original-channel OFF')
  end
  loadVM()
  local ok,err=pcall(run,{loadVM=loadVM,input=input,press=press,off=off,controls=controls,sent=sent,
    tick=function(t)now=t;env.update()end,init=function()env.init()end,
    incoming=function(n)env.onReceiveMIDI({0x91,n,100},{true})end})
  if ok then print(name..': PASS') else failures=failures+1;io.stderr:write(name..': '..tostring(err)..'\n')end
end
scenario('D1 VM re-entry releases on original channel once',function(t)
 t.input('channel_up',true);t.input('arm',true);t.press(60);assert(#t.sent==1)
 t.loadVM();t.off(2,3,60);assert(t.controls.note_60.values.x==0)
 t.init();t.input('disarm',true);assert(#t.sent==2,'re-entry must not repeat attempted OFF')
end)
scenario('D1 same-VM init releases multiple notes once',function(t)
 t.input('arm',true);t.press(60);t.press(61);t.init();assert(#t.sent==4)
 local notes={};for i=3,4 do t.off(i,2,t.sent[i][2]);notes[t.sent[i][2]]=true end
 assert(notes[60] and notes[61]);t.loadVM();assert(#t.sent==4)
end)
scenario('D2 timeout resets stale x and first re-armed tap sounds',function(t)
 t.input('arm',true);t.press(60);t.controls.note_60.values.touch=false;t.tick(10001)
 t.off(2,2,60);assert(t.controls.note_60.values.x==0,'timeout left x=1')
 t.input('arm',true);t.press(60);assert(#t.sent==3,'first recovery tap silent')
 t.controls.note_60.values.x=0;t.off(4,2,60)
end)
scenario('RELEASE and channel change clear x without reentrant duplicate OFF',function(t)
 t.input('arm',true);t.press(60);t.input('disarm',true);t.off(2,2,60)
 assert(t.controls.note_60.values.x==0);t.input('arm',true);t.press(60)
 t.input('channel_up',true);t.off(4,2,60);assert(t.controls.note_60.values.x==0)
 t.loadVM();assert(#t.sent==4)
end)
scenario('receive-only notes never acquire release debt',function(t)
 t.incoming(60);assert(t.controls.note_60.tag=='');t.loadVM();t.init();t.input('disarm',true)
 assert(#t.sent==0);assert(t.controls.note_60.values.x==0)
 assert(t.controls.feedback.values.text=='No incoming MIDI observed')
 assert(t.controls.note_60.color.r==0.16)
end)
scenario('attempted OFF is never retried on re-entry',function(t)
 t.input('arm',true);t.press(60);t.controls.note_60.values.x=0;t.off(2,2,60)
 t.loadVM();t.input('disarm',true);assert(#t.sent==2)
end)
scenario('invalid and foreign markers cannot emit MIDI',function(t)
 for _,v in ipairs({'custom','dnbox-held:0','dnbox-held:17','dnbox-held:2.5','dnbox-held:02','dnbox-held:2garbage'}) do
  t.controls.note_60.tag=v;t.loadVM();assert(#t.sent==0,'invalid marker emitted MIDI')
 end
end)
assert(failures==0,failures..' of '..count..' lifecycle scenarios failed')
print('TouchOSC lifecycle: '..count..' scenarios PASS')
