#!/usr/bin/env python3
"""Describe actual YM-register use in identified original-ROM trace windows.

The observation is bounded by each trace's ORIGIN/END. Absence in these windows
does not prove absence in an unobserved full score, fade or cross-command path.
"""
import argparse
from collections import Counter, defaultdict
import csv
import hashlib
import json
from pathlib import Path


def main():
    p = argparse.ArgumentParser(description=__doc__)
    source = p.add_mutually_exclusive_group(required=True)
    source.add_argument("--traces", type=Path, help="Historical A53 music-*.csv directory")
    source.add_argument("--full-board-report", type=Path, help="Complete coupled reference manifest; verify every trace hash")
    p.add_argument("--out", type=Path, required=True)
    a = p.parse_args()
    report = {"kind": "ORIGINAL_ROM_REGISTER_OBSERVATION_NOT_NATIVE_AUDIO_VALIDATION", "tracks": []}
    if a.full_board_report:
        manifest = json.loads(a.full_board_report.read_text())
        if manifest.get('profile') != 'full-board' or not manifest.get('complete'):
            raise ValueError('Require a complete explicitly coupled full-board reference')
        paths = [(Path(t['trace']), t['command'], t['output_sha256']['trace']) for t in manifest['tracks']]
        report.update(profile='full-board', reference_report_sha256=hashlib.sha256(a.full_board_report.read_bytes()).hexdigest(),
                      command_identity_source='Verified full-board manifest; this capture uses zero in the ORIGIN command field')
    else:
        paths = [(path, None, None) for path in sorted(a.traces.glob('music-*.csv'))]
        report.update(profile='historical-A53-partial-MMIO', command_identity_source='Historical ORIGIN command field')
    for path, expected_command, expected_sha in paths:
        before_sha = hashlib.sha256(path.read_bytes()).hexdigest()
        if expected_sha is not None and before_sha != expected_sha:
            raise ValueError('Reference trace identity mismatch: ' + str(path))
        registers = [0] * 256
        keys = [0] * 8
        ever_keyed = set()
        amd = pmd = 0
        origin = end = command = None
        count = 0
        channel_keys = Counter()
        masks = Counter()
        algorithms = Counter()
        feedbacks = Counter()
        dt2s = Counter()
        keyscales = Counter()
        modes = defaultdict(set)
        lfo_configs = set()
        keyed_lfo_states = 0
        configured_lfo_on_ever_keyed = set()
        unsupported = []
        patch_configs = set()
        with path.open(newline="") as f:
            for row in csv.DictReader(f):
                kind, cycle = row["kind"], int(row["cycle"])
                reg, value = int(row["a"]), int(row["b"])
                if kind == "ORIGIN": origin, command = cycle, reg if expected_command is None else expected_command
                if kind == "END": end = cycle
                if kind != "YM_D": continue
                count += 1
                registers[reg] = value
                if reg in (1, 0x0f, 0x14, 0x18, 0x19, 0x1b): modes[reg].add(value)
                if ((reg == 0x0f and value & 128) or (reg == 0x14 and value & 128) or
                    (reg == 1 and value)):
                    unsupported.append({"cycle": cycle, "register": reg, "value": value})
                if reg == 0x19:
                    if value & 128: pmd = value & 127
                    else: amd = value
                if 0x38 <= reg < 0x40 and value & 0x73:
                    lfo_configs.add((reg & 7, value))
                if reg == 8:
                    channel, mask = value & 7, (value >> 3) & 15
                    keys[channel] = mask
                    if mask:
                        ever_keyed.add(channel)
                        channel_keys[channel] += 1
                        masks[mask] += 1
                        algorithms[registers[0x20 + channel] & 7] += 1
                        feedbacks[(registers[0x20 + channel] >> 3) & 7] += 1
                        patch = [registers[0x20 + channel] & 0x3f]
                        for op in range(4):
                            offset = op * 8 + channel
                            dt2s[registers[0xc0 + offset] >> 6] += 1
                            keyscales[registers[0x80 + offset] >> 6] += 1
                            patch.extend(registers[group + offset] for group in (0x40,0x80,0xa0,0xc0,0xe0))
                        patch_configs.add(tuple(patch))
                for channel in range(8):
                    sens = registers[0x38 + channel]
                    modulating = bool((pmd and sens & 0x70) or (amd and sens & 3))
                    if modulating and keys[channel]: keyed_lfo_states += 1
                    if modulating and channel in ever_keyed: configured_lfo_on_ever_keyed.add(channel)
        assert origin is not None and end is not None
        if hashlib.sha256(path.read_bytes()).hexdigest() != before_sha:
            raise ValueError('Reference trace changed during the audit')
        report["tracks"].append({
            "command": command, "trace": str(path.resolve()), "trace_sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
            "origin_cycle": origin, "end_cycle": end, "observed_seconds": (end - origin) / 3579545,
            "ym_data_writes_including_boot": count, "keyons_by_channel": dict(channel_keys),
            "key_masks": dict(masks), "algorithms_at_keyon": dict(algorithms),
            "feedback_at_keyon": dict(feedbacks), "dt2_at_keyon": dict(dt2s),
            "key_scale_at_keyon": dict(keyscales), "distinct_keyon_configs_excluding_TL": len(patch_configs),
            "special_register_values": {f"0x{r:02X}": sorted(v) for r, v in modes.items()},
            "nonzero_lfo_sensitivity_configs": sorted(lfo_configs),
            "modulating_lfo_observed_while_keyed": keyed_lfo_states,
            "modulating_lfo_on_ever_keyed_channels": sorted(configured_lfo_on_ever_keyed),
            "unsupported_noise_csm_test_writes": unsupported,
        })
    report["tracks_count"] = len(report["tracks"])
    report["ym_writes"] = sum(t["ym_data_writes_including_boot"] for t in report["tracks"])
    report["unsupported_writes"] = sum(len(t["unsupported_noise_csm_test_writes"]) for t in report["tracks"])
    report["modulating_lfo_on_ever_keyed_channels"] = sum(len(t["modulating_lfo_on_ever_keyed_channels"]) for t in report["tracks"])
    a.out.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k: v for k, v in report.items() if k != "tracks"}, indent=2))


if __name__ == "__main__":
    main()
