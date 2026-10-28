#!/usr/bin/env python3
"""Inventory arcade selector IDs and port evidence without claiming completion."""

import ast
import hashlib
import json
import re
import tarfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ROM = ROOT / 'src/runtime/audit/runtime18-cd/ROM68K.BIN'
FAMILY = ROOT / 'src/runtime/tests/r18/action_reversal_family_verify_rom.py'
BRIDGE = ROOT / 'src/runtime/tests/r18/action_reversal_family_oracle_bridge.c'
TESTS = ROOT / 'src/runtime/tests/r18'
ACTION_MATRIX = ROOT / 'src/runtime/generated/wf_action_matrix.c'
REPORTS = ROOT / 'validation/local-20261026/action-interleaved'
RING_REPORTS = ROOT / 'validation/local-20261026'
RING_VERIFIER = ROOT / 'src/runtime/tests/r18/action68_sequence_verify_rom.py'
ACTION79_VERIFIER = ROOT / 'src/runtime/tests/r18/action79_interleaved_verify_rom.py'
OUT = ROOT / 'validation/local-20261026/action-selector-inventory.json'

runtime_root = ROOT/'src/runtime'
def assert_frozen_sources(sources):
    for name, digest in sources.items():
        normalized = name.replace('\\', '/')
        actual = hashlib.sha256((runtime_root/normalized).read_bytes()).hexdigest()
        assert actual == digest, normalized


def family_ids():
    tree = ast.parse(FAMILY.read_text())
    value = next(node.value for node in tree.body if isinstance(node, ast.Assign)
                 and any(isinstance(target, ast.Name) and target.id == 'FAMILY'
                         for target in node.targets))
    return set(ast.literal_eval(value))


rom = ROM.read_bytes()
assert hashlib.sha1(rom).hexdigest() == 'a726c46f350723ea73158bc78055d17d418b14e2'
family = family_ids()
named = {int(match.group(1), 16): path for path in TESTS.glob('action*_verify_rom.py')
         if (match := re.fullmatch(r'action([0-9a-fA-F]{2})_verify_rom\.py', path.name))}
assert len(family) == 72 and len(named) == 78
assert family | set(named) == set(range(0x8f))

triples = re.findall(r'\{(0x[0-9a-fA-F]+),(0x[0-9a-fA-F]+),(0x[0-9a-fA-F]+)\}',
                    ACTION_MATRIX.read_text())
assert len(triples) == 12 * 21
matrix = [[tuple(int(value, 16) for value in row)
           for row in triples[character*21:(character+1)*21]]
          for character in range(12)]
for rows in matrix:
    assert all(value == 0xff or value < 0x8f for row in rows for value in row)

evidence = {selector: [] for selector in range(0x8f)}
report_count = 0
verifier_hash = hashlib.sha256(FAMILY.read_bytes()).hexdigest()
bridge_hash = hashlib.sha256(BRIDGE.read_bytes()).hexdigest()
for report_path in sorted(REPORTS.glob('action*.json')):
    report = json.loads(report_path.read_text())
    assert report['status'] == 'PASS' and report['cases'] == 12
    assert report['rom_sha1'] == hashlib.sha1(rom).hexdigest()
    assert report['test_sha256'] == verifier_hash and report['bridge_sha256'] == bridge_hash
    assert len(report['sequence_outcomes']) == 12
    assert sum(outcome['steps'] for outcome in report['sequence_outcomes']) == report['total_steps']
    if report['victim_follow']:
        assert sum(outcome['victim_steps'] for outcome in report['sequence_outcomes']) == report['total_victim_steps']
        if report['jump_contact'] == 'counter61' or report['jump10_contact'] == 'counter61':
            assert all(outcome['victim_selector_at_impact'] & 0xff == 0x77 and
                       outcome['victim_state_after_follow'] & 0xff == 5 and
                       outcome['target_selector'] & 0xff == 0x61 and
                       outcome['selector'] & 0xff == 0x37
                       for outcome in report['sequence_outcomes'])
        else:
            assert all(outcome['victim_at_impact'] in (3, 6, 7, 8, 26, 28, 32) and
                       outcome['victim_state_after_follow'] & 0xff == 0
                       for outcome in report['sequence_outcomes'])
            if report['selectors'] == ['0xe']:
                assert all(outcome['victim_selector_at_impact'] == 0x51
                           for outcome in report['sequence_outcomes'])
    report_count += 1
    for selector in report['selectors']:
        evidence[int(selector, 16)].append(str(report_path.relative_to(ROOT)).replace('\\', '/'))

