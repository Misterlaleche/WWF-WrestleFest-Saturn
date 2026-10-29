local m=manager.machine
local s=m.devices[':maincpu'].spaces['program']
local a=m.devices[':audiocpu'].spaces['program']
local p=m.ioport.ports[':P1'].fields
local frame=0
local started=nil
local root='/workspace/scratch/8e35fd0cc989/wp02/verification/mame/audio_music'
local f=io.open(root..'/writes.csv','w')
f:write('frame,time,device,address,data,mask\n')
local function record(dev,offset,data,mask)
 f:write(string.format('%d,%.9f,%s,%X,%X,%X\n',frame,m.time:as_double(),dev,offset,data,mask))
end
local main_tap=s:install_write_tap(0x14000c,0x14000d,'wp02-sound-command',function(offset,data,mask)record('main',offset,data,mask)end)
local oki_pending=false
local oki_tap=a:install_write_tap(0xd800,0xd800,'wp02-oki-command',function(offset,data,mask)
 record('oki',offset,data,mask)
 if oki_pending then
  oki_pending=false
  -- Retain physical music voice0/mask1 only, including original nibble.
  -- YM timing/IRQ and bank writes are untouched.
  local filtered=data&0x1f
  record('oki_isolated',offset,filtered,mask)
  return filtered
 elseif (data&0x80)~=0 then oki_pending=true end
end)
local bank_tap=a:install_write_tap(0xe800,0xe800,'wp02-oki-bank',function(offset,data,mask)record('bank',offset,data,mask)end)
local ymreg=0
local ym_tap=a:install_write_tap(0xc800,0xc801,'wp02-ym-command',function(offset,data,mask)
 if offset==0xc800 then ymreg=data else record('ym',ymreg,data,mask)end
end)
_G.wp02_audio_taps={main_tap,oki_tap,bank_tap,ym_tap}
local function held(n,x,y) p[n]:set_value(frame>=x and frame<y and 1 or 0) end
emu.register_frame_done(function()
 frame=frame+1
 held('Coin 1',150,154);held('1 Player Start',1400,1404);held('P1 Right',1470,1474)
 p['P1 Button 1']:set_value(((frame>=1540 and frame<1544)or(frame>=1720 and frame<1724)or(frame>=1920 and frame<1924))and 1 or 0)
 local present=s:read_u16(0x1c14ce)==0x8001 and s:read_u16(0x1c14d0)==0x1f and s:read_u16(0x1c14d2)~=0xffff
 if present and not started then
  started=frame;print('INTRO_START',frame,m.time:as_double())
  local ram=io.open(root..'/soundram-start.bin','wb');ram:write(a:read_range(0xc000,0xc7ff,8));ram:close()
 end
 if frame==4817 then
  -- Source-extraction boundary only: stop main CPU before gameplay's new
  -- music producer, while original Z80/YM/OKI continue uninterrupted.
  s:write_u16(0x1c3ff0,0x60fe)
  m.devices[':maincpu'].state['SR'].value=0x2700
  m.devices[':maincpu'].state['CURPC'].value=0x1c3ff0
  print('MAIN_HELD_FOR_CONTINUOUS_AUDIO',frame)
 end
 if frame>=5200 then
  print('INTRO_END',frame,m.time:as_double());f:close();m:exit()
 end
end,'frame')
