#!/usr/bin/env python3
"""Check action pose references against each selected wrestler's arcade sprite set."""

import hashlib
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ROM_PATH = ROOT / 'src/runtime/audit/runtime18-cd/ROM68K.BIN'
TABLE_AUDIT = ROOT / 'validation/local-20261026/action-animation-table-audit.json'
RUNTIME_AUDIT = ROOT / 'validation/local-20261026/action-runtime-descriptor-audit.json'
RUNTIME_SOURCE = ROOT / 'src/runtime/src/wf_r17_actions_r11.c'
ACTION79_REPORT = ROOT / 'validation/local-20261026/action79-recheck.json'
ACTION79_VERIFIER = ROOT / 'src/runtime/tests/r18/action79_verify_rom.py'
ACTION1F_REPORT = ROOT / 'validation/local-20261026/action1f-all-character-recheck.json'
ACTION1F_VERIFIER = ROOT / 'src/runtime/tests/r18/action1f_verify_rom.py'
INVENTORY = ROOT / 'validation/local-20261026/action-selector-inventory.json'
OUTPUT = ROOT / 'validation/local-20261026/action-pose-link-audit.json'

sys.path.insert(0, str(ROOT / 'src/runtime/tools'))
from wf_r18_rom_graphics import Rom  # noqa: E402

rom_bytes = ROM_PATH.read_bytes()
rom_sha1 = hashlib.sha1(rom_bytes).hexdigest()
assert rom_sha1 == 'a726c46f350723ea73158bc78055d17d418b14e2'
tables = json.loads(TABLE_AUDIT.read_text())
runtime = json.loads(RUNTIME_AUDIT.read_text())
inventory = json.loads(INVENTORY.read_text())
assert tables['status'] == 'PASS' and tables['rom_sha1'] == rom_sha1
assert runtime['status'] == 'PASS' and runtime['rom_sha1'] == rom_sha1
assert runtime['source_sha256'] == hashlib.sha256(RUNTIME_SOURCE.read_bytes()).hexdigest()
assert inventory['rom_sha1'] == rom_sha1
assert len(tables['action_selectors']) == len(inventory['selectors']) == 143

# The graphics parser uses only the verified main 68000 image for pose tables.
graphics = object.__new__(Rom)
graphics.p = rom_bytes
links = 0
parsed = 0
missing = []
unique = set()
for selector, (action, item) in enumerate(zip(tables['action_selectors'],
                                              inventory['selectors'])):
    assert action['id'] == item['id'] == f'{selector:02X}'
    for character in item['characters_in_generated_choice_matrix']:
        assert 0 <= character < 12
        for raw in action['poses_raw']:
            pose = int(raw, 16) & 0x7fff
            links += 1
            unique.add((character, pose))
            address = graphics.pose_entry(character, pose)
            if address in (None, 0xfffe, 0xffff):
                missing.append({'selector': action['id'], 'character': character,
                                'pose': f'{pose:04X}', 'entry': address})
                continue
            rows = graphics.pose_rows(character, pose, partner=0)
            assert rows, (selector, character, pose, 'empty ROM sprite')
            parsed += 1

# $2F's initial descriptor is for graphics set 1. Its callback changes to
# $16C7E for other sets; the initial $020F is deliberately absent in set 5.
assert missing == [{'selector': '2F', 'character': 5, 'pose': '020F',
                    'entry': 0xfffe}], missing
alternate_address = 0x16c7e
assert int.from_bytes(rom_bytes[alternate_address:alternate_address + 4], 'big') == 0x16c9e
alternate_count = int.from_bytes(rom_bytes[alternate_address + 6:alternate_address + 8], 'big')
assert alternate_count == 6
alternate_poses = [int.from_bytes(rom_bytes[alternate_address + 8 +
                2 * alternate_count + 2 * frame:alternate_address + 10 +
                2 * alternate_count + 2 * frame], 'big')
                   for frame in range(alternate_count)]
assert all(graphics.pose_entry(5, pose & 0x7fff) not in (None, 0xfffe, 0xffff)
           and graphics.pose_rows(5, pose & 0x7fff, partner=0)
           for pose in alternate_poses)

