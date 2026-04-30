#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

"""
E2E test runner for OpenRTX.

Takes a test script file containing emulator shell commands (key, sleep,
screenshot, quit, etc.) and runs it against the linux emulator.  Any
"screenshot <name>.bmp" commands in the script automatically become
assertions: the captured image is compared pixel-for-pixel against a
golden reference in tests/e2e/golden/<base_test>/<variant>/<name>.bmp.

The base test name is derived from the script filename by stripping a
trailing _<variant> suffix when present (e.g. about_default.txt with
variant "default" resolves to golden dir golden/about/default/).

Run all tests (default):
    python scripts/run_e2e.py
    python scripts/run_e2e.py --build-dir build_linux_debug
    python scripts/run_e2e.py -j 4    # limit parallelism

Single test:
    python scripts/run_e2e.py tests/e2e/about_default.txt --variant default
    python scripts/run_e2e.py tests/e2e/main_menu.txt --binary ./build/bin

Common flags:
    --update-golden    overwrite golden images with current output
    --tolerance N      max differing pixels before failure (default: 0)
"""

import argparse
import io
import os
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

from PIL import Image, ImageChops
from tqdm import tqdm

PROJECT_ROOT = Path(__file__).resolve().parent.parent
E2E_DIR = PROJECT_ROOT / "tests" / "e2e"

# Variant name -> binary filename (relative to build dir)
VARIANTS = {
    "default": "openrtx_linux",
    "module17": "openrtx_linux_mod17",
}


def load_normalized(path):
    """Load an image and normalize to RGB so byte comparisons are
    consistent regardless of source mode (e.g. RGB vs RGBA, palette)."""
    return Image.open(path).convert("RGB")


def compare_images(actual_img, golden_img):
    """Compare two already-loaded RGB images, return count of differing
    pixels.  Raises ValueError on size mismatch."""
    if actual_img.size != golden_img.size:
        raise ValueError(
            f"Size mismatch: actual {actual_img.size}"
            f" vs golden {golden_img.size}"
        )

    diff = ImageChops.difference(actual_img, golden_img)
    bbox = diff.getbbox()
    if bbox is None:
        return 0

    # Reduce per-pixel RGB diff to a single channel: nonzero where any
    # channel differs.  Then count nonzero pixels.
    mask = diff.convert("L").point(lambda v: 255 if v else 0, mode="L")
    return sum(1 for px in mask.getdata() if px)


def generate_diff_image(actual_img, golden_img, diff_path):
    """Generate a diff image highlighting differing pixels in red."""
    diff = ImageChops.difference(actual_img, golden_img)
    mask = diff.convert("L").point(lambda v: 255 if v else 0, mode="L")
    red = Image.new("RGB", actual_img.size, (255, 0, 0))
    out = Image.composite(red, Image.new("RGB", actual_img.size), mask)
    out.save(diff_path)


def resolve_base_name(script_name, variant):
    """Strip trailing _<variant> suffix from the script name."""
    suffix = f"_{variant}"
    if script_name.endswith(suffix):
        return script_name[: -len(suffix)]
    return script_name


def is_safe_screenshot_name(name):
    """Reject anything that isn't a plain basename in the tmpdir."""
    if not name or name != Path(name).name:
        return False
    if name.startswith(".") or name in ("", ".", ".."):
        return False
    return True


