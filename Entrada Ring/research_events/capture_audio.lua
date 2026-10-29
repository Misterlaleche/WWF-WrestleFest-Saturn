-- Observation only. Original arcade input sequence; no ROM/RAM/register writes.
local out=assert(os.getenv('WF_CAPTURE_OUT'))
local limit=tonumber(os.getenv('WF_CAPTURE_LIMIT') or '1710')
local m=manager.machine
local main=m.devices[':maincpu']
local audio=assert(m.devices[':audiocpu'])
local amem=audio.spaces['program']
local mmem=main.spaces['program']
local screen=m.screens[':screen']
local fields=m.ioport.ports[':P1'].fields
local script={{'Coin 1',750,754},{'1 Player Start',800,804},{'P1 Button 1',960,963},{'P1 Button 1',1060,1063},{'P1 Right',1100,1103},{'P1 Button 1',1160,1163}}
local frame=0
local ymaddr=0
local log=assert(io.open(out..'/audio_writes.csv','w'))
local frames=assert(io.open(out..'/audio_frames.csv','w'))
log:write('frame,screen_frame,time_seconds,chip,address,value,pc\n')
frames:write('frame,screen_frame,time_seconds\n')
local taps={};_G.wp01_audio_taps=taps
local function emit(chip,address,value,cpu)
 log:write(string.format('%d,%d,%.12f,%s,%d,%d,%04X\n',frame,screen:frame_number(),m.time:as_double(),chip,address,value,cpu.state['CURPC'].value))
end
taps[#taps+1]=amem:install_write_tap(0xc800,0xc801,'wp01-audio-ym',function(offset,data,mask)
 if offset==0xc800 then ymaddr=data else emit('YM',ymaddr,data,audio) end
end)
taps[#taps+1]=amem:install_write_tap(0xd800,0xd800,'wp01-audio-oki',function(offset,data,mask) emit('OKI',0,data,audio) end)
taps[#taps+1]=amem:install_write_tap(0xe800,0xe800,'wp01-audio-bank',function(offset,data,mask) emit('BANK',0,data,audio) end)
taps[#taps+1]=mmem:install_write_tap(0x14000c,0x14000d,'wp01-audio-command',function(offset,data,mask) emit('COMMAND',offset,data,main) end)
print('AUDIO CAPTURE MAME '..emu.app_version())
for tag,dev in pairs(m.devices) do if tag:find('audio') or tag:find('ym') or tag:find('oki') then print(tag..' '..dev.name) end end
emu.register_frame_done(function()
 frame=frame+1
 frames:write(string.format('%d,%d,%.12f\n',frame,screen:frame_number(),m.time:as_double()))
 for _,v in ipairs(script) do
  if frame==v[2] then fields[v[1]]:set_value(1) end
  if frame==v[3]+1 then fields[v[1]]:set_value(0) end
 end
 if frame%250==0 then print('AUDIO frame '..frame);log:flush();frames:flush() end
 if frame>=limit then log:close();frames:close();print('AUDIO COMPLETE '..frame);m:exit() end
end,'WP01 native audio evidence')