ring_evidence = {selector: [] for selector in range(0x8f)}
ring_count = 0
ring_verifier_hash = hashlib.sha256(RING_VERIFIER.read_bytes()).hexdigest()
# Preserve the action-only historical reports solely when the old implementation
# is byte-identical after removing the one newly exported collision wrapper.
actions_path = ROOT/'src/runtime/src/wf_r17_actions_r11.c'
current_actions = actions_path.read_bytes().replace(b"\r\n", b"\n")
wrapper = b"""wf_status_t wf_r17_action_cleanup_212a0(wf_runtime_t*rt,wf_actor_raw_t*a,uint32_t*blocked_rom){
 wf_r17_actions_r11_trace_t trace={0};
 if(!rt||!a)return WF_BAD_STATE;
 wf_status_t status=interrupt_cleanup_212a0(rt,a,&trace);
 if(blocked_rom)*blocked_rom=trace.blocked_dependency_rom;
 return status;
}
"""
archive = ROOT/'validation/local-20261028/action-source-r05/action-source.tar.gz'
manifest = json.loads((archive.parent/'source-manifest.json').read_text())
assert hashlib.sha256(archive.read_bytes()).hexdigest() == manifest['archive_sha256']
with tarfile.open(archive, 'r:gz') as frozen:
    old_actions = frozen.extractfile('runtime18/src/runtime/src/wf_r17_actions_r11.c').read()
entry = manifest['files']['src/runtime/src/wf_r17_actions_r11.c']
assert hashlib.sha256(old_actions).hexdigest() == entry['build_sha256']
assert current_actions.count(wrapper) == 1 and current_actions.replace(wrapper, b'') == old_actions
for test in (RING_VERIFIER, ACTION79_VERIFIER, BRIDGE, *(TESTS/f'action{route}_oracle_bridge.c' for route in ('79', '1f', '2d', '2e', '2f'))):
    assert b'wf_r17_action_cleanup_212a0' not in test.read_bytes()
actions_hash = entry['source_sha256']
resolver_hash = hashlib.sha256((ROOT/'src/runtime/src/wf_r17_resolver_280dc.c').read_bytes()).hexdigest()
transition_report = RING_REPORTS/'ringside-transition-current-rom.json'
for pattern in ('action68-interleaved-natural-*.json',
                'action6a-interleaved-postscene-*.json',
                'action6b-interleaved-natural-*.json',
                'action7a-interleaved-natural-*.json'):
    for report_path in sorted(RING_REPORTS.glob(pattern)):
        report = json.loads(report_path.read_text())
        assert report['status'] == 'PASS' and report['rom_sha1'] == hashlib.sha1(rom).hexdigest()
        assert report['verifier_sha256'] == ring_verifier_hash
        assert report['bridge_sha256'] == bridge_hash and report['source_sha256'] == actions_hash
        if report['route'] == '6a':
            assert report['source_transition_report_sha256'] == hashlib.sha256(transition_report.read_bytes()).hexdigest()
        ring_count += 1
        for _, selector in report['selector_history']:
            if report['route'] == '7a' and selector == 0:
                continue  # Actor was cleared; this is not a new $00 action.
            ring_evidence[selector].append(str(report_path.relative_to(ROOT)).replace('\\', '/'))
assert ring_count == 93, ring_count

action79_path = RING_REPORTS/'action79-interleaved-all-characters.json'
action79 = json.loads(action79_path.read_text())
assert action79['status'] == 'PASS' and action79['rom_sha1'] == hashlib.sha1(rom).hexdigest()
assert action79['verifier_sha256'] == hashlib.sha256(ACTION79_VERIFIER.read_bytes()).hexdigest()
assert action79['action_source_sha256'] == actions_hash
assert action79['resolver_source_sha256'] == resolver_hash
assert action79['bridges_sha256']['action_reversal_family_oracle_bridge.c'] == bridge_hash
assert action79['bridges_sha256']['action79_oracle_bridge.c'] == hashlib.sha256(
    (TESTS/'action79_oracle_bridge.c').read_bytes()).hexdigest()
