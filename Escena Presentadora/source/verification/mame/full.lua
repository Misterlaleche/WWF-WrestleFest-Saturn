local m=manager.machine
local s=m.devices[':maincpu'].spaces['program']
local p=m.ioport.ports[':P1'].fields
local frame=0
local started=nil
local data=io.open('/workspace/scratch/8e35fd0cc989/wp02/verification/mame/full/frames.bin','wb')
local out=io.open('/workspace/scratch/8e35fd0cc989/wp02/verification/mame/full/frames.csv','w')
out:write('frame,state,presenter_flags,sel,x,y,identity0\n')
local function held(n,a,b) p[n]:set_value(frame>=a and frame<b and 1 or 0) end
emu.register_frame_done(function()
 frame=frame+1
 held('Coin 1',150,154)
 held('1 Player Start',1400,1404)
 held('P1 Right',1470,1474)
 p['P1 Button 1']:set_value(((frame>=1540 and frame<1544) or (frame>=1720 and frame<1724) or (frame>=1920 and frame<1924)) and 1 or 0)
 local present=s:read_u16(0x1c14ce)==0x8001 and s:read_u16(0x1c14d0)==0x1f and s:read_u16(0x1c14d2)~=0xffff
 if present and not started then started=frame;print('INTRO_START',frame) end
 if started then
  local n=frame-started
  local function dump(a,len) return s:read_range(a,a+len-1,8) end
  data:write(string.pack('>I4',frame),dump(0x1c0000,0x4000),dump(0xc2000,0x2000),dump(0xc0000,0x2000),dump(0x80000,0x1000),dump(0x82000,0x1000),dump(0x100000,8))
  out:write(string.format('%d,%d,%d,%d,%d,%d,%d\n',frame,s:read_u8(0x1c14eb),s:read_u16(0x1c14ce),s:read_u16(0x1c14d2),s:read_u16(0x1c14d4),s:read_u16(0x1c14d8),s:read_u8(0x1c0607)))
  if n%30==0 or s:read_u8(0x1c14eb)>=0x15 then m.screens[':screen']:snapshot(string.format('/workspace/scratch/8e35fd0cc989/wp02/verification/mame/full/sn%04d.png',n)) end
  if not present and n>10 then print('INTRO_END',frame,n);out:close();data:close();m:exit() end
 end
 if frame==6000 then out:close();m:exit() end
end,'frame')