extra_links = 0
extra_unique = set()
for descriptor in runtime['additional_descriptors']:
    for selector_id in descriptor['possible_action_selectors']:
        item = inventory['selectors'][int(selector_id, 16)]
        for character in item['characters_in_generated_choice_matrix']:
            for raw in descriptor['poses_raw']:
                if raw == 'FFFF':  # Intentional invisible pose in $125C0.
                    continue
                pose = int(raw, 16) & 0x7fff
                extra_links += 1
                extra_unique.add((character, pose))
                address = graphics.pose_entry(character, pose)
                assert address not in (None, 0xfffe, 0xffff), (
                    descriptor['rom'], selector_id, character, raw, address)
                assert graphics.pose_rows(character, pose, partner=0), (
                    descriptor['rom'], selector_id, character, raw, 'empty ROM sprite')
assert extra_links == 203 and len(extra_unique) == 176
additional_pairs = sorted(extra_unique - unique)
assert len(additional_pairs) == 60

# $79 is selected dynamically, so it has no row in the generated choice
# matrix. The ROM table at $1A30C names one animation descriptor per set.
action79_evidence = json.loads(ACTION79_REPORT.read_text())
assert action79_evidence['rom_sha1'] == rom_sha1
assert action79_evidence['passed'] == action79_evidence['count'] == 7611
assert action79_evidence['action_source_sha256'] == runtime['source_sha256']
assert action79_evidence['verifier_sha256'] == hashlib.sha256(ACTION79_VERIFIER.read_bytes()).hexdigest()
assert len(action79_evidence['covered_branch_sites']) == 20
runtime_by_address = {int(item['rom'], 16): item
                      for item in runtime['additional_descriptors']}
action79_base = tables['action_selectors'][0x79]
assert action79_base['descriptor_rom'] == '01A1BE'
action79_pairs = set()
action79_frames = 0
for character in range(12):
    table_address = 0x1a30c + 4 * character
    descriptor_address = int.from_bytes(rom_bytes[table_address:table_address + 4],
                                        'big')
    descriptor = (action79_base if descriptor_address == 0x1a1be else
                  runtime_by_address[descriptor_address])
    for raw in descriptor['poses_raw']:
        pose = int(raw, 16) & 0x7fff
        address = graphics.pose_entry(character, pose)
        assert address not in (None, 0xfffe, 0xffff), (
            'action79', character, hex(descriptor_address), raw)
        assert graphics.pose_rows(character, pose, partner=0)
        action79_frames += 1
        action79_pairs.add((character, pose))
assert action79_frames == 36 and len(action79_pairs) == 31
new_action79_pairs = sorted(action79_pairs - unique - extra_unique)
assert len(new_action79_pairs) == 30

# $1E/$1F share the ROM table entry; their object-release flag chooses the
# second descriptor. The differential verifier covers both choices for all 12.
action1f_evidence = json.loads(ACTION1F_REPORT.read_text())
assert action1f_evidence['status'] == 'PASS' and action1f_evidence['cases'] == 8552
assert action1f_evidence['natural_continuations'] == 58
assert action1f_evidence['groups']['natural-continuation-raw1E'] == 29
assert action1f_evidence['groups']['natural-continuation-raw1F'] == 29
assert action1f_evidence['groups']['all-character-descriptor-raw1E'] == 144
assert action1f_evidence['groups']['all-character-descriptor-raw1F'] == 144
assert action1f_evidence['source_sha256'] == runtime['source_sha256']
assert action1f_evidence['verifier_sha256'] == hashlib.sha256(ACTION1F_VERIFIER.read_bytes()).hexdigest()
action1f_base = tables['action_selectors'][0x1f]
assert action1f_base['descriptor_rom'] == tables['action_selectors'][0x1e]['descriptor_rom'] == '0159F8'
action1f_alternate = runtime_by_address[0x15a0c]
action1f_pairs = set()
for character in range(12):
    for descriptor in (action1f_base, action1f_alternate):
        for raw in descriptor['poses_raw']:
            pose = int(raw, 16) & 0x7fff
            address = graphics.pose_entry(character, pose)
            assert address not in (None, 0xfffe, 0xffff)
            assert graphics.pose_rows(character, pose, partner=0)
            action1f_pairs.add((character, pose))
assert len(action1f_pairs) == 72
new_action1f_pairs = sorted(action1f_pairs - unique - extra_unique - action79_pairs)
assert len(new_action1f_pairs) == 36

