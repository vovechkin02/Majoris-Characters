"""Decode bounded results from the local Characters input-search command."""

import base64
import binascii

MAX_QUERY_BYTES = 512
MAX_RESULTS = 20
MAX_OUTPUT_BYTES = 65536


def decode_results(output: str) -> list[tuple[str, str]]:
    """Return exact UTF-8 identities and labels, or reject a malformed response."""
    if len(output.encode("utf-8")) > MAX_OUTPUT_BYTES:
        raise ValueError("Input search output exceeds its limit")
    results = []
    for line in output.splitlines():
        try:
            encoded_text, encoded_name = line.split("\t")
            text = base64.b64decode(encoded_text, validate=True).decode("utf-8")
            name = base64.b64decode(encoded_name, validate=True).decode("utf-8")
        except (ValueError, UnicodeError, binascii.Error) as error:
            raise ValueError("Malformed input search result") from error
        if not text or len(text) > 32 or "\x00" in text or not name or "\x00" in name:
            raise ValueError("Invalid input search identity")
        results.append((text, name))
        if len(results) > MAX_RESULTS:
            raise ValueError("Too many input search results")
    return results