assert [case['character'] for case in action79['cases']] == list(range(12))
assert action79['total_steps'] == sum(case['steps'] for case in action79['cases']) == 822
assert action79['visual_character_pose_pairs'] == action79['graphic_covered_visual_pairs'] == 54
other_evidence = {selector: [] for selector in range(0x8f)}
other_evidence[0x79].append(str(action79_path.relative_to(ROOT)).replace('\\', '/'))
natural00_path = ROOT/'validation/local-20261028/revalidate-24aa8/action00-alternate.json'
natural00 = json.loads(natural00_path.read_text())
assert natural00['status'] == 'PASS' and natural00['rom_sha1'] == hashlib.sha1(rom).hexdigest()
assert natural00['source_character'] == 3 and natural00['total_steps'] == 396
assert [case['character'] for case in natural00['cases']] == list(range(12))
assert all(case['steps'] == 33 and case['terminal_state'] == 0 for case in natural00['cases'])
assert natural00['graphic_character_pose_pairs'] == 48
assert natural00['verifier_sha256'] == hashlib.sha256(
    (TESTS/'action00_alternate_natural_verify_rom.py').read_bytes()).hexdigest()
assert natural00['snapshot_sha256'] == hashlib.sha256(
    (ROOT/'validation/local-20261024/tag-winner-pressure-after1f/snapshot-01216.bin').read_bytes()).hexdigest()
assert natural00['historical_trace_sha256'] == hashlib.sha256(
    (ROOT/'validation/local-20261024/tag-winner-pressure-after1f/trace.csv').read_bytes()).hexdigest()
assert natural00['graphics_report_sha256'] == hashlib.sha256(
    (ROOT/'validation/local-20261027/action-pose-metasprite-after-wordcache.json').read_bytes()).hexdigest()
assert_frozen_sources(natural00['sources_sha256'])
other_evidence[0].append(str(natural00_path.relative_to(ROOT)).replace('\\', '/'))
natural07_path = ROOT/'validation/local-20261028/revalidate-24aa8/action07-natural.json'
natural07 = json.loads(natural07_path.read_text())
assert natural07['status'] == 'PASS' and natural07['rom_sha1'] == hashlib.sha1(rom).hexdigest()
assert natural07['source_character'] == 7 and natural07['total_steps'] == 460
assert [case['character'] for case in natural07['cases']] == list(range(12))
assert all(case['steps'] == (42 if case['character'] == 7 else 38)
           and case['terminal_state'] == 0 for case in natural07['cases'])
assert natural07['characters_with_rom_poses'] == [7, 8]
assert natural07['cpu_choice_tables_checked'] == 144
assert {hit['character'] for hit in natural07['human_choice_hits']} == {7, 8}
assert {hit['character'] for hit in natural07['cpu_choice_hits']} == {7, 8}
assert natural07['verifier_sha256'] == hashlib.sha256(
    (TESTS/'action07_natural_continuation_verify_rom.py').read_bytes()).hexdigest()
assert natural07['snapshot_sha256'] == hashlib.sha256(
    (ROOT/'validation/local-20261024/tag-current-basic-afterraw79/snapshot-00408.bin').read_bytes()).hexdigest()
assert natural07['historical_trace_sha256'] == hashlib.sha256(
    (ROOT/'validation/local-20261024/tag-current-basic-afterraw79/trace.csv').read_bytes()).hexdigest()
assert natural07['graphics_report_sha256'] == hashlib.sha256(
    (ROOT/'validation/local-20261027/action-pose-metasprite-after-wordcache.json').read_bytes()).hexdigest()
