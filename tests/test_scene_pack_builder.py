#!/usr/bin/env python3
"""Reproduce the measured D03 pack and independently verify all scene views."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest import mock
import zlib

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
import build_assets as builder
import extract_disc_assets as importer

EXPECTED_BYTES = 250985
EXPECTED_SHA256 = '478e0b8be09aa69d65abb31ea849eae60c46e47fe285766dceaa302ad8e1dc2d'
SOURCE = ROOT/'source_assets'
MEASURED = None


class ScenePackTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.original = [(SOURCE/name).read_bytes() for name in builder.SCENE_ASSETS]
        cls.packed = builder.build_scene_pack(cls.original)

    def test_measured_pack_identity(self):
        self.assertEqual(len(self.packed), EXPECTED_BYTES)
        self.assertEqual(hashlib.sha256(self.packed).hexdigest(), EXPECTED_SHA256)
        if MEASURED is not None:
            self.assertEqual(self.packed, MEASURED.read_bytes())

    def test_all_canonical_files_and_first_use_aliases(self):
        magic, version, size, files, blocks, at, total, checksum = struct.unpack_from('>4s7I', self.packed)
        self.assertEqual((magic, version, size, files, blocks), (b'WFSC', 2, 8192, 7, 102))
        self.assertEqual(total, len(self.packed)); self.assertLessEqual(total, 253952)
        self.assertEqual(checksum, zlib.adler32(self.packed[32:at]) & 0xffffffff)
        spans = [struct.unpack_from('>2I', self.packed, 32+7*16+i*8) for i in range(blocks)]
        physical = {}; source_spans = {}; first = 0; cursor = at
        for i, raw in enumerate(self.original):
            count = (len(raw)+8191)//8192
            self.assertEqual(struct.unpack_from('>4I', self.packed, 32+i*16), (len(raw), first, count, 0))
            decoded = []
            for n in range(count):
                start, end = span = spans[first+n]
                expected = raw[n*8192:(n+1)*8192]
                self.assertTrue(at <= start < end <= total)
                if span not in physical:
                    self.assertEqual(start, cursor); cursor = end
                    decoder = zlib.decompressobj()
                    physical[span] = decoder.decompress(self.packed[start:end])+decoder.flush()
                    self.assertTrue(decoder.eof)
                    self.assertFalse(decoder.unused_data or decoder.unconsumed_tail)
                self.assertEqual(physical[span], expected)
                self.assertEqual(source_spans.setdefault(expected, span), span)
                decoded.append(physical[span])
            self.assertEqual(b''.join(decoded), raw); first += count
        self.assertEqual((first, len(physical), cursor), (102, 88, total))

    def test_repeat_is_byte_identical(self):
        self.assertEqual(builder.build_scene_pack(self.original), self.packed)

    def test_legacy_zlib_cannot_silently_exceed_new_reservation(self):
        with self.assertRaisesRegex(ValueError, '253952'):
            builder.build_scene_pack(self.original, compressor='zlib')
        with self.assertRaisesRegex(ValueError, 'compressor'):
            builder.build_scene_pack(self.original, compressor='unknown')

    def test_oversized_cli_pack_is_rejected_before_output_writes(self):
        with tempfile.TemporaryDirectory(prefix='wf-scene-reject-') as temp:
            out = Path(temp)/'cd'
            argv = ['build_assets.py', '--source-dir', str(SOURCE), '--output-dir', str(out),
                    '--scene-compressor', 'zlib']
            with mock.patch.object(sys, 'argv', argv), \
                 mock.patch.object(builder, 'build_sprite_pack', return_value=b'unused'), \
                 self.assertRaisesRegex(ValueError, '253952'):
                builder.main()
            self.assertFalse(out.exists())

    def test_cli_manifest_and_defaults_preserve_sprite_and_music_contract(self):
        # The expensive sprite compressor is unchanged; use its measured output
        # while exercising the real CLI's complete byte verification and writes.
        sprites = (ROOT/'cd/SPRITES.WFP').read_bytes()
        with tempfile.TemporaryDirectory(prefix='wf-scene-builder-') as temp:
            out = Path(temp)
            argv = ['build_assets.py', '--source-dir', str(SOURCE), '--output-dir', str(out/'cd'),
                    '--manifest', str(out/'manifest.json')]
            with mock.patch.object(sys, 'argv', argv), \
                 mock.patch.object(builder, 'build_sprite_pack', return_value=sprites) as sprite_build, \
                 mock.patch.object(builder, 'build_scene_pack', return_value=self.packed) as scene_build:
                builder.main()
            sprite_build.assert_called_once(); scene_build.assert_called_once()
            self.assertEqual(sprite_build.call_args.args[1:], (4096, 'zopfli'))
            self.assertTrue(sprite_build.call_args.args[0] == (SOURCE/'SPRITES.PAK').read_bytes(), 'Sprite input changed')
            self.assertEqual(scene_build.call_args.args[1:], ('zopfli',))
            self.assertTrue(scene_build.call_args.args[0] == self.original, 'Scene input changed')
            self.assertEqual((out/'cd/SPRITES.WFP').read_bytes(), sprites)
            manifest = json.loads((out/'manifest.json').read_text())
            self.assertEqual(manifest['scene_compressor'], 'zopfli')
            self.assertEqual(manifest['scene_compressor_options'],
                             dict(numiterations=15, blocksplitting=1, blocksplittinglast=0, blocksplittingmax=15))
            self.assertEqual(manifest['resident_scene_capacity'], 253952)
            self.assertEqual((manifest['sprite_cache_bytes'], manifest['sprite_cache_blocks']), (610304, 149))
            self.assertEqual((manifest['sprite_cache_bytes_with_frontend'], manifest['sprite_cache_blocks_with_frontend']), (593920, 145))
            self.assertEqual(manifest['music_prefix_bytes'], 114688)
            self.assertEqual(manifest['music_prefix_workspace_bytes'], 0)

    def test_derived_lock_changes_only_scene_pack(self):
        legacy = importer.load_lock(ROOT/'docs/asset-lock-b12.json')
        current = importer.load_lock(ROOT/'asset-lock.json')
        old_scene = legacy['files']['SCENES.WFP']
        self.assertEqual(old_scene, dict(bytes=268356, sha256='f217d8e9781045afc25ec0ec86938607102ddff7fd040aeb8173b6c7fc5343e2'))
        self.assertEqual(current['files']['SCENES.WFP'], dict(bytes=EXPECTED_BYTES, sha256=EXPECTED_SHA256))
        self.assertEqual(current['total_bytes'], legacy['total_bytes']-old_scene['bytes']+EXPECTED_BYTES)
        self.assertEqual({k:v for k,v in legacy['files'].items() if k!='SCENES.WFP'},
                         {k:v for k,v in current['files'].items() if k!='SCENES.WFP'})


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-dir', type=Path, default=SOURCE)
    parser.add_argument('--measured-pack', type=Path)
    args, remaining = parser.parse_known_args()
    SOURCE, MEASURED = args.source_dir, args.measured_pack
    unittest.main(argv=[sys.argv[0], *remaining])
