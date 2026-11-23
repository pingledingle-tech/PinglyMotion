#!/usr/bin/env python3
# Tests for tools/check_shared_lib.py
import os
import shutil
import sys
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..")
sys.path.insert(0, os.path.join(ROOT, "tools"))
import check_shared_lib as c  # noqa: E402

SRC = """
# comment SET (PINGLYMOTION_SHARED_SRC fake.cpp)
SET (PINGLYMOTION_SHARED_SRC
              mesh.cpp # trailing comment
              scriptlab/labtree.cpp
)
ADD_LIBRARY (pinglymotion_shared STATIC ${PINGLYMOTION_SHARED_SRC})
"""
SHARED = ["mesh.cpp", "scriptlab/labtree.cpp"]


def bad(text, folder="tests/foo"):
    return [rel for rel, _t in c.violations(SHARED, text, folder)]


class SharedLib(unittest.TestCase):
    def test_reads_the_list_and_skips_comments(self):
        self.assertEqual(c.shared_sources(SRC), SHARED)

    def test_missing_list_is_an_error(self):
        with self.assertRaises(ValueError):
            c.shared_sources("SET (OTHER a.cpp)")

    def test_linking_is_fine(self):
        self.assertEqual(bad(
            "ADD_EXECUTABLE (t tst_a.cpp)\n"
            "TARGET_LINK_LIBRARIES (t pinglymotion_shared)"), [])

    def test_source_dir_path_is_flagged(self):
        self.assertEqual(bad(
            "ADD_EXECUTABLE (t tst_a.cpp ${CMAKE_SOURCE_DIR}/src/mesh.cpp)"),
            ["mesh.cpp"])

    def test_subfolder_file_is_flagged(self):
        self.assertEqual(bad(
            "ADD_EXECUTABLE (t tst_a.cpp\n"
            "   ${CMAKE_SOURCE_DIR}/src/scriptlab/labtree.cpp)"),
            ["scriptlab/labtree.cpp"])

    def test_call_is_case_insensitive(self):
        self.assertEqual(bad(
            "add_executable(t tst_a.cpp ${CMAKE_SOURCE_DIR}/src/mesh.cpp)"),
            ["mesh.cpp"])
        self.assertEqual(bad(
            "Add_Executable (t tst_a.cpp ../../src/mesh.cpp)"), ["mesh.cpp"])

    def test_variable_list_is_followed(self):
        self.assertEqual(bad(
            "SET (APP_SRC ${CMAKE_SOURCE_DIR}/src/other.cpp\n"
            "             ${CMAKE_SOURCE_DIR}/src/mesh.cpp)\n"
            "ADD_EXECUTABLE (t tst_a.cpp ${APP_SRC})"), ["mesh.cpp"])

    def test_other_variable_prefix_is_matched_by_path_end(self):
        self.assertEqual(bad(
            "ADD_EXECUTABLE (t tst_a.cpp ${SRC_ROOT}/src/mesh.cpp)"),
            ["mesh.cpp"])

    def test_quoted_path_is_flagged(self):
        self.assertEqual(bad(
            'ADD_EXECUTABLE (t tst_a.cpp "${CMAKE_SOURCE_DIR}/src/mesh.cpp")'),
            ["mesh.cpp"])

    def test_not_shared_file_is_fine(self):
        self.assertEqual(bad(
            "ADD_EXECUTABLE (t tst_a.cpp ${CMAKE_SOURCE_DIR}/src/footlock.cpp"
            " ${CMAKE_SOURCE_DIR}/tests/uirender/uirender.cpp)"), [])

    def test_own_file_with_same_name_is_fine(self):
        self.assertEqual(bad("ADD_EXECUTABLE (t tst_a.cpp mesh.cpp)"), [])

    def test_commented_out_line_is_fine(self):
        self.assertEqual(bad(
            "ADD_EXECUTABLE (t tst_a.cpp\n"
            "   # ${CMAKE_SOURCE_DIR}/src/mesh.cpp\n)"), [])

    def test_other_calls_are_not_looked_at(self):
        self.assertEqual(bad(
            "TARGET_SOURCES (t PRIVATE ${CMAKE_SOURCE_DIR}/src/mesh.cpp)"),
            [])

    def test_the_repo_passes_today(self):
        self.assertEqual(c.check(ROOT), [])

    def test_a_bad_test_in_a_copy_of_the_repo_fails(self):
        tmp = tempfile.mkdtemp()
        try:
            os.makedirs(os.path.join(tmp, "src"))
            os.makedirs(os.path.join(tmp, "tests", "foo"))
            with open(os.path.join(tmp, "src", "CMakeLists.txt"), "w") as f:
                f.write(SRC)
            with open(os.path.join(tmp, "tests", "foo", "CMakeLists.txt"),
                      "w") as f:
                f.write("ADD_EXECUTABLE (t tst_a.cpp"
                        " ${CMAKE_SOURCE_DIR}/src/mesh.cpp)\n")
            problems = c.check(tmp)
            self.assertEqual(len(problems), 1)
            self.assertIn("link pinglymotion_shared", problems[0].lower())
            self.assertIn("do not list the file", problems[0])
            self.assertIn("tests/foo/CMakeLists.txt", problems[0])
        finally:
            shutil.rmtree(tmp)


if __name__ == "__main__":
    unittest.main()