# The natural raw-$1E/$1F snapshots carry poses before their descriptors
# advance. Include both in the direct graphics comparison.
capture_manifest = json.loads((ROOT / 'src/runtime/tests/r18/action1f_manifest.json').read_text())
for name, slot, pose, expected_hash in (
        ('action1f_snapshot_tag_pressure_4189_raw1e.bin', 1, 0x807c,
         capture_manifest['alias1e']['snapshot_sha256']),
        ('action1f_snapshot_tag_pressure_3747.bin', 5, 0x007f,
         capture_manifest['snapshot_sha256'])):
    capture = (ROOT / 'src/runtime/tests/r18' / name).read_bytes()
    assert hashlib.sha256(capture).hexdigest() == expected_hash
    capture_actor = 56 + 0x5b0 + slot * 0x10c
    assert int.from_bytes(capture[capture_actor + 4:capture_actor + 6], 'big') == pose
action1e_1f_capture_pairs = {(character, pose) for character in range(12)
                            for pose in (0x7c, 0x7f)}
for character, pose in action1e_1f_capture_pairs:
    assert graphics.pose_entry(character, pose) not in (None, 0xfffe, 0xffff)
    assert graphics.pose_rows(character, pose, partner=0)
new_action1e_1f_capture_pairs = sorted(action1e_1f_capture_pairs - unique - extra_unique
                                      - action79_pairs - action1f_pairs)
assert len(new_action1e_1f_capture_pairs) == 22

capture2f_path = ROOT / 'src/runtime/tests/r18/action2f_snapshot_f88acad.bin'
capture2f = capture2f_path.read_bytes()
manifest2f = json.loads((ROOT / 'src/runtime/tests/r18/action2f_manifest.json').read_text())
assert hashlib.sha256(capture2f).hexdigest() == manifest2f['sha256']
capture2f_actor = 56 + 0x5b0 + 0x10c
assert int.from_bytes(capture2f[capture2f_actor + 4:capture2f_actor + 6], 'big') == 0x39
action2f_capture_pairs = {(character, 0x39) for character in range(12)}
for character, pose in action2f_capture_pairs:
    assert graphics.pose_entry(character, pose) not in (None, 0xfffe, 0xffff)
    assert graphics.pose_rows(character, pose, partner=0)
assert len(action2f_capture_pairs - unique - extra_unique - action79_pairs
           - action1f_pairs - action1e_1f_capture_pairs) == 12

discovery2f_path = ROOT / 'validation/local-20261026/action2f-interleaved-pose-discovery.json'
discovery2f = json.loads(discovery2f_path.read_text())
assert discovery2f['status'] == 'PASS' and discovery2f['route'] == '2f'
assert discovery2f['rom_sha1'] == rom_sha1
assert discovery2f['snapshot_sha256'] == manifest2f['sha256']
assert discovery2f['verifier_sha256'] == hashlib.sha256(
    (ROOT / 'src/runtime/tests/r18/action79_interleaved_verify_rom.py').read_bytes()).hexdigest()
assert discovery2f['action_source_sha256'] == runtime['source_sha256']
assert discovery2f['resolver_source_sha256'] == hashlib.sha256(
    (ROOT / 'src/runtime/src/wf_r17_resolver_280dc.c').read_bytes()).hexdigest()
assert discovery2f['bridges_sha256']['action2f_oracle_bridge.c'] == hashlib.sha256(
    (ROOT / 'src/runtime/tests/r18/action2f_oracle_bridge.c').read_bytes()).hexdigest()
assert len(discovery2f['cases']) == 12 and discovery2f['total_steps'] == 1537
assert discovery2f['graphic_covered_visual_pairs'] is None
action2f_interleaved_pairs = {(item['character'], int(item['pose'], 16))
                              for item in discovery2f['visual_character_pose_pair_list']}
assert len(action2f_interleaved_pairs) == discovery2f['visual_character_pose_pairs'] == 84
assert inventory['selectors'][0x2f]['characters_in_generated_choice_matrix'] == [1, 5]
action2f_matrix_pairs = {(character, pose) for character, pose in action2f_interleaved_pairs
                         if character in (1, 5)}
assert len(action2f_matrix_pairs) == 14
for character, pose in action2f_matrix_pairs:
    assert graphics.pose_entry(character, pose) not in (None, 0xfffe, 0xffff)
    assert graphics.pose_rows(character, pose, partner=0)
action2f_nonmatrix_missing = {(character, pose) for character, pose in action2f_interleaved_pairs
                              if character not in (1, 5) and
                              graphics.pose_entry(character, pose) in (None, 0xfffe, 0xffff)}
assert len(action2f_nonmatrix_missing) == 26
assert not (action2f_matrix_pairs - unique - extra_unique - action79_pairs
            - action1f_pairs - action1e_1f_capture_pairs - action2f_capture_pairs)

