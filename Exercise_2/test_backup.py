import unittest
import os
import shutil
import subprocess
import stat
import tempfile
import sys

# --- ANSI Color Codes ---
COLOR_RESET = "\033[0m"
COLOR_RED = "\033[91m"
COLOR_GREEN = "\033[92m"
COLOR_YELLOW = "\033[93m"
COLOR_BLUE = "\033[94m"
COLOR_MAGENTA = "\033[95m"
COLOR_CYAN = "\033[96m"
COLOR_WHITE = "\033[97m"
COLOR_BOLD = "\033[1m"
# --- /Color Codes ---

# Path to the backup executable (assumed to be in the current directory)
BACKUP_EXECUTABLE_NAME = "backup"
BACKUP_EXECUTABLE = "./backup"


class TestBackupTool(unittest.TestCase):

    def setUp(self):
        """Set up temporary directories for source and destination for each test."""
        self.test_dir = tempfile.mkdtemp(prefix="backup_test_")
        self.src_dir = os.path.join(self.test_dir, "source")
        self.dest_dir = os.path.join(self.test_dir, "dest")
        os.makedirs(self.src_dir)

        if not os.path.isfile(BACKUP_EXECUTABLE):
            print(f"{COLOR_RED}{COLOR_BOLD}Error: Backup executable '{BACKUP_EXECUTABLE}' not found.{COLOR_RESET}", file=sys.stderr)
            self.fail(f"Backup executable '{BACKUP_EXECUTABLE}' not found.")
        if not os.access(BACKUP_EXECUTABLE, os.X_OK):
            print(f"{COLOR_RED}{COLOR_BOLD}Error: Backup executable '{BACKUP_EXECUTABLE}' is not executable.{COLOR_RESET}", file=sys.stderr)
            self.fail(f"Backup executable '{BACKUP_EXECUTABLE}' is not executable.")

        # --- Rainbow Header ---
        test_name = self._testMethodName
        colors = [COLOR_RED, COLOR_YELLOW, COLOR_GREEN, COLOR_CYAN, COLOR_BLUE, COLOR_MAGENTA]
        colored_test_name = "".join(colors[i % len(colors)] + char for i, char in enumerate(test_name))
        print(f"\n{COLOR_MAGENTA}--- Running test: {COLOR_BOLD}{colored_test_name}{COLOR_RESET}{COLOR_MAGENTA} ---{COLOR_RESET}")
        # --- /Rainbow Header ---

        print(f"{COLOR_BLUE}Source Dir:{COLOR_RESET} {self.src_dir}")
        print(f"{COLOR_BLUE}Destination Dir:{COLOR_RESET} {self.dest_dir}")

    def tearDown(self):
        """Clean up temporary directories after each test."""
        if os.path.exists(self.test_dir):
            shutil.rmtree(self.test_dir)
            # print(f"{COLOR_YELLOW}Cleaned up: {self.test_dir}{COLOR_RESET}")

    def _run_backup(self, src=None, dest=None):
        """Helper method to run the backup tool."""
        src = src or self.src_dir
        dest = dest or self.dest_dir
        command = [BACKUP_EXECUTABLE, src, dest]
        print(f"{COLOR_YELLOW}Executing:{COLOR_RESET} {' '.join(command)}")
        try:
            result = subprocess.run(command, capture_output=True, text=True, check=False, timeout=10) # Added timeout
        except subprocess.TimeoutExpired as e:
            print(f"{COLOR_RED}Execution Timed Out!{COLOR_RESET}")
            self.fail(f"Backup command timed out: {e}")
        except Exception as e:
            print(f"{COLOR_RED}Execution Failed: {e}{COLOR_RESET}")
            self.fail(f"Failed to run backup command: {e}")


        exit_code_color = COLOR_GREEN if result.returncode == 0 else COLOR_RED
        print(f"{exit_code_color}Exit Code: {result.returncode}{COLOR_RESET}")

        if result.stdout:
            print(f"{COLOR_WHITE}stdout:{COLOR_RESET}\n{result.stdout.strip()}")
        if result.stderr:
            # Color stderr red if exit code was non-zero, otherwise yellow (warnings?)
            stderr_color = COLOR_RED if result.returncode != 0 else COLOR_YELLOW
            print(f"{stderr_color}stderr:{COLOR_RESET}\n{result.stderr.strip()}")
        return result

    def _create_file(self, path, content=""):
        """Helper to create a file."""
        with open(path, "w") as f:
            f.write(content)
        print(f"{COLOR_CYAN}Created file:{COLOR_RESET} {path} (content: '{content[:10]}...')")

    def _create_dir(self, path):
        """Helper to create a directory."""
        os.makedirs(path)
        print(f"{COLOR_CYAN}Created dir:{COLOR_RESET} {path}")

    def _create_symlink(self, target, link_path):
        """Helper to create a symbolic link."""
        os.symlink(target, link_path)
        print(f"{COLOR_CYAN}Created symlink:{COLOR_RESET} {link_path} -> {target}")

    def _verify_backup(self, src_base, dest_base):
        """Recursively verifies the backup matches the source structure and properties."""
        src_items = sorted(os.listdir(src_base))
        dest_items = sorted(os.listdir(dest_base))

        # Check for missing/extra items first
        self.assertListEqual(dest_items, src_items,
                             f"Mismatch in directory listing for {COLOR_BLUE}{dest_base}{COLOR_RESET}. "
                             f"Src: {src_items}, Dest: {dest_items}")

        for item in src_items:
            src_path = os.path.join(src_base, item)
            dest_path = os.path.join(dest_base, item)

            print(f"{COLOR_MAGENTA}Verifying:{COLOR_RESET} {dest_path}")
            # Existence already checked by comparing listings above
            # self.assertTrue(os.path.exists(dest_path), f"Destination item missing: {dest_path}")

            src_stat = os.lstat(src_path)
            dest_stat = os.lstat(dest_path)
            src_mode = src_stat.st_mode
            dest_mode = dest_stat.st_mode

            # 1. Check Type Match and specific properties
            if stat.S_ISREG(src_mode):
                self.assertTrue(stat.S_ISREG(dest_mode), f"{COLOR_RED}Type mismatch (not regular file): {dest_path}{COLOR_RESET}")
                self.assertEqual(src_stat.st_ino, dest_stat.st_ino,
                                 f"{COLOR_RED}Not a hard link (inodes differ): {src_path} ({src_stat.st_ino}) vs {dest_path} ({dest_stat.st_ino}){COLOR_RESET}")
                self.assertEqual(stat.S_IMODE(src_mode), stat.S_IMODE(dest_mode),
                                 f"{COLOR_RED}Permission mismatch for file: {dest_path} ({oct(stat.S_IMODE(src_mode))} vs {oct(stat.S_IMODE(dest_mode))}){COLOR_RESET}")

            elif stat.S_ISDIR(src_mode):
                self.assertTrue(stat.S_ISDIR(dest_mode), f"{COLOR_RED}Type mismatch (not directory): {dest_path}{COLOR_RESET}")
                self.assertEqual(stat.S_IMODE(src_mode), stat.S_IMODE(dest_mode),
                                 f"{COLOR_RED}Permission mismatch for directory: {dest_path} ({oct(stat.S_IMODE(src_mode))} vs {oct(stat.S_IMODE(dest_mode))}){COLOR_RESET}")
                self._verify_backup(src_path, dest_path) # Recurse

            elif stat.S_ISLNK(src_mode):
                self.assertTrue(stat.S_ISLNK(dest_mode), f"{COLOR_RED}Type mismatch (not symlink): {dest_path}{COLOR_RESET}")
                src_link_target = os.readlink(src_path)
                dest_link_target = os.readlink(dest_path)
                self.assertEqual(src_link_target, dest_link_target,
                                 f"{COLOR_RED}Symlink target mismatch: {dest_path} points to '{dest_link_target}', expected '{src_link_target}'{COLOR_RESET}")
            else:
                # Use red for failure indication
                self.fail(f"{COLOR_RED}Unsupported file type found in source: {src_path}{COLOR_RESET}")


    # --- Test Cases (logic remains the same, only output formatting changes) ---

    def test_basic_structure(self):
        """Test a basic structure with files, subdirs, and a symlink."""
        self._create_file(os.path.join(self.src_dir, "file1.txt"), "content1")
        self._create_file(os.path.join(self.src_dir, "file2.txt"), "content2")
        subdir1 = os.path.join(self.src_dir, "subdir1")
        self._create_dir(subdir1)
        self._create_file(os.path.join(subdir1, "file3.txt"), "content3")
        subdir2 = os.path.join(self.src_dir, "subdir2")
        self._create_dir(subdir2)
        target = "../file1.txt"
        link_path = os.path.join(subdir2, "link_to_file1")
        self._create_symlink(target, link_path)

        result = self._run_backup()
        self.assertEqual(result.returncode, 0, f"{COLOR_RED}Backup script failed unexpectedly{COLOR_RESET}")
        self.assertTrue(os.path.isdir(self.dest_dir), f"{COLOR_RED}Destination directory was not created{COLOR_RESET}")
        self._verify_backup(self.src_dir, self.dest_dir)

    def test_empty_source_directory(self):
        """Test backing up an empty directory."""
        result = self._run_backup()
        self.assertEqual(result.returncode, 0, f"{COLOR_RED}Backup script failed for empty dir{COLOR_RESET}")
        self.assertTrue(os.path.isdir(self.dest_dir), f"{COLOR_RED}Destination directory was not created{COLOR_RESET}")
        self.assertEqual(len(os.listdir(self.dest_dir)), 0, f"{COLOR_RED}Destination directory is not empty{COLOR_RESET}")

    def test_source_with_only_files(self):
        """Test backing up a directory containing only files."""
        self._create_file(os.path.join(self.src_dir, "f1.dat"), "data1")
        self._create_file(os.path.join(self.src_dir, "f2.log"), "data2")
        result = self._run_backup()
        self.assertEqual(result.returncode, 0, f"{COLOR_RED}Backup script failed unexpectedly{COLOR_RESET}")
        self.assertTrue(os.path.isdir(self.dest_dir), f"{COLOR_RED}Destination directory was not created{COLOR_RESET}")
        self._verify_backup(self.src_dir, self.dest_dir)

    def test_source_with_only_empty_dirs(self):
        """Test backing up a directory containing only empty subdirectories."""
        self._create_dir(os.path.join(self.src_dir, "empty1"))
        self._create_dir(os.path.join(self.src_dir, "empty2"))
        result = self._run_backup()
        self.assertEqual(result.returncode, 0, f"{COLOR_RED}Backup script failed unexpectedly{COLOR_RESET}")
        self.assertTrue(os.path.isdir(self.dest_dir), f"{COLOR_RED}Destination directory was not created{COLOR_RESET}")
        self._verify_backup(self.src_dir, self.dest_dir)

    def test_deeply_nested_structure(self):
        """Test a more complex nested directory structure."""
        d1 = os.path.join(self.src_dir, "d1")
        d1d2 = os.path.join(d1, "d2")
        d1d2d3 = os.path.join(d1d2, "d3")
        self._create_dir(d1)
        self._create_dir(d1d2)
        self._create_dir(d1d2d3)
        self._create_file(os.path.join(self.src_dir, "root.txt"), "root")
        self._create_file(os.path.join(d1, "level1.txt"), "l1")
        self._create_file(os.path.join(d1d2d3, "level3.txt"), "l3")
        target = "../../root.txt"
        link_path = os.path.join(d1d2, "link_to_root")
        self._create_symlink(target, link_path)

        result = self._run_backup()
        self.assertEqual(result.returncode, 0, f"{COLOR_RED}Backup script failed unexpectedly{COLOR_RESET}")
        self.assertTrue(os.path.isdir(self.dest_dir), f"{COLOR_RED}Destination directory was not created{COLOR_RESET}")
        self._verify_backup(self.src_dir, self.dest_dir)

    def test_permission_preservation(self):
        """Test if file and directory permissions are preserved."""
        file_ro = os.path.join(self.src_dir, "readonly.txt")
        file_rw = os.path.join(self.src_dir, "readwrite.txt")
        dir_exec = os.path.join(self.src_dir, "executable_dir")

        self._create_file(file_ro, "read only")
        os.chmod(file_ro, 0o444)
        self._create_file(file_rw, "read write")
        os.chmod(file_rw, 0o664)
        self._create_dir(dir_exec)
        os.chmod(dir_exec, 0o755)

        original_perms = {
            file_ro: stat.S_IMODE(os.lstat(file_ro).st_mode),
            file_rw: stat.S_IMODE(os.lstat(file_rw).st_mode),
            dir_exec: stat.S_IMODE(os.lstat(dir_exec).st_mode)
        }

        result = self._run_backup()
        self.assertEqual(result.returncode, 0, f"{COLOR_RED}Backup script failed unexpectedly{COLOR_RESET}")
        self.assertTrue(os.path.isdir(self.dest_dir), f"{COLOR_RED}Destination directory was not created{COLOR_RESET}")
        self._verify_backup(self.src_dir, self.dest_dir)

        # Explicit checks (verification includes this, but belt-and-suspenders)
        dest_file_ro = os.path.join(self.dest_dir, "readonly.txt")
        dest_file_rw = os.path.join(self.dest_dir, "readwrite.txt")
        dest_dir_exec = os.path.join(self.dest_dir, "executable_dir")
        self.assertEqual(stat.S_IMODE(os.lstat(dest_file_ro).st_mode), original_perms[file_ro], f"{COLOR_RED}Read-only file permissions mismatch{COLOR_RESET}")
        self.assertEqual(stat.S_IMODE(os.lstat(dest_file_rw).st_mode), original_perms[file_rw], f"{COLOR_RED}Read-write file permissions mismatch{COLOR_RESET}")
        self.assertEqual(stat.S_IMODE(os.lstat(dest_dir_exec).st_mode), original_perms[dir_exec], f"{COLOR_RED}Executable dir permissions mismatch{COLOR_RESET}")

    def test_symlink_to_directory(self):
        """Test backing up a symlink that points to a directory."""
        real_dir = os.path.join(self.src_dir, "real_dir")
        self._create_dir(real_dir)
        self._create_file(os.path.join(real_dir, "inside.txt"), "inside")
        link_dir = os.path.join(self.src_dir, "link_to_dir")
        self._create_symlink("real_dir", link_dir)

        result = self._run_backup()
        self.assertEqual(result.returncode, 0, f"{COLOR_RED}Backup script failed unexpectedly{COLOR_RESET}")
        self.assertTrue(os.path.isdir(self.dest_dir), f"{COLOR_RED}Destination directory was not created{COLOR_RESET}")
        self._verify_backup(self.src_dir, self.dest_dir)

        dest_link_path = os.path.join(self.dest_dir, "link_to_dir")
        self.assertTrue(os.path.islink(dest_link_path), f"{COLOR_RED}Symlink to directory was not created in destination{COLOR_RESET}")
        self.assertEqual(os.readlink(dest_link_path), "real_dir", f"{COLOR_RED}Symlink target mismatch for directory link{COLOR_RESET}")

    # --- Error Condition Tests ---

    def test_error_source_does_not_exist(self):
        """Test running backup when the source directory does not exist."""
        non_existent_src = os.path.join(self.test_dir, "non_existent_source")
        result = self._run_backup(src=non_existent_src)
        self.assertNotEqual(result.returncode, 0, f"{COLOR_YELLOW}Backup script should fail if source doesn't exist, but exit code was 0{COLOR_RESET}")
        self.assertIn("src dir", result.stderr.lower(),
                      f"{COLOR_RED}Expected 'src dir' error message not found in stderr: {result.stderr}{COLOR_RESET}")
        self.assertFalse(os.path.exists(self.dest_dir), f"{COLOR_RED}Destination directory should not be created on source error{COLOR_RESET}")

    def test_error_destination_already_exists(self):
        """Test running backup when the destination directory already exists."""
        os.makedirs(self.dest_dir)
        preexisting_file = os.path.join(self.dest_dir, "preexisting.txt")
        self._create_file(preexisting_file, "exists")

        result = self._run_backup()
        self.assertNotEqual(result.returncode, 0, f"{COLOR_YELLOW}Backup script should fail if destination exists, but exit code was 0{COLOR_RESET}")
        self.assertIn("backup dir", result.stderr.lower(),
                      f"{COLOR_RED}Expected 'backup dir' error message not found in stderr: {result.stderr}{COLOR_RESET}")
        self.assertTrue(os.path.exists(preexisting_file), f"{COLOR_RED}Pre-existing file was unexpectedly removed{COLOR_RESET}")

    def test_error_no_arguments(self):
        """Test running backup with no arguments."""
        command = [BACKUP_EXECUTABLE]
        print(f"{COLOR_YELLOW}Executing:{COLOR_RESET} {' '.join(command)}")
        result = subprocess.run(command, capture_output=True, text=True, check=False)
        self.assertNotEqual(result.returncode, 0, f"{COLOR_YELLOW}Backup script should fail with no arguments, but exit code was 0{COLOR_RESET}")
        self.assertTrue(len(result.stderr) > 0 or len(result.stdout) > 0,
                        f"{COLOR_RED}Expected some error/usage output for no arguments, but got none{COLOR_RESET}")

    def test_error_one_argument(self):
        """Test running backup with only one argument."""
        command = [BACKUP_EXECUTABLE, self.src_dir]
        print(f"{COLOR_YELLOW}Executing:{COLOR_RESET} {' '.join(command)}")
        result = subprocess.run(command, capture_output=True, text=True, check=False)
        self.assertNotEqual(result.returncode, 0, f"{COLOR_YELLOW}Backup script should fail with one argument, but exit code was 0{COLOR_RESET}")
        self.assertTrue(len(result.stderr) > 0 or len(result.stdout) > 0,
                        f"{COLOR_RED}Expected some error/usage output for one argument, but got none{COLOR_RESET}")

    def test_error_too_many_arguments(self):
        """Test running backup with too many arguments."""
        command = [BACKUP_EXECUTABLE, self.src_dir, self.dest_dir, "extra_arg"]
        print(f"{COLOR_YELLOW}Executing:{COLOR_RESET} {' '.join(command)}")
        result = subprocess.run(command, capture_output=True, text=True, check=False)
        self.assertNotEqual(result.returncode, 0, f"{COLOR_YELLOW}Backup script should fail with too many arguments, but exit code was 0{COLOR_RESET}")
        self.assertTrue(len(result.stderr) > 0 or len(result.stdout) > 0,
                        f"{COLOR_RED}Expected some error/usage output for too many arguments, but got none{COLOR_RESET}")


if __name__ == '__main__':
    # Check for executable before running any tests
    if not os.path.isfile(BACKUP_EXECUTABLE_NAME):
        print(f"{COLOR_RED}{COLOR_BOLD}Error: Backup executable '{BACKUP_EXECUTABLE_NAME}' not found in the current directory.{COLOR_RESET}", file=sys.stderr)
        print(f"{COLOR_YELLOW}Please compile the C code (gcc -o {BACKUP_EXECUTABLE_NAME} backup.c) first.{COLOR_RESET}", file=sys.stderr)
        sys.exit(1)
    if not os.access(BACKUP_EXECUTABLE, os.X_OK):
        print(f"{COLOR_RED}{COLOR_BOLD}Error: Backup executable '{BACKUP_EXECUTABLE_NAME}' is not executable.{COLOR_RESET}", file=sys.stderr)
        print(f"{COLOR_YELLOW}Please check file permissions (chmod +x {BACKUP_EXECUTABLE_NAME}).{COLOR_RESET}", file=sys.stderr)
        sys.exit(1)

    # Run tests with unittest's runner (which also adds its own output formatting)
    unittest.main(verbosity=2)
