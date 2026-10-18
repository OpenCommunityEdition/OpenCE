"""Read and write the generators' text so the same script runs on Python 2.6 and 3.

The Macs that build the session (c2d, the G4s) have Python 2.6 and no pathlib or argparse; the host has
Python 3. Text is read and written as latin-1 without newline translation, so both write the same bytes.
"""
import io
import os


def read_text(path):
    with io.open(path, 'r', encoding='latin-1', newline='') as f:
        return f.read()


def read_bytes(path):
    with open(path, 'rb') as f:
        return bytearray(f.read())


def write_text(path, text):
    """Write text, creating its folder; leave the file alone when it already holds exactly this (so make and
    the session's own staleness checks see an unchanged file as unchanged)."""
    if not isinstance(text, type(u'')):
        text = text.decode('latin-1')
    folder = os.path.dirname(path)
    if folder and not os.path.isdir(folder):
        os.makedirs(folder)
    if os.path.isfile(path) and read_text(path) == text:
        return
    with io.open(path, 'w', encoding='latin-1', newline='') as f:
        f.write(text)