discovery2e = json.loads((ROOT / 'validation/local-20261026/action2e-interleaved-pose-discovery.json').read_text())
manifest2e = json.loads((ROOT / 'src/runtime/tests/r18/action2e_manifest.json').read_text())
assert discovery2e['status'] == 'PASS' and discovery2e['route'] == '2e'
assert discovery2e['rom_sha1'] == rom_sha1
assert discovery2e['snapshot_sha256'] == manifest2e['sha256']
assert discovery2e['verifier_sha256'] == hashlib.sha256(
    (ROOT / 'src/runtime/tests/r18/action79_interleaved_verify_rom.py').read_bytes()).hexdigest()
assert discovery2e['action_source_sha256'] == runtime['source_sha256']
assert discovery2e['resolver_source_sha256'] == hashlib.sha256(
    (ROOT / 'src/runtime/src/wf_r17_resolver_280dc.c').read_bytes()).hexdigest()
assert discovery2e['bridges_sha256']['action2e_oracle_bridge.c'] == hashlib.sha256(
    (ROOT / 'src/runtime/tests/r18/action2e_oracle_bridge.c').read_bytes()).hexdigest()
assert len(discovery2e['cases']) == 12 and discovery2e['total_steps'] == 408
assert discovery2e['graphic_covered_visual_pairs'] is None
action2e_interleaved_pairs = {(item['character'], int(item['pose'], 16))
                              for item in discovery2e['visual_character_pose_pair_list']}
assert len(action2e_interleaved_pairs) == discovery2e['visual_character_pose_pairs'] == 24
assert inventory['selectors'][0x2e]['characters_in_generated_choice_matrix'] == [1, 5, 9]
action2e_matrix_pairs = {(character, pose) for character, pose in action2e_interleaved_pairs
                         if character in (1, 5, 9)}
assert len(action2e_matrix_pairs) == 6
for character, pose in action2e_matrix_pairs:
    assert graphics.pose_entry(character, pose) not in (None, 0xfffe, 0xffff)
    assert graphics.pose_rows(character, pose, partner=0)
action2e_nonmatrix_missing = {(character, pose) for character, pose in action2e_interleaved_pairs
                              if character not in (1, 5, 9) and
                              graphics.pose_entry(character, pose) in (None, 0xfffe, 0xffff)}
assert len(action2e_nonmatrix_missing) == 9
assert not (action2e_matrix_pairs - unique - extra_unique - action79_pairs
            - action1f_pairs - action1e_1f_capture_pairs - action2f_capture_pairs)

manifest2d = json.loads((ROOT / 'src/runtime/tests/r18/action2d_manifest.json').read_text())
capture2d = (ROOT / 'src/runtime/tests/r18/action2d_snapshot_full_flow_3614.bin').read_bytes()
assert hashlib.sha256(capture2d).hexdigest() == manifest2d['sha256']
capture2d_actor = 56 + 0x5b0 + 4*0x10c
assert int.from_bytes(capture2d[capture2d_actor + 4:capture2d_actor + 6], 'big') == 0x000d
assert inventory['selectors'][0x2d]['characters_in_generated_choice_matrix'] == [0, 5, 6, 10, 11]
action2d_capture_pairs = {(character, 0x0d) for character in (0, 5, 6, 10, 11)}
for character, pose in action2d_capture_pairs:
    assert graphics.pose_entry(character, pose) not in (None, 0xfffe, 0xffff)
    assert graphics.pose_rows(character, pose, partner=0)
new_action2d_capture_pairs = sorted(action2d_capture_pairs - unique - extra_unique
                                    - action79_pairs - action1f_pairs
                                    - action1e_1f_capture_pairs - action2f_capture_pairs)
assert len(new_action2d_capture_pairs) == 5

discovery2d = json.loads((ROOT / 'validation/local-20261026/action2d-interleaved-pose-discovery.json').read_text())
assert discovery2d['status'] == 'PASS' and discovery2d['route'] == '2d'
assert discovery2d['rom_sha1'] == rom_sha1
assert discovery2d['snapshot_sha256'] == manifest2d['sha256']
assert discovery2d['verifier_sha256'] == hashlib.sha256(
    (ROOT / 'src/runtime/tests/r18/action79_interleaved_verify_rom.py').read_bytes()).hexdigest()
assert discovery2d['action_source_sha256'] == runtime['source_sha256']
assert discovery2d['resolver_source_sha256'] == hashlib.sha256(
    (ROOT / 'src/runtime/src/wf_r17_resolver_280dc.c').read_bytes()).hexdigest()