assert_frozen_sources(natural07['sources_sha256'])
other_evidence[7].append(str(natural07_path.relative_to(ROOT)).replace('\\', '/'))
royal07_path = ROOT/'validation/local-20261028/revalidate-24aa8/action07-royal.json'
royal07 = json.loads(royal07_path.read_text())
assert royal07['status'] == 'PASS' and royal07['rom_sha1'] == hashlib.sha1(rom).hexdigest()
assert royal07['source_character'] == 8 and royal07['total_steps'] == 38
assert len(royal07['cases']) == 1 and royal07['cases'][0]['character'] == 8
assert royal07['cases'][0]['steps'] == 38 and royal07['cases'][0]['terminal_state'] == 0
assert royal07['verifier_sha256'] == hashlib.sha256(
    (TESTS/'action07_royal_natural_verify_rom.py').read_bytes()).hexdigest()
royal07_capture = ROOT/'validation/local-20261024/full-flow-combat-native-f8c-royal-after60-basic'
assert royal07['snapshot_sha256'] == hashlib.sha256((royal07_capture/'snapshot-01205.bin').read_bytes()).hexdigest()
assert royal07['historical_trace_sha256'] == hashlib.sha256((royal07_capture/'trace.csv').read_bytes()).hexdigest()
assert royal07['graphics_report_sha256'] == hashlib.sha256(
    (ROOT/'validation/local-20261027/action-pose-metasprite-after-wordcache.json').read_bytes()).hexdigest()
assert_frozen_sources(royal07['sources_sha256'])
other_evidence[7].append(str(royal07_path.relative_to(ROOT)).replace('\\', '/'))
matrix_evidence = {selector: [] for selector in range(0x8f)}
matrix00_path = ROOT/'validation/local-20261028/revalidate-24aa8/action00-matrix.json'
matrix00 = json.loads(matrix00_path.read_text())
assert matrix00['status'] == 'PASS' and not matrix00['limited'] and matrix00['cases'] == 96384
assert matrix00['rom_sha1'] == hashlib.sha1(rom).hexdigest()
assert matrix00['verifier_sha256'] == hashlib.sha256(
    (TESTS/'action00_resolver_matrix_verify_rom.py').read_bytes()).hexdigest()
assert matrix00['capture_sha256'] == hashlib.sha256(
    (ROOT/'validation/local-20261028/action00-tag-natural-1261/pre-f4c2-snapshot.bin').read_bytes()).hexdigest()
assert matrix00['callback_helper_resolver_conditionals_with_both_outcomes'] == 51
assert matrix00['dispatcher_animation_conditionals_with_both_outcomes'] == 6
assert_frozen_sources(matrix00['sources_sha256'])
matrix_evidence[0].append(str(matrix00_path.relative_to(ROOT)).replace('\\', '/'))
phase_evidence = {selector: [] for selector in range(0x8f)}
phase00_dir = ROOT/'validation/local-20261028/action00-tag-natural-1261'
phase00_original = (phase00_dir/'pre-f4c2-snapshot.bin').read_bytes()
phase00_alternate = (phase00_dir/'pre-f4c2-alternate-controlled.bin').read_bytes()
assert len(phase00_original) == len(phase00_alternate)
role_offset = 56 + 0x5b0 + 4*0x10c + 0x56
flag_offset = 56 + 0x5b0 + 4*0x10c + 0x33
assert phase00_original[role_offset] & 0x80
assert phase00_alternate[flag_offset] == phase00_original[flag_offset] | 0x20
assert [i for i, (left, right) in enumerate(zip(phase00_original, phase00_alternate))
        if left != right] == [flag_offset]
phase_verifier = hashlib.sha256((TESTS/'f4c2_sequence_verify_rom.py').read_bytes()).hexdigest()
for name, ticks, capture, indices in (
        ('f4c2-sequence-39.json', 39, phase00_original, [0, 11, 18, 23, 34, 38]),
        ('f4c2-alternate-sequence-33.json', 33, phase00_alternate, [0, 6, 12, 18, 28, 32])):
    path = ROOT/'validation/local-20261028/revalidate-24aa8'/name
    report = json.loads(path.read_text())
    assert report['status'] == 'PASS' and report['ticks_compared'] == ticks
    assert report['rom_sha1'] == hashlib.sha1(rom).hexdigest()
    assert report['capture_sha256'] == hashlib.sha256(capture).hexdigest()
    assert report['verifier_sha256'] == phase_verifier
    assert_frozen_sources(report['sources_sha256'])
    path4 = report['actor_paths']['4']
    assert [entry[0] for entry in path4] == indices
    assert [entry[1][2] for entry in path4] == [0, 0x92, 0x93, 0x94, 0x93, 0x93]
    assert path4[-1][1][0] == 0 and all(entry[1][1] == 0 for entry in path4)
    phase_evidence[0].append(str(path.relative_to(ROOT)).replace('\\', '/'))
