"""Causal red test: real PCM3 enable, not just diagnostic ring counter."""
import unittest
import argparse
import sys
import profile_firmware as profile


class Deadline(unittest.TestCase):
    def test_future_published_music_start_meets_unchanged_soft_deadline(self):
        probe = profile.Probe(1)
        probe.setUp()
        try:
            probe.activate()
            for voice in range(3):
                self.assertEqual(probe.command(11, (voice << 12) | (1 << 11) | (20 << 4) | 3), 0)
            result = probe.feed(profile.events_for(2, 3, True), 2, 3)
            starts = [row for row in result["pcm3_controls"]
                      if row["address"] == 0x100710 and row["value"]]
            self.assertGreater(len(starts), 5)
            self.assertTrue(all(row["delay_fs"] >= 0 for row in starts), "early audible enable")
            self.assertEqual(result["pcm_engine_underruns"], 0)
            self.assertLessEqual(max(row["delay_fs"] for row in starts), 256,
                                 "PCM3 gain enable misses existing256Fs soft deadline")
        finally:
            probe.cpu.set_instr_hook_callback(None)
            probe.tearDown()


if __name__ == "__main__":
    parser=argparse.ArgumentParser()
    parser.add_argument('--build',type=profile.Path)
    parser.add_argument('--elf-sha')
    args,left=parser.parse_known_args()
    if args.build:
        profile.original.BUILD=args.build.resolve()
        assert args.elf_sha and profile.sha(profile.original.BUILD/'SCSP.ELF')==args.elf_sha
    unittest.main(argv=[sys.argv[0],*left],verbosity=2)
