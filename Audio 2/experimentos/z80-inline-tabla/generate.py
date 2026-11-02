from pathlib import Path
import importlib.util,json
base=Path(__file__).resolve().parent;repo=base/'repo';out=base/'generated';out.mkdir(exist_ok=True)
spec=importlib.util.spec_from_file_location('inline_gen',repo/'tools/audio/native_probe/generate_z80_control.py');g=importlib.util.module_from_spec(spec);spec.loader.exec_module(g)
report=g.generate(repo,repo/'input/sound.rom',out/'z80-native.c');(out/'generator.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