def run_test(script_path, binary, variant, tolerance, update_golden,
             build_dir, log=print, wrapper=None):
    """Run a single e2e test.  Returns True on success.

    All progress and diagnostic output goes through the ``log`` callable
    so callers (e.g. parallel runners) can buffer per-test output and
    avoid interleaving.

    ``wrapper`` is an optional list of argv tokens inserted between
    faketime and the emulator binary, e.g. ``["gdb", "--batch", "-ex",
    "run", "-ex", "bt", "--args"]`` or ``["valgrind"]``.
    """
    test_name = script_path.stem
    base_name = resolve_base_name(test_name, variant)
    golden_dir = E2E_DIR / "golden" / base_name / variant
    fail_dir = build_dir / "e2e_failures" / base_name / variant

    def _preserve_partial_screenshots(tmpdir, screenshots):
        """Copy any screenshots that were produced before a crash/timeout
        into fail_dir so they can be inspected after tmpdir is wiped."""
        produced = [
            (n, tmpdir / n) for n in screenshots if (tmpdir / n).is_file()
        ]
        if not produced:
            return
        fail_dir.mkdir(parents=True, exist_ok=True)
        for name, path in produced:
            shutil.copy2(path, fail_dir / f"actual_{name}")
        log(f"  Partial screenshots saved to: {fail_dir}")
        for name, _ in produced:
            log(f"    {fail_dir / f'actual_{name}'}")

    with tempfile.TemporaryDirectory() as tmpdir_str:
        tmpdir = Path(tmpdir_str)

        # Parse test script, rewrite screenshot paths into tmpdir
        screenshots = []
        rewritten_lines = []

        with open(script_path) as f:
            for lineno, line in enumerate(f, 1):
                line = line.rstrip("\n")
                m = re.match(r"^\s*screenshot\s+(.+)$", line)
                if m:
                    name = m.group(1).strip()
                    if not is_safe_screenshot_name(name):
                        log(
                            f"FAIL: {script_path.name}:{lineno}: unsafe"
                            f" screenshot name {name!r} (must be a plain"
                            f" basename)"
                        )
                        return False
                    screenshots.append(name)
                    rewritten_lines.append(
                        f"screenshot {tmpdir / name}"
                    )
                else:
                    rewritten_lines.append(line)

        rewritten_script = "\n".join(rewritten_lines) + "\n"

        # Give each test its own codeplug so parallel runs don't collide
        codeplug = PROJECT_ROOT / "default.rtxc"
        if codeplug.is_file():
            shutil.copy2(codeplug, tmpdir / "default.rtxc")

        log(f"Running E2E test: {base_name} ({variant})")

        env = {
            **os.environ,
            "TZ": "UTC",
            "XDG_STATE_HOME": str(tmpdir / "state"),
            "SDL_VIDEODRIVER": "offscreen",
            "SDL_AUDIODRIVER": "dummy",
        }

        cmd = ["faketime", "-f", "2000-01-01 00:00:00"]
        if wrapper:
            cmd += list(wrapper)
        cmd.append(str(binary))

        try:
            result = subprocess.run(
                cmd,
                input=rewritten_script,
                capture_output=True,
                text=True,
                timeout=30,
                cwd=str(tmpdir),
                env=env,
            )
        except subprocess.TimeoutExpired as e:
            log("FAIL: emulator timed out after 30s")
            if e.stderr:
                stderr = e.stderr
                if isinstance(stderr, bytes):
                    stderr = stderr.decode(errors="replace")
                log("  --- emulator stderr ---")
                log(stderr.rstrip())
                log("  -----------------------")
            _preserve_partial_screenshots(tmpdir, screenshots)
            return False

        if result.returncode != 0:
            log(f"FAIL: emulator exited with code {result.returncode}")
            if result.stderr:
                log("  --- emulator stderr ---")
                log(result.stderr.rstrip())
                log("  -----------------------")
            _preserve_partial_screenshots(tmpdir, screenshots)
            return False

        # -- compare screenshots ------------------------------------------

        failures = 0

        for name in screenshots:
            actual = tmpdir / name
            golden = golden_dir / name

            if not actual.is_file():
                log(f"  FAIL: screenshot not produced: {name}")
                if result.stderr:
                    log("    --- emulator stderr ---")
                    log(result.stderr.rstrip())
                    log("    -----------------------")
                failures += 1
                continue

            if update_golden:
                golden_dir.mkdir(parents=True, exist_ok=True)
                shutil.copy2(actual, golden)
                log(f"  updated golden: {name}")
                continue

            if not golden.is_file():
                log(f"  FAIL: golden image missing: {golden}")
                log("    (run with --update-golden to create)")
                failures += 1
                continue

            try:
                actual_img = load_normalized(actual)
                golden_img = load_normalized(golden)
                diff_pixels = compare_images(actual_img, golden_img)
            except Exception as e:
                log(f"  FAIL: {name} -- comparison error: {e}")
                failures += 1
                continue

            if diff_pixels <= tolerance:
                log(f"  PASS: {name} ({diff_pixels} pixels differ)")
            else:
                log(
                    f"  FAIL: {name} -- {diff_pixels} pixels differ"
                    f" (tolerance: {tolerance})"
                )
                fail_dir.mkdir(parents=True, exist_ok=True)
                shutil.copy2(actual, fail_dir / f"actual_{name}")
                shutil.copy2(golden, fail_dir / f"expected_{name}")
                try:
                    generate_diff_image(
                        actual_img, golden_img,
                        fail_dir / f"diff_{name}",
                    )
                except Exception as e:
                    log(f"    (diff image generation failed: {e})")
                log(f"    actual:   {fail_dir / f'actual_{name}'}")
                log(f"    expected: {fail_dir / f'expected_{name}'}")
                log(f"    diff:     {fail_dir / f'diff_{name}'}")
                failures += 1

        if failures > 0:
            log(f"  Snapshot failures saved to: {fail_dir}")
            return False

        log("  All assertions passed.")
        return True


