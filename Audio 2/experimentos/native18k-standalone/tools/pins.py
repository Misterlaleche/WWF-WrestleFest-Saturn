"""Explicit immutable snapshot inputs, shared by generation and building."""
from pathlib import Path
import hashlib,json

def sha(data):return hashlib.sha256(data).hexdigest()
def load(package):return json.loads((Path(package)/'PIN.json').read_text())
def verify_sources(package,pin):
 for name,expected in pin['source_sha256'].items():
  if sha((Path(package)/name).read_bytes())!=expected:raise ValueError('Pinned source changed: '+name)
def verify_rom(rom,pin):
 data=Path(rom).read_bytes()
 if sha(data)!=pin['rom_sha256']:raise ValueError('Unsupported complete sound ROM')
 return data
