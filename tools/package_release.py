#!/usr/bin/env python3
"""Package a selected frozen variant into separate source and disc ZIPs.

Runtime sources/Jo/COMMON/makefiles come from --variant. Support tools, tests,
docs, README, requirements and asset-lock come from --project-root; the source
manifest records every origin and digest. The actual post-build jo_options.txt
is retained verbatim, separately from the pre-build source-identity snapshot.
No release date or production-ready declaration is generated. --dry-run checks
the inputs and hashes without creating archives. No compiler or emulator runs.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import sys
import tempfile
import zipfile

import extract_disc_assets as disc_assets

ROOT = Path(__file__).resolve().parents[1]
CANONICAL = ("ROM68K.BIN", "SPRITES.PAK", "FONT8.BIN", "R18RING.BIN",
             "R18MENU.BIN", "R18MAP1.BIN", "R18MAP2.BIN", "R18MAP4.BIN", "R18MAP5.BIN", "R18MAP6.BIN")
RUNTIME_DIRS = ("core", "port", "sdk/jo_engine", "sdk/Compiler/COMMON")
SUPPORT_DIRS = ("tools", "tests", "docs")
ROOT_SUPPORT = ("README.md", "requirements-assets.txt", "asset-lock.json")
SOURCE_SUFFIXES = {".c", ".h", ".cc", ".cpp", ".hpp", ".s", ".inc", ".py", ".sh",
                   ".bat", ".mk", ".ld", ".linker", ".json", ".md", ".txt", ".csv",
                   ".scsp", ".dsp", ".cbp", ".cppcheck", ".yml", ".yaml", ".html",
                   ".css", ".js", ".svg", ".png"}
SOURCE_NAMES = {"Makefile", "makefile", "jo_engine_makefile", "README", "LICENSE", "COPYING", "NOTICE"}
SKIP_DIRS = {"build", "variants", "cd", "__pycache__", ".git", ".venv", "node_modules", ".pytest_cache"}
OPTIONS = "sdk/jo_engine/jo_options.txt"
MAX_EVIDENCE_BYTES = 16 * 1024 * 1024
ACTOR_EVIDENCE_SUFFIX = "-terminal-actors.bin"
ACTOR_EVIDENCE_BYTES = 2412


class PackageError(ValueError):
    pass


def require(condition, message):
    if not condition:
        raise PackageError(message)


def identity(path):
    digest, count = hashlib.sha256(), 0
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
            count += len(chunk)
    return {"bytes": count, "sha256": digest.hexdigest()}


def encoded(value):
    return (json.dumps(value, indent=2, sort_keys=True) + "\n").encode("utf-8")


def checked_member(name):
    require(isinstance(name, str) and name and "\\" not in name and ":" not in name and
            all(ord(character) >= 32 and ord(character) != 127 for character in name) and
            all(part not in ("", ".", "..") for part in name.split("/")),
            f"Unsafe ZIP member name: {name!r}")
    return name


def source_file(path, archive_name):
    return (archive_name == "sdk/Compiler/COMMON/IP.BIN" or path.name in SOURCE_NAMES or
            path.name.startswith("LICENSE.") or path.suffix.lower() in SOURCE_SUFFIXES)


def allowed_evidence(path, size):
    return (path.suffix.lower() in (".json", ".log", ".png") or
            (path.name.endswith(ACTOR_EVIDENCE_SUFFIX) and size == ACTOR_EVIDENCE_BYTES))


def collect_tree(base, relative, origin, selection):
    folder = base / relative
    require(folder.is_dir() and not folder.is_symlink(), f"Missing regular source directory: {folder}")
    for parent, folders, files in os.walk(folder, followlinks=False):
        parent = Path(parent)
        folders[:] = sorted(name for name in folders if name not in SKIP_DIRS and
                            not (parent / name).is_symlink() and not name.startswith("."))
        for name in sorted(files):
            path = parent / name
            archive_name = path.relative_to(base).as_posix()
            if name.startswith(".") or path.is_symlink() or not path.is_file() or not source_file(path, archive_name):
                continue
            selection[archive_name] = (path, origin)


def cue_bytes(path, new_name):
    lines = path.read_bytes().decode("ascii").splitlines(keepends=True)
    files = []
    tracks = []
    for index, line in enumerate(lines):
        text = line.rstrip("\r\n")
        if re.match(r"\s*FILE\b", text, re.I):
            match = re.fullmatch(r'\s*FILE\s+"([^"]+)"\s+BINARY\s*', text, re.I)
            require(match is not None, "Unsupported CUE FILE declaration")
            files.append((index, match.group(1)))
        if re.match(r"\s*TRACK\b", text, re.I):
            tracks.append(text)
    require(len(files) == 1 and files[0][1].lower() == "game.bin", "CUE must reference only the variant's game.bin")
    require(len(tracks) == 1 and re.fullmatch(r"\s*TRACK\s+01\s+MODE1/2352\s*", tracks[0], re.I),
            "Only a single MODE1/2352 data track is supported")
    require(any(re.fullmatch(r"\s*INDEX\s+01\s+00:00:00\s*", line.rstrip("\r\n"), re.I) for line in lines),
            "CUE data track must begin at LBA 0")
    lines[files[0][0]] = f'FILE "{new_name}.bin" BINARY\n'
    return "".join(lines).encode("ascii")


def checked_disc(variant, lock_path):
    image, raw = variant / "game.bin", variant / "game.raw"
    require(image.is_file() and raw.is_file() and not image.is_symlink() and not raw.is_symlink(),
            "Frozen variant requires regular game.bin and game.raw files")
    try:
        lock = disc_assets.load_lock(lock_path)
        result = disc_assets.extract_assets(image, lock_path, variant / "unused-verification-output", True)
        require(result["format"] == "mode1-2352", "Variant BIN does not match MODE1/2352 CUE")
        with disc_assets.DiscImage(image) as disc:
            entries = disc.root_entries()
            require(set(entries) == set(lock["files"]) | {"0.BIN"},
                    "Disc contains files outside asset-lock plus the build executable")
            require(all(not entry.flags & 2 for entry in entries.values()), "Unexpected disc subdirectory")
            digest = hashlib.sha256()
            for chunk in disc.stream(entries["0.BIN"]):
                digest.update(chunk)
            program = identity(raw)
            require(entries["0.BIN"].size == program["bytes"] and digest.hexdigest() == program["sha256"],
                    "Disc 0.BIN does not match the frozen game.raw")
    except disc_assets.AssetError as error:
        raise PackageError(str(error)) from error
    return lock, result, program


def zip_info(name, mode=0o644):
    # ZIP's fixed minimum timestamp makes repeated packaging deterministic;
    # it is format metadata, not a release date or a claimed build timestamp.
    info = zipfile.ZipInfo(checked_member(name))
    info.compress_type = zipfile.ZIP_DEFLATED
    info._compresslevel = 6
    info.create_system = 3
    info.external_attr = (0o100000 | mode) << 16
    return info


def add_path(archive, name, path, expected):
    digest, count = hashlib.sha256(), 0
    mode = 0o755 if path.stat().st_mode & 0o111 else 0o644
    with path.open("rb") as source, archive.open(zip_info(name, mode), "w", force_zip64=True) as target:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            target.write(chunk)
            digest.update(chunk)
            count += len(chunk)
    require(count == expected["bytes"] and digest.hexdigest() == expected["sha256"],
            f"Input changed while packaging: {path}")


def package_release(project, variant, canonical, output_dir, name="WrestleFest_Saturn", evidence=(), dry_run=False):
    project, variant, canonical, output_dir = map(lambda p: Path(p).resolve(), (project, variant, canonical, output_dir))
    require(re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_-]{0,79}", name) is not None, "Invalid package name")
    require(variant.is_dir() and variant != project, "Select a separate frozen variant directory")
    selection = {}
    for relative in RUNTIME_DIRS:
        collect_tree(variant, relative, "frozen_variant", selection)
    for relative in ("makefile", "core_sources.mk", "source-identity.json"):
        path = variant / relative
        require(path.is_file() and not path.is_symlink(), f"Missing frozen file: {relative}")
        selection[relative] = (path, "frozen_variant")
    require(OPTIONS in selection, "Frozen build options are missing")
    for relative in SUPPORT_DIRS:
        collect_tree(project, relative, "support_tree", selection)
    for relative in ROOT_SUPPORT:
        path = project / relative
        require(path.is_file() and not path.is_symlink(), f"Missing support file: {relative}")
        selection[relative] = (path, "support_tree")
    for asset in CANONICAL:
        path = canonical / asset
        require(path.is_file() and not path.is_symlink(), f"Missing canonical source asset: {asset}")
        selection["source_assets/" + asset] = (path, "canonical_assets")
    evidence_total = 0
    evidence_base = (project / "evidence").resolve()
    for requested in evidence:
        requested = Path(requested)
        path = requested if requested.is_absolute() else project / requested
        require(path.is_file() and not path.is_symlink(), f"Evidence must be a regular file: {requested}")
        size = path.stat().st_size
        require(allowed_evidence(path, size),
                f"Evidence must be JSON/log/PNG or *{ACTOR_EVIDENCE_SUFFIX} of exactly "
                f"{ACTOR_EVIDENCE_BYTES} bytes: {requested}")
        try:
            relative = path.resolve().relative_to(evidence_base)
        except ValueError as error:
            raise PackageError("Evidence must be inside the project's evidence directory") from error
        evidence_total += size
        require(evidence_total <= MAX_EVIDENCE_BYTES, "Selected evidence exceeds the 16 MiB budget; select smaller summaries")
        selection["evidence/" + relative.as_posix()] = (path, "selected_evidence")

    for relative in selection:
        checked_member(relative)
    files = {relative: dict(identity(path), origin=origin) for relative, (path, origin) in sorted(selection.items())}
    # Also check the bytes actually hashed: a file that changes after selection
    # must not expand the narrow binary exception or the evidence budget.
    hashed_evidence_bytes = 0
    for relative, row in files.items():
        if row["origin"] == "selected_evidence":
            require(allowed_evidence(Path(relative), row["bytes"]),
                    f"Evidence changed size or type while hashing: {relative}")
            hashed_evidence_bytes += row["bytes"]
    require(hashed_evidence_bytes <= MAX_EVIDENCE_BYTES,
            "Selected evidence exceeds the 16 MiB budget after hashing")
    frozen = json.loads((variant / "source-identity.json").read_text(encoding="utf-8"))
    require(isinstance(frozen, dict), "Invalid pre-build source identity")
    checked = 0
    for relative, expected in frozen.items():
        checked_member(relative)
        require(isinstance(expected, str) and re.fullmatch(r"[0-9a-f]{64}", expected) is not None,
                f"Invalid frozen source digest: {relative}")
        if not source_file(Path(relative), relative):
            continue  # Build objects are excluded even if an older snapshot listed them.
        require(relative in files and files[relative]["origin"] == "frozen_variant",
                f"Frozen source is missing from the package: {relative}")
        if relative != OPTIONS:
            require(files[relative]["sha256"] == expected, f"Frozen source identity mismatch: {relative}")
            checked += 1
    flags = (variant / OPTIONS).read_bytes().decode("utf-8")
    rewritten_cue = cue_bytes(variant / "game.cue", name)
    lock, disc_result, program = checked_disc(variant, project / "asset-lock.json")
    for asset in CANONICAL:
        if asset in lock["files"]:
            require(all(files["source_assets/" + asset][key] == lock["files"][asset][key] for key in ("bytes", "sha256")),
                    f"Canonical source asset differs from the disc lock: {asset}")
    disc_identity = identity(variant / "game.bin")
    source_manifest = {
        "format": "wf-jo-source-package", "version": 1, "variant": variant.name,
        "files": files, "build_options_verbatim": flags,
        "build_options_scope": "Jo's recorded compile options; link rules are retained in the frozen makefiles.",
        "build_options_sha256": files[OPTIONS]["sha256"],
        "prebuild_identity_verified_files": checked,
        "prebuild_options_sha256": frozen.get(OPTIONS),
        "options_note": "jo_options.txt is generated during compilation; its packaged bytes are the actual post-build flags, not the earlier snapshot.",
        "origins": {"frozen_variant": "Selected frozen runtime, Jo/COMMON and makefiles",
                    "support_tree": "Current project tools/tests/docs/README/requirements/asset-lock",
                    "canonical_assets": f"{len(CANONICAL)} explicitly named canonical graphics/program source assets",
                    "selected_evidence": "Explicit JSON/log/PNG selection; full emulator states are excluded"},
        "disc_image": disc_identity, "disc_program": program,
        "locked_asset_files": disc_result["files"], "locked_asset_bytes": disc_result["bytes"],
    }
    disc_manifest = {"format": "wf-jo-disc-package", "version": 1, "variant": variant.name,
                     "files": {name + ".bin": disc_identity,
                               name + ".cue": {"bytes": len(rewritten_cue), "sha256": hashlib.sha256(rewritten_cue).hexdigest()}},
                     "disc_program": program, "asset_lock_sha256": files["asset-lock.json"]["sha256"],
                     "build_options_verbatim": flags}
    if dry_run:
        return {"variant": variant.name, "source_files": len(files), "source_bytes": sum(row["bytes"] for row in files.values()),
                "disc_bytes": disc_identity["bytes"], "selected_evidence_bytes": evidence_total,
                "locked_asset_files": disc_result["files"], "build_options_verbatim": flags, "archives_created": False}

    output_dir.mkdir(parents=True, exist_ok=True)
    source_name, disc_name = name + "_source.zip", name + "_disc.zip"
    with tempfile.TemporaryDirectory(prefix=".wf-package-", dir=output_dir) as temporary:
        staged = Path(temporary)
        prefix = name + "_source/"
        with zipfile.ZipFile(staged / source_name, "w", allowZip64=True) as archive:
            for relative, (path, _) in sorted(selection.items()):
                add_path(archive, prefix + relative, path, files[relative])
            archive.writestr(zip_info(prefix + "source-manifest.json"), encoded(source_manifest))
        with zipfile.ZipFile(staged / disc_name, "w", allowZip64=True) as archive:
            add_path(archive, name + ".bin", variant / "game.bin", disc_identity)
            archive.writestr(zip_info(name + ".cue"), rewritten_cue)
            archive.writestr(zip_info("disc-manifest.json"), encoded(disc_manifest))
        result = {"format": "wf-jo-package-pair", "version": 1, "variant": variant.name,
                  "source_archive": dict(identity(staged / source_name), filename=source_name),
                  "disc_archive": dict(identity(staged / disc_name), filename=disc_name),
                  "build_options_verbatim": flags, "asset_lock_sha256": files["asset-lock.json"]["sha256"]}
        manifest_name = name + "_manifest.json"
        (staged / manifest_name).write_bytes(encoded(result))
        for filename in (source_name, disc_name, manifest_name):
            os.replace(staged / filename, output_dir / filename)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--variant", required=True, type=Path, help="Frozen directory, or its name under build/variants")
    parser.add_argument("--project-root", type=Path, default=ROOT)
    parser.add_argument("--source-assets", type=Path, default=ROOT.parent / "research/runtime/assets")
    parser.add_argument("--output-dir", type=Path, default=ROOT.parent / "deliverables")
    parser.add_argument("--name", default="WrestleFest_Saturn")
    parser.add_argument("--evidence", action="append", type=Path, default=[],
                        help="Project JSON/log/PNG or *-terminal-actors.bin of exactly 2412 bytes; repeat to select; maximum 16 MiB total")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    variant = args.variant
    if not variant.is_absolute() and not variant.is_dir():
        variant = args.project_root / "build/variants" / variant
    try:
        result = package_release(args.project_root, variant, args.source_assets, args.output_dir,
                                 args.name, args.evidence, args.dry_run)
        print(json.dumps(result, indent=2))
    except (PackageError, disc_assets.AssetError, OSError, UnicodeError, json.JSONDecodeError) as error:
        parser.exit(1, f"Packaging failed: {error}\n")


if __name__ == "__main__":
    main()
