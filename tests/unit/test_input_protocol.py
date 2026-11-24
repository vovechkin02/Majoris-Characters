"""Test the input-source wire format without requiring IBus or a graphical session."""

import base64
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools" / "input"))
from input_protocol import MAX_OUTPUT_BYTES, MAX_RESULTS, decode_results


def row(text: str, name: str = "Character") -> str:
    """Encode the same two UTF-8 fields as the native application's CLI."""
    return "\t".join(base64.b64encode(value.encode()).decode() for value in (text, name))


class InputProtocolTest(unittest.TestCase):
    """Preserve exact identities and reject malformed or excessive responses."""

    def test_exact_unicode(self) -> None:
        """Preserve combining marks, joiners, selectors, and controls other than NUL."""
        texts = ["e\u0301", "é", "👩🏽‍💻", "©\ufe0e", "©\ufe0f", "\t", "\n"]
        encoded = "\n".join(row(text) for text in texts) + "\n"
        self.assertEqual([text for text, _ in decode_results(encoded)], texts)
        self.assertEqual(decode_results(""), [])

    def test_invalid(self) -> None:
        """Reject invalid base64, UTF-8, NUL, missing labels, and oversized identities."""
        for value in ("invalid", "%%%%\tYQ==", "/w==\tYQ==", row("\x00"),
                      row(""), row("a", ""), row("x" * 33)):
            with self.subTest(value=value), self.assertRaises(ValueError):
                decode_results(value)

    def test_bounds(self) -> None:
        """Enforce the result count and output byte budget before publishing candidates."""
        self.assertEqual(len(decode_results("\n".join([row("A")] * MAX_RESULTS))),
                         MAX_RESULTS)
        for value in ("\n".join([row("A")] * (MAX_RESULTS + 1)),
                      "a" * (MAX_OUTPUT_BYTES + 1)):
            with self.assertRaises(ValueError):
                decode_results(value)


if __name__ == "__main__":
    unittest.main()