interleaved_1e_1f = {}
for route in ('1e', '1f'):
    path = RING_REPORTS/f'action{route}-interleaved-all-characters.json'
    report = json.loads(path.read_text())
    assert report['status'] == 'PASS' and report['route'] == route
    assert report['rom_sha1'] == hashlib.sha1(rom).hexdigest()
    assert report['verifier_sha256'] == hashlib.sha256(ACTION79_VERIFIER.read_bytes()).hexdigest()
    assert report['action_source_sha256'] == actions_hash
    assert report['resolver_source_sha256'] == resolver_hash
    assert report['bridges_sha256']['action_reversal_family_oracle_bridge.c'] == bridge_hash
    assert report['bridges_sha256']['action1f_oracle_bridge.c'] == hashlib.sha256(
        (TESTS/'action1f_oracle_bridge.c').read_bytes()).hexdigest()
    assert report['graphic_action_report_sha256'] == hashlib.sha256(
        (RING_REPORTS/'action-pose-metasprite-verify.json').read_bytes()).hexdigest()
    assert report['graphic_royal_report_sha256'] == hashlib.sha256(
        (RING_REPORTS/'royal-exit-poses-direct-rom.json').read_bytes()).hexdigest()
    assert [case['character'] for case in report['cases']] == list(range(12))
    assert report['total_steps'] == sum(case['steps'] for case in report['cases']) == 348
    assert report['visual_character_pose_pairs'] == report['graphic_covered_visual_pairs'] == 60
    interleaved_1e_1f[route] = report
    other_evidence[int(route, 16)].append(str(path.relative_to(ROOT)).replace('\\', '/'))
action2f_path = RING_REPORTS/'action2f-interleaved-all-characters.json'
action2f = json.loads(action2f_path.read_text())
assert action2f['status'] == 'PASS' and action2f['route'] == '2f'
assert action2f['rom_sha1'] == hashlib.sha1(rom).hexdigest()
assert action2f['verifier_sha256'] == hashlib.sha256(ACTION79_VERIFIER.read_bytes()).hexdigest()
assert action2f['action_source_sha256'] == actions_hash
assert action2f['resolver_source_sha256'] == resolver_hash
assert action2f['bridges_sha256']['action_reversal_family_oracle_bridge.c'] == bridge_hash
assert action2f['bridges_sha256']['action2f_oracle_bridge.c'] == hashlib.sha256(
    (TESTS/'action2f_oracle_bridge.c').read_bytes()).hexdigest()
assert action2f['graphic_action_report_sha256'] == hashlib.sha256(
    (RING_REPORTS/'action-pose-metasprite-verify.json').read_bytes()).hexdigest()
assert [case['character'] for case in action2f['cases']] == list(range(12))
assert action2f['total_steps'] == sum(case['steps'] for case in action2f['cases']) == 1537
assert action2f['visual_character_pose_pairs'] == 84
assert action2f['graphic_target_visual_pairs'] == action2f['graphic_covered_visual_pairs'] == 14
assert [case['character'] for case in action2f['cases'] if case['character'] in (1,5)] == [1,5]
other_evidence[0x2f].append(str(action2f_path.relative_to(ROOT)).replace('\\', '/'))
action2e_path = RING_REPORTS/'action2e-interleaved-all-characters.json'
action2e = json.loads(action2e_path.read_text())
assert action2e['status'] == 'PASS' and action2e['route'] == '2e'
assert action2e['rom_sha1'] == hashlib.sha1(rom).hexdigest()
assert action2e['verifier_sha256'] == hashlib.sha256(ACTION79_VERIFIER.read_bytes()).hexdigest()
assert action2e['action_source_sha256'] == actions_hash
assert action2e['resolver_source_sha256'] == resolver_hash
assert action2e['bridges_sha256']['action_reversal_family_oracle_bridge.c'] == bridge_hash
assert action2e['bridges_sha256']['action2e_oracle_bridge.c'] == hashlib.sha256(
    (TESTS/'action2e_oracle_bridge.c').read_bytes()).hexdigest()
