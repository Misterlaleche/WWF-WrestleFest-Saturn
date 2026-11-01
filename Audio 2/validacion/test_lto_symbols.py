"""LTO-local diagnostics must remain observable without guessing ambiguous names."""
from pathlib import Path
import struct,unittest
from observe_cold import inserted_source,ORIGINAL

def fixture():
    entries=[('_rt.lto_priv.0',0x06009000,256),
             ('_duplicate.lto_priv.0',0x06009100,4),('_duplicate.lto_priv.1',0x06009200,4),
             ('_public',0x06009300,4),('_public.lto_priv.2',0x06009400,4)]
    strings=bytearray(b'\0');symbols=bytearray(16)
    for name,address,size in entries:
        offset=len(strings);strings.extend(name.encode()+b'\0')
        symbols.extend(struct.pack('>IIIBBH',offset,address,size,0,0,0))
    data=bytearray(172);data[:7]=b'\x7fELF\x01\x02\x01'
    struct.pack_into('>I',data,32,52);struct.pack_into('>HH',data,42,32,0)
    struct.pack_into('>HH',data,46,40,3)
    struct.pack_into('>10I',data,92,0,3,0,0,172,len(strings),0,0,1,0)
    struct.pack_into('>10I',data,132,0,2,0,0,172+len(strings),len(symbols),1,0,4,16)
    return bytes(data+strings+symbols)

class LtoSymbols(unittest.TestCase):
    def image(self):
        ns={'__file__':str(ORIGINAL),'__name__':'fixture'}
        exec(compile(inserted_source(),str(ORIGINAL),'exec'),ns)
        return ns['Elf'](Path('fixture.elf'),fixture())
    def test_unique_lto_runtime_symbol_is_available_to_counter_reader(self):
        elf=self.image()
        self.assertEqual(elf.symbols.get('_rt'),(0x06009000,256))
        self.assertEqual(elf.symbols['_rt.lto_priv.0'],(0x06009000,256))
    def test_ambiguous_or_public_symbols_are_not_replaced(self):
        elf=self.image()
        self.assertNotIn('_duplicate',elf.symbols)
        self.assertEqual(elf.symbols['_public'],(0x06009300,4))

if __name__=='__main__':unittest.main()
