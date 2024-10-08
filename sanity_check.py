#!/usr/bin/env python3
import re
import sys
import argparse
from pathlib import Path
import zipfile

BYTES_IN_MB = 1024 * 1024

# config
MAX_ARCHIVE_SIZE = 5 * BYTES_IN_MB
MAX_UNCOMPRESSED_SIZE = 50 * BYTES_IN_MB


def print_banner(line):
    print("=" * len(line))
    print(line)
    print("=" * len(line))


def fail(msg):
    print_banner("Sanity check failed. :-(")
    print(msg)
    sys.exit(1)


def check_archive_size(archive_path):
    archive_size = archive_path.stat().st_size
    if archive_size > MAX_ARCHIVE_SIZE:
        size_in_mb = archive_size / BYTES_IN_MB
        fail(
            "ZIP archive too large "
            + f"(max {MAX_ARCHIVE_SIZE // BYTES_IN_MB}MB, "
            + f"got {size_in_mb:.2f}MB)"
        )


def check_uncompressed_size(archive_path):
    with zipfile.ZipFile(archive_path, "r") as archive_file:
        total_size = 0
        for file_info in archive_file.infolist():
            total_size += file_info.file_size
        if total_size > MAX_UNCOMPRESSED_SIZE:
            fail(
                "Uncompressed size is too large "
                + f"(max {MAX_UNCOMPRESSED_SIZE // BYTES_IN_MB}B, "
                + f"got {total_size:.2f}MB)"
            )


def check_exploit_files(root_path):
    missing_files = []

    levels_count = 0
    while (next_level_dir := root_path / f"level{levels_count + 1}").is_dir():
        levels_count += 1

        exploit_path = next_level_dir / "exploit.sh"
        if not exploit_path.exists() or not exploit_path.is_file():
            missing_files.append(exploit_path)

        readme_path = next_level_dir / "README"
        if not readme_path.exists() or not readme_path.is_file():
            missing_files.append(readme_path)

    return (missing_files, levels_count)


def check_readme_file(archive_root):
    readme_path = archive_root / "README"
    # Changed in version 3.9: Added support for text and binary modes for open.
    open_mode = "rb" if sys.version_info.minor >= 9 else "r"
    with readme_path.open(open_mode) as readme_file:
        lines = [line.decode().strip() for line in readme_file]

    if len(lines) != 5:
        fail(f"invalid README: expected 5 lines, got {len(lines)}")

    hackerhandle, name, email, vunetid, studentid = lines

    if not re.match(r"^[a-z][a-z0-9]{2,15}$", hackerhandle):
        fail(
            f"Invalid hacker handle in README: {hackerhandle}\n"
            "\t- should only contain lowercase alphanumeric characters\n"
            "\t- should be 3-16 characters\n"
            "\t- should start with a lowercase character"
        )

    if not re.match(r"^[^@]*@.+\.[^.]+$", email):
        fail(f"Invalid email in README: {email}")

    if not re.match(r"^[a-z]{3}[0-9]{3}$", vunetid):
        fail(f"Invalid vunetid in README: {vunetid}")


def main(args):
    if not zipfile.is_zipfile(args.archive_path):
        fail("The path provided does not point to a ZIP file")

    check_archive_size(args.archive_path)
    check_uncompressed_size(args.archive_path)

    archive_root = zipfile.Path(args.archive_path)

    (missing_files, levels_count) = check_exploit_files(archive_root)
    if levels_count == 0:
        fail("You are submitting an archive with no level folders!")

    if missing_files:
        missing_files_str = [str(file_path) for file_path in missing_files]
        fail(
            f"Please provide the following files: {missing_files_str}\n"
            "\t- make sure that the files are stored in the right path,\n"
            "\t  usually the root of the archive"
        )

    check_readme_file(archive_root)

    # all checks succeeded
    print_banner(f"Sanity check passed. You are submitting {levels_count} levels!")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("archive_path", type=Path)
    args = parser.parse_args()

    main(args)