assert action2e['graphic_action_report_sha256'] == hashlib.sha256(
    (RING_REPORTS/'action-pose-metasprite-verify.json').read_bytes()).hexdigest()
assert [case['character'] for case in action2e['cases']] == list(range(12))
assert action2e['total_steps'] == sum(case['steps'] for case in action2e['cases']) == 408
assert action2e['visual_character_pose_pairs'] == 24
assert action2e['graphic_target_visual_pairs'] == action2e['graphic_covered_visual_pairs'] == 6
other_evidence[0x2e].append(str(action2e_path.relative_to(ROOT)).replace('\\', '/'))
action2d_path = RING_REPORTS/'action2d-interleaved-all-characters.json'
action2d = json.loads(action2d_path.read_text())
assert action2d['status'] == 'PASS' and action2d['route'] == '2d'
assert action2d['rom_sha1'] == hashlib.sha1(rom).hexdigest()
assert action2d['verifier_sha256'] == hashlib.sha256(ACTION79_VERIFIER.read_bytes()).hexdigest()
assert action2d['action_source_sha256'] == actions_hash
assert action2d['resolver_source_sha256'] == resolver_hash
assert action2d['bridges_sha256']['action_reversal_family_oracle_bridge.c'] == bridge_hash
assert action2d['bridges_sha256']['action2d_oracle_bridge.c'] == hashlib.sha256(
    (TESTS/'action2d_oracle_bridge.c').read_bytes()).hexdigest()
assert action2d['graphic_action_report_sha256'] == hashlib.sha256(
    (RING_REPORTS/'action-pose-metasprite-verify.json').read_bytes()).hexdigest()
assert [case['character'] for case in action2d['cases']] == list(range(12))
assert action2d['total_steps'] == sum(case['steps'] for case in action2d['cases']) == 276
assert action2d['visual_character_pose_pairs'] == 24
assert action2d['graphic_target_visual_pairs'] == action2d['graphic_covered_visual_pairs'] == 10
other_evidence[0x2d].append(str(action2d_path.relative_to(ROOT)).replace('\\', '/'))
selection_reports = {}
for route, characters, human_count, cpu_count in (
        ('2d', {0,5,6,10,11}, 15, 14),
        ('2e', {1,5,9}, 6, 11), ('2f', {1,5}, 5, 0)):
    path = RING_REPORTS/f'action{route}-selection-sources.json'
    report = json.loads(path.read_text())
    assert report['status'] == 'PASS' and report['selector'] == route
    assert report['rom_sha1'] == hashlib.sha1(rom).hexdigest()
    assert report['generated_matrix_sha256'] == hashlib.sha256(ACTION_MATRIX.read_bytes()).hexdigest()
    assert report['human_table_rows_checked'] == 252 and report['cpu_tables_checked'] == 144
    assert len(report['human_selector_hits']) == human_count
    assert {item['character'] for item in report['human_selector_hits']} == characters
    assert len(report['cpu_selector_hits']) == cpu_count
    assert {item['character'] for item in report['cpu_selector_hits']} <= characters
    selection_reports[route] = str(path.relative_to(ROOT)).replace('\\', '/')

selectors = []
for selector in range(0x8f):
    offset = 0x12614 + selector*4
    descriptor = int.from_bytes(rom[offset:offset+4], 'big')
    assert 0 < descriptor < len(rom)
    characters = [character for character, rows in enumerate(matrix)
                  if any(selector in row for row in rows)]
    selectors.append({
        'id': f'{selector:02X}',
        'descriptor_rom': f'{descriptor:06X}',
        'characters_in_generated_choice_matrix': characters,
        'family_differential_script': selector in family,
        'named_differential_script':
            str(named[selector].relative_to(ROOT)).replace('\\', '/') if selector in named else None,
        'new_interleaved_sequence_reports': evidence[selector],
        'ring_scene_interleaved_reports': ring_evidence[selector],
        'other_interleaved_sequence_reports': other_evidence[selector],
        'additional_differential_matrix_reports': matrix_evidence[selector],
        'whole_phase_sequence_reports': phase_evidence[selector],
    })

