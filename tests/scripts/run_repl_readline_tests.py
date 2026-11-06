#!/usr/bin/env python3
"""Exercise REPL search and line editing through a real POSIX terminal.

Usage: ZC=./zc python3 tests/scripts/run_repl_readline_tests.py
"""

import os
import re
import select
import subprocess
import tempfile
import time
import unittest
from pathlib import Path

if os.name == "posix":
    import pty


ANSI = re.compile(rb"\x1b\[[0-9;]*[A-Za-z]")
LINE = b"let total = 1;"


@unittest.skipUnless(os.name == "posix", "requires a POSIX pseudo-terminal")
class ReadlineTests(unittest.TestCase):
    def start_repl(self, history=(LINE,)):
        home_dir = tempfile.TemporaryDirectory(prefix="zenc-readline-")
        self.addCleanup(home_dir.cleanup)
        Path(home_dir.name, ".zenc_history").write_bytes(b"\n".join(history) + b"\n")
        master, slave = pty.openpty()
        self.addCleanup(os.close, master)
        try:
            proc = subprocess.Popen(
                [str(Path(os.environ.get("ZC", "./zc")).resolve()), "repl"],
                stdin=slave,
                stdout=slave,
                stderr=slave,
                env={**os.environ, "HOME": home_dir.name},
            )
        finally:
            os.close(slave)

        def stop():
            if proc.poll() is None:
                proc.kill()
            proc.wait(timeout=5)

        self.addCleanup(stop)
        self.master = master
        self.output = b""
        self.expect(b"zenc >>> ")
        return proc

    def expect(self, text):
        deadline = time.monotonic() + 5
        while True:
            clean = ANSI.sub(b"", self.output)
            if text in clean:
                self.output = clean.split(text, 1)[1]
                return
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                self.fail(f"expected {text!r}; terminal output was {clean!r}")
            ready, _, _ = select.select([self.master], [], [], remaining)
            if ready:
                chunk = os.read(self.master, 65536)
                if not chunk:
                    self.fail(f"REPL closed before displaying {text!r}")
                self.output += chunk

    def send(self, keys, expected):
        os.write(self.master, keys)
        self.expect(expected)

    def search(self):
        self.send(b"\x12tot", b"(reverse-i-search)`tot': " + LINE)

    def test_search_backspace_edits_query_and_keeps_match(self):
        for key in (b"\x7f", b"\x08"):
            with self.subTest(key=key):
                self.start_repl()
                self.search()
                self.send(key, b"(reverse-i-search)`to': " + LINE)

    def test_search_backspace_finds_latest_match_again(self):
        latest = b"let tone = 2;"
        self.start_repl((LINE, latest))
        self.search()
        self.send(b"\x7f", b"(reverse-i-search)`to': " + latest)

    def test_search_backspace_on_empty_query_keeps_match(self):
        self.start_repl()
        self.send(b"\x12", b"(reverse-i-search)`': " + LINE)
        self.send(b"\x7f", b"(reverse-i-search)`': " + LINE)

    def test_search_escape_keeps_match_and_resumes_editing(self):
        self.start_repl()
        self.search()
        # Send Esc alone: it must not wait for an escape sequence's next byte.
        self.send(b"\x1b", b"zenc >>> " + LINE + b"\r")
        self.send(b" ", b"zenc >>> " + LINE + b" \r")
        self.send(b"\x7f", b"zenc >>> " + LINE + b"\r")

    def test_repeated_ctrl_r_selects_older_match(self):
        latest = b"let total = 2;"
        self.start_repl((LINE, latest))
        self.send(b"\x12tot", b"(reverse-i-search)`tot': " + latest)
        self.send(b"\x12", b"(reverse-i-search)`tot': " + LINE)

    def test_search_ctrl_g_resumes_editing(self):
        self.start_repl()
        self.search()
        self.send(b"\x07", b"zenc >>> " + LINE + b"\r")

    def test_search_ctrl_c_cancels_line(self):
        self.start_repl()
        self.search()
        self.send(b"\x03", b"zenc >>> ")
        self.send(b"q", b"zenc >>> q\r")

    def test_search_enter_accepts_match(self):
        self.start_repl((b":help",))
        self.send(b"\x12help", b"(reverse-i-search)`help': :help")
        self.send(b"\r", b":reset")
        self.expect(b"zenc >>> ")

    def test_normal_backspace_and_arrow_keys(self):
        self.start_repl()
        self.send(b"abc", b"zenc >>> abc\r")
        self.send(b"\x7f", b"zenc >>> ab\r")
        self.send(b"\x1b[DZ", b"zenc >>> aZb\r")
        self.send(b"\x1b[CQ", b"zenc >>> aZbQ\r")


if __name__ == "__main__":
    unittest.main(verbosity=2)