def discover_tests(build_dir):
    """Discover test scripts and yield (script_path, binary, variant)
    tuples.

    Convention: if a script name ends with _<variant>, it is
    variant-specific and only runs with that variant.  Otherwise it is
    shared and runs with every known variant.
    """
    scripts = sorted(E2E_DIR.glob("*.txt"))

    for script in scripts:
        name = script.stem
        matched_variant = None
        for v in VARIANTS:
            if name.endswith(f"_{v}"):
                matched_variant = v
                break

        if matched_variant:
            binary = build_dir / VARIANTS[matched_variant]
            yield script, binary, matched_variant
        else:
            for v, bin_name in VARIANTS.items():
                yield script, build_dir / bin_name, v


def check_faketime():
    """Verify faketime is available on PATH."""
    if not shutil.which("faketime"):
        print("FAIL: 'faketime' not found on PATH", file=sys.stderr)
        sys.exit(1)


def check_binary(binary):
    """Verify binary exists and is executable."""
    if not os.access(binary, os.X_OK):
        print(
            f"FAIL: binary not found or not executable: {binary}",
            file=sys.stderr,
        )
        sys.exit(1)


# Marker that indicates the binary was built with -Dtest_version=e2e-test.
# Baked into GIT_VERSION and rendered on the Info screen, so it must
# appear verbatim in the binary's rodata.
TEST_VERSION_MARKER = b"e2e-test"


def warn_if_missing_test_version(binary):
    """Warn (non-fatally) when the binary was not built with
    -Dtest_version=e2e-test.  Info-screen goldens rely on the fixed
    version string, so info_* tests will fail otherwise."""
    try:
        with open(binary, "rb") as f:
            data = f.read()
    except OSError:
        # Not worth failing over; check_binary will catch real issues.
        return

    if TEST_VERSION_MARKER in data:
        return

    print(
        f"WARNING: {binary} was not built with -Dtest_version=e2e-test;"
        f" info_* tests will fail.\n"
        f"  Reconfigure with:"
        f" meson setup --reconfigure <build_dir> -Dtest_version=e2e-test",
        file=sys.stderr,
    )


def main():
    parser = argparse.ArgumentParser(
        description="OpenRTX E2E test runner"
    )
    parser.add_argument(
        "test_script",
        nargs="?",
        type=Path,
        help="Path to test script (.txt); omit to run all discovered"
             " tests",
    )
    parser.add_argument(
        "--binary",
        type=Path,
        help="Path to openrtx binary",
    )
    parser.add_argument(
        "--variant",
        default="default",
        help="UI variant (default: default)",
    )
    parser.add_argument(
        "--build-dir",
        type=Path,
        default=PROJECT_ROOT / "build_linux",
        help="Build directory containing emulator binaries"
             " (default: build_linux)",
    )
    parser.add_argument(
        "-j", "--jobs",
        type=int,
        default=os.cpu_count() or 1,
        help="Parallel test workers when running all tests"
             " (default: number of CPUs)",
    )
    parser.add_argument(
        "--tolerance",
        type=int,
        default=0,
        help="Max differing pixels before a screenshot fails"
             " (default: 0)",
    )
    parser.add_argument(
        "--update-golden",
        action="store_true",
        help="Overwrite golden images with current output",
    )
    parser.add_argument(
        "--wrapper",
        default="",
        help="Command (with args) to wrap the emulator with, e.g."
             " 'gdb --batch -ex run -ex bt --args' or 'valgrind'."
             "  Inserted between faketime and the binary."
             "  Must be non-interactive: stdin is piped to the test"
             " script and stdout/stderr are captured.",
    )
    args = parser.parse_args()

    tolerance = args.tolerance
    update_golden = args.update_golden
    wrapper = shlex.split(args.wrapper) if args.wrapper else None

    if not args.test_script:
        return run_all(args, tolerance, update_golden, wrapper)

    script_path = args.test_script.resolve()
    build_dir = args.build_dir.resolve()

    if args.binary:
        binary = args.binary.resolve()
    else:
        binary = build_dir / VARIANTS.get(args.variant, "openrtx_linux")

    check_faketime()
    check_binary(binary)
    warn_if_missing_test_version(binary)

    if not script_path.is_file():
        print(
            f"FAIL: test script not found: {script_path}",
            file=sys.stderr,
        )
        sys.exit(1)

    ok = run_test(
        script_path, binary, args.variant, tolerance, update_golden,
        build_dir, wrapper=wrapper,
    )
    sys.exit(0 if ok else 1)