assert discovery2d['bridges_sha256']['action2d_oracle_bridge.c'] == hashlib.sha256(
    (ROOT / 'src/runtime/tests/r18/action2d_oracle_bridge.c').read_bytes()).hexdigest()
assert len(discovery2d['cases']) == 12 and discovery2d['total_steps'] == 276
assert discovery2d['graphic_covered_visual_pairs'] is None
action2d_interleaved_pairs = {(item['character'], int(item['pose'], 16))
                              for item in discovery2d['visual_character_pose_pair_list']}
assert len(action2d_interleaved_pairs) == discovery2d['visual_character_pose_pairs'] == 24
action2d_matrix_pairs = {(character, pose) for character, pose in action2d_interleaved_pairs
                         if character in (0, 5, 6, 10, 11)}
assert len(action2d_matrix_pairs) == 10
for character, pose in action2d_matrix_pairs:
    assert graphics.pose_entry(character, pose) not in (None, 0xfffe, 0xffff)
    assert graphics.pose_rows(character, pose, partner=0)
action2d_nonmatrix_missing = {(character, pose) for character, pose in action2d_interleaved_pairs
                              if character not in (0, 5, 6, 10, 11) and
                              graphics.pose_entry(character, pose) in (None, 0xfffe, 0xffff)}
assert len(action2d_nonmatrix_missing) == 7
assert not (action2d_matrix_pairs - unique - extra_unique - action79_pairs
            - action1f_pairs - action1e_1f_capture_pairs - action2f_capture_pairs
            - action2d_capture_pairs)

report = {
    'status': 'PASS',
    'rom_sha1': rom_sha1,
    'generated_table_sha256': tables['table_sha256'],
    'runtime_descriptor_source_sha256': runtime['source_sha256'],
    'action_matrix_pose_links': links,
    'base_pose_links_parsed': parsed,
    'unique_character_pose_pairs': len(unique),
    'candidate_alternate_pose_links': extra_links,
    'candidate_alternate_unique_pairs': len(extra_unique),
    'additional_pose_pairs': [dict(character=character, pose=f'{pose:04X}')
                              for character, pose in additional_pairs],
    'action79_dynamic_descriptor_frames': action79_frames,
    'action79_dynamic_unique_pairs': len(action79_pairs),
    'action79_new_pose_pairs': [dict(character=character, pose=f'{pose:04X}')
                                for character, pose in new_action79_pairs],
    'action1e_1f_dynamic_unique_pairs': len(action1f_pairs),
    'action1e_1f_new_pose_pairs': [dict(character=character, pose=f'{pose:04X}')
                                    for character, pose in new_action1f_pairs],
    'action1e_1f_capture_new_pose_pairs': [dict(character=character, pose=f'{pose:04X}')
                                           for character, pose in new_action1e_1f_capture_pairs],
    'action2f_capture_new_pose_pairs': [dict(character=character, pose=f'{pose:04X}')
                                        for character, pose in sorted(action2f_capture_pairs)],
    'action2f_interleaved_matrix_pose_pairs': len(action2f_matrix_pairs),
    'action2f_controlled_nonmatrix_missing_pose_pairs': len(action2f_nonmatrix_missing),
    'action2e_interleaved_matrix_pose_pairs': len(action2e_matrix_pairs),
    'action2e_controlled_nonmatrix_missing_pose_pairs': len(action2e_nonmatrix_missing),
    'action2d_capture_new_pose_pairs': [dict(character=character, pose=f'{pose:04X}')
                                        for character, pose in new_action2d_capture_pairs],
    'action2d_interleaved_matrix_pose_pairs': len(action2d_matrix_pairs),
    'action2d_controlled_nonmatrix_missing_pose_pairs': len(action2d_nonmatrix_missing),
    'base_descriptor_absences': missing,
    'action2f_alternate_descriptor': {
        'rom': f'{alternate_address:06X}',
        'character': 5,
        'poses': [f'{pose:04X}' for pose in alternate_poses],
        'all_pose_records_present': True,
    },
    'limits': [
        'The generated character choice matrix does not cover every dynamic selector transition.',
        'Alternate descriptor links use a conservative C source call graph, not proven branch reachability.',
        'The alternate descriptor data is present; this audit does not execute the $2F branch.',
        'ROM sprite definitions were parsed, not compared with Saturn VDP1 output.',
    ],
}
OUTPUT.write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps({key: report[key] for key in ('status', 'action_matrix_pose_links',
      'base_pose_links_parsed', 'unique_character_pose_pairs',
      'candidate_alternate_pose_links', 'base_descriptor_absences')}))
