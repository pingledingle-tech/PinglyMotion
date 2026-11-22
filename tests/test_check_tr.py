#!/usr/bin/env python3
"""Tests for tools/check_tr.py (docs/TRANSLATION.md). Standard library only."""

import os
import sys
import unittest

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import check_tr  # noqa: E402


def lines(code):
    return [number for number, _line in check_tr.find_untranslated(code)]


class FindUntranslated(unittest.TestCase):
    def test_marked_texts_pass(self):
        code = '\n'.join([
            'label->setText(tr("Hello there"));',
            'label->setText(QObject::tr("Hello"));',
            'label->setToolTip(QCoreApplication::translate("Ctx", "Hello there"));',
            'QMessageBox::warning(this, tr("Title"), tr("Body text"));',
        ])
        self.assertEqual(lines(code), [])

    def test_unmarked_texts_are_found(self):
        code = '\n'.join([
            'label->setText("Checking for updates...");',
            'label->setText(tr("Fine"));',
            'setWindowTitle("Settings");',
            'QMessageBox::warning(this, "Title", tr("Body text"));',
            'label->setText("Last: " + when);',
        ])
        self.assertEqual(lines(code), [1, 3, 4, 5])

    def test_keys_paths_and_commands_are_not_texts(self):
        code = '\n'.join([
            'item->setText(0, o["stop_reason"].toString());',
            'button->setIcon(QIcon(":/icons/play.png")); label->setText(":/x/y.png");',
            'm_command->setPlaceholderText(QStringLiteral("@detach=n"));',
            'combo->addItem("-");',
        ])
        self.assertEqual(lines(code), [])

    def test_marker_and_comments(self):
        code = '\n'.join([
            'combo->addItem("Site");  // tr-ok: a name, not a text',
            '// label->setText("Commented out");',
            ' * label->setText("In a comment block");',
        ])
        self.assertEqual(lines(code), [])


class TsFile(unittest.TestCase):
    def test_pairs(self):
        xml = ('<?xml version="1.0"?><TS version="2.1" language="en"><context><name>A</name>'
               '<message><source>One %1</source><translation type="unfinished"></translation></message>'
               '</context></TS>')
        self.assertEqual(check_tr.ts_messages(xml), {("A", "One %1")})


if __name__ == "__main__":
    unittest.main()
