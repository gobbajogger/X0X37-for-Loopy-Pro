"""Assembler guards only; no original component tests, network or private payloads."""
import argparse
import contextlib
import hashlib
import importlib.util
import io
from pathlib import Path
import stat
import shlex
import subprocess
import sys
import tarfile
import tempfile
import unittest
from unittest import mock
import zipfile

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("dnbox_assembler", ROOT / "assemble_workspace.py")
assembler = importlib.util.module_from_spec(spec)
spec.loader.exec_module(assembler)


class AssemblerGuards(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)

    def tearDown(self):
        self.temporary.cleanup()

    def tar(self, names):
        path = self.root / "case.tar"
        with tarfile.open(path, "w") as archive:
            for name in names:
                member = tarfile.TarInfo(name)
                member.size = 3
                archive.addfile(member, io.BytesIO(b"abc"))
        return path

    def test_tar_traversal_absolute_and_drive_names(self):
        for name in ["../outside", "/absolute", "C:/outside", "dir\\outside", "dir/../outside"]:
            with self.subTest(name=name):
                with self.assertRaises(assembler.AssemblyError):
                    assembler.scan_tar(self.tar([name]))
        self.assertFalse((self.root / "outside").exists())

    def test_tar_links_are_rejected(self):
        for kind in [tarfile.SYMTYPE, tarfile.LNKTYPE]:
            with self.subTest(kind=kind):
                path = self.root / "link.tar"
                with tarfile.open(path, "w") as archive:
                    member = tarfile.TarInfo("link")
                    member.type = kind
                    member.linkname = "../outside"
                    archive.addfile(member)
                with self.assertRaises(assembler.AssemblyError):
                    assembler.scan_tar(path)

    def test_zip_traversal_and_symlink_are_rejected(self):
        for name in ["../outside", "/absolute", "C:/outside", "dir\\outside"]:
            with self.subTest(name=name):
                path = self.root / "case.zip"
                with zipfile.ZipFile(path, "w") as archive:
                    archive.writestr(name, "abc")
                with self.assertRaises(assembler.AssemblyError):
                    assembler.scan_zip(path)
        path = self.root / "link.zip"
        with zipfile.ZipFile(path, "w") as archive:
            member = zipfile.ZipInfo("link")
            member.create_system = 3
            member.external_attr = (stat.S_IFLNK | 0o777) << 16
            archive.writestr(member, "../outside")
        with self.assertRaises(assembler.AssemblyError):
            assembler.scan_zip(path)

    def test_case_collision_and_windows_reserved_names(self):
        for names in [["A.txt", "a.txt"], ["CON.txt"], ["trailing. "], ["folder", "folder/file"]]:
            with self.subTest(names=names):
                with self.assertRaises(assembler.AssemblyError):
                    assembler.scan_tar(self.tar(names))

    def test_selected_tar_extraction_omits_unselected_files(self):
        path = self.tar(["site/index.html", "private/not-selected.txt"])
        out = self.root / "out"
        self.assertEqual(assembler.extract_tar(path, out, "site/"), 1)
        self.assertEqual((out / "index.html").read_bytes(), b"abc")
        self.assertEqual(set(assembler.files_under(out)), {"index.html"})

    def test_unsafe_unselected_tar_member_still_rejects_before_writes(self):
        path = self.tar(["site/index.html", "../outside"])
        out = self.root / "out"
        with self.assertRaises(assembler.AssemblyError):
            assembler.extract_tar(path, out, "site/")
        self.assertFalse(out.exists())

    def test_nonempty_output_is_never_removed(self):
        out = self.root / "existing"
        out.mkdir()
        marker = out / "owner-work.txt"
        marker.write_text("keep me")
        with self.assertRaises(assembler.AssemblyError):
            assembler.ensure_fresh_output(out)
        self.assertEqual(marker.read_text(), "keep me")
        empty = self.root / "empty"
        empty.mkdir()
        assembler.ensure_fresh_output(empty)

    def test_bad_input_hash_creates_no_output_or_parent(self):
        archive = self.root / "wrong.tar"
        archive.write_bytes(b"not an original archive")
        out = self.root / "not-created" / "workspace"
        command = [sys.executable, str(ROOT / "assemble_workspace.py"), "--website", str(archive), "--preparation", str(archive), "--companion", str(archive), "--output", str(out)]
        result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        self.assertEqual(result.returncode, 2)
        self.assertIn("Wrong SHA-256", result.stderr)
        self.assertFalse(out.parent.exists())

    def test_manifest_guards_duplicate_and_escape(self):
        checksum = hashlib.sha256(b"abc").hexdigest()
        for text in [checksum + "  ../outside\n", checksum + "  file\n" + checksum + "  FILE\n", "bad manifest"]:
            with self.subTest(text=text):
                with self.assertRaises(assembler.AssemblyError):
                    assembler.parse_manifest(text)

    def test_size_and_count_limits(self):
        with self.assertRaises(assembler.AssemblyError):
            assembler.validate_members([("large", False, assembler.MAX_MEMBER_BYTES + 1)])
        with self.assertRaises(assembler.AssemblyError):
            assembler.validate_members([("file", False, 1)] * (assembler.MAX_MEMBERS + 1))

    def test_restart_command_quotes_spaces_and_shell_metacharacters(self):
        parts = ["python3", "--directory", "/tmp/My Music/$(`literal`)/host"]
        self.assertEqual(shlex.split(assembler.format_command(parts, windows=False)), parts)
        windows = [r"C:\Program Files\Python38\python.exe", "--directory", "C:\\My Music\\$literal`tick\\O'Brien\\host"]
        self.assertEqual(assembler.format_command(windows, windows=True), "& 'C:\\Program Files\\Python38\\python.exe' '--directory' 'C:\\My Music\\$literal`tick\\O''Brien\\host'")

    def test_port_busy_restart_preserves_output_and_powershell_literal_arguments(self):
        output = self.root / "My Music" / "$literal`tick" / "O'Brien"
        output.mkdir(parents=True)
        marker = output / "verified-work.txt"
        marker.write_text("keep verified assembly")
        original_restart = assembler.restart_command
        stdout, stderr = io.StringIO(), io.StringIO()
        with mock.patch.object(assembler, "assemble", return_value=(output, {"own_manifest_sha256": "f" * 64})), mock.patch.object(assembler, "serve", side_effect=OSError("port busy")), mock.patch.object(assembler.sys, "executable", r"C:\Program Files\Python38\python.exe"), mock.patch.object(assembler, "restart_shell", return_value="PowerShell"), mock.patch.object(assembler, "restart_command", side_effect=lambda path: original_restart(path, windows=True)), contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
            code = assembler.main(["--output", str(output), "--serve", "8000"])
        self.assertEqual(code, 2)
        self.assertEqual(marker.read_text(), "keep verified assembly")
        self.assertIn("Assembly is verified and saved", stderr.getvalue())
        self.assertIn("Restart in PowerShell with a free port: & 'C:\\Program Files\\Python38\\python.exe'", stderr.getvalue())
        self.assertIn("$literal`tick/O''Brien/host'", stderr.getvalue().replace("\\", "/"))


if __name__ == "__main__":
    unittest.main()