def run_all(args, tolerance, update_golden, wrapper=None):
    """Discover and run every e2e test, in parallel via a thread pool.

    Each test runs in its own tmpdir + env, so concurrent execution is
    safe.  Per-test output is buffered and flushed atomically through
    ``tqdm.write`` so the progress bar stays clean.
    """
    build_dir = args.build_dir.resolve()
    check_faketime()

    tests = list(discover_tests(build_dir))
    if not tests:
        print("No e2e test scripts found.", file=sys.stderr)
        sys.exit(1)

    # Pre-partition runnable vs skipped so the progress bar reflects
    # only work we'll actually do.
    runnable = []
    skipped_names = []
    warned_binaries = set()
    for script, binary, variant in tests:
        base = resolve_base_name(script.stem, variant)
        if not os.access(binary, os.X_OK):
            skipped_names.append(
                (f"{base} ({variant})", str(binary))
            )
            continue
        if binary not in warned_binaries:
            warn_if_missing_test_version(binary)
            warned_binaries.add(binary)
        runnable.append((script, binary, variant, base))

    for name, path in skipped_names:
        print(f"SKIP: {name} (binary not found: {path})")

    if not runnable:
        print("\nNo runnable tests.")
        sys.exit(1)

    jobs = max(1, min(args.jobs, len(runnable)))

    passed = 0
    failed = 0
    failed_names = []

    def _worker(item):
        script, binary, variant, base = item
        buf = io.StringIO()
        ok = run_test(
            script, binary, variant, tolerance, update_golden,
            build_dir, log=lambda msg="": print(msg, file=buf),
            wrapper=wrapper,
        )
        return base, variant, ok, buf.getvalue()

    with ThreadPoolExecutor(max_workers=jobs) as ex:
        futures = [ex.submit(_worker, item) for item in runnable]
        bar = tqdm(
            total=len(runnable),
            desc=f"e2e (j={jobs})",
            unit="test",
            dynamic_ncols=True,
        )
        try:
            for fut in as_completed(futures):
                base, variant, ok, output = fut.result()
                status = "PASS" if ok else "FAIL"
                tqdm.write(f"[{status}] {base} ({variant})")
                if not ok:
                    if output.strip():
                        tqdm.write(output.rstrip())
                    failed += 1
                    failed_names.append(f"{base} ({variant})")
                else:
                    passed += 1
                bar.update(1)
        except KeyboardInterrupt:
            tqdm.write("\nInterrupted; cancelling pending tests...")
            for fut in futures:
                fut.cancel()
            ex.shutdown(wait=False, cancel_futures=True)
            sys.exit(130)
        finally:
            bar.close()

    total = len(tests)
    skipped = len(skipped_names)
    print()
    print(
        f"{passed} passed, {failed} failed, {skipped} skipped,"
        f" {total} total"
    )

    if failed_names:
        print("\nFailed tests:")
        for name in failed_names:
            print(f"  {name}")

    if skipped_names:
        print("\nSkipped tests:")
        for name, _ in skipped_names:
            print(f"  {name}")

    sys.exit(0 if failed == 0 else 1)


if __name__ == "__main__":
    main()