result = {
    'rom_sha1': hashlib.sha1(rom).hexdigest(),
    'rom_selector_table': '0x12614',
    'selector_ids': 143,
    'family_script_ids': len(family),
    'named_script_ids': len(named),
    'script_union_ids': len(family | set(named)),
    'verified_current_sequence_reports': report_count,
    'verified_ring_scene_sequence_reports': ring_count,
    'verified_action00_resolver_matrix_cases': matrix00['cases'],
    'verified_action00_alternate_natural_steps': natural00['total_steps'],
    'verified_action07_natural_continuation_steps': natural07['total_steps'],
    'verified_action07_royal_natural_steps': royal07['total_steps'],
    'verified_action79_interleaved_cases': len(action79['cases']),
    'verified_action79_interleaved_steps': action79['total_steps'],
    'verified_action1e_1f_interleaved_cases': sum(len(report['cases']) for report in interleaved_1e_1f.values()),
    'verified_action1e_1f_interleaved_steps': sum(report['total_steps'] for report in interleaved_1e_1f.values()),
    'verified_action2f_interleaved_cases': len(action2f['cases']),
    'verified_action2f_interleaved_steps': action2f['total_steps'],
    'verified_action2f_matrix_graphic_pairs': action2f['graphic_covered_visual_pairs'],
    'verified_action2e_interleaved_cases': len(action2e['cases']),
    'verified_action2e_interleaved_steps': action2e['total_steps'],
    'verified_action2e_matrix_graphic_pairs': action2e['graphic_covered_visual_pairs'],
    'verified_action2d_interleaved_cases': len(action2d['cases']),
    'verified_action2d_interleaved_steps': action2d['total_steps'],
    'verified_action2d_matrix_graphic_pairs': action2d['graphic_covered_visual_pairs'],
    'action2d_normal_selection_sources_report': selection_reports['2d'],
    'action2e_normal_selection_sources_report': selection_reports['2e'],
    'action2f_normal_selection_sources_report': selection_reports['2f'],
    'characters': [{'id': character,
                    'selectors_in_generated_choice_matrix':
                        len({value for row in rows for value in row if value != 0xff})}
                   for character, rows in enumerate(matrix)],
    'selectors': selectors,
    'limits': [
        'A test script being present does not establish that every internal branch passed.',
        'The generated choice matrix does not list selectors reached dynamically by other routines.',
        'The action and ring/scene report counts measure evidence files, not the percentage of certified selectors.',
        'The ring/scene reports include controlled character and scene variants; they are not all natural captures.',
        '$2F has two character IDs in the generated choice matrix (1 and 5); its other ten tested IDs are controlled variants, and dynamic reachability remains unproven.',
        '$2E has three character IDs in the generated choice matrix (1, 5 and 9); its other nine tested IDs are controlled variants, and dynamic reachability remains unproven.',
        '$2D has five character IDs in the generated choice matrix (0, 5, 6, 10 and 11); its other seven tested IDs are controlled variants, and dynamic reachability remains unproven.',
        'Historical action-only reports retain their original action source hash; byte comparison verifies the sole later change is an unused public collision wrapper.',
        'Older sequence reports still need review.',
        'Animation images, full match flow, and physical Saturn timing are outside this inventory.',
    ],
}
OUT.parent.mkdir(parents=True, exist_ok=True)
OUT.write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps({'selectors': result['selector_ids'],
                  'family_scripts': len(family), 'named_scripts': len(named),
                  'script_union': result['script_union_ids'],
                  'verified_current_sequence_reports': report_count,
                  'verified_ring_scene_sequence_reports': ring_count,
                  'verified_action79_interleaved_cases': len(action79['cases']),
                  'verified_action1e_1f_interleaved_cases': result['verified_action1e_1f_interleaved_cases'],
                  'verified_action2f_interleaved_cases': result['verified_action2f_interleaved_cases'],
                  'verified_action2e_interleaved_cases': result['verified_action2e_interleaved_cases'],
                  'verified_action2d_interleaved_cases': result['verified_action2d_interleaved_cases'],
                  'new_sequence_selectors': sorted(f'{key:02X}' for key, paths in evidence.items() if paths),
                  'output': str(OUT)}))
