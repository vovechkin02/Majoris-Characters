"""Extract the repository's literal gettext messages into a deterministic template."""

import argparse
import ast
from collections import defaultdict
import json
from pathlib import Path
import re
import xml.etree.ElementTree as ET


# The project uses ordinary quoted literals, C adjacency and Vala concatenation.
TOKEN = re.compile(
    r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|'
    r"'(?:\\.|[^'\\])*'|[A-Za-z_]\w*|[^\s]", re.DOTALL
)
PRINTF = re.compile(r'%(?:\d+\$)?[-+ #0]*\d*(?:\.\d+)?(?:hh|ll|[hljztL])?'
                    r'[diouxXfFeEgGaAcsp%]')


def literal_messages(source: str):
    """Yield marked C/Vala literals, excluding comments and dynamic lookups."""
    tokens = [m for m in TOKEN.finditer(source)
              if not m.group().startswith(('/*', '//'))]
    for index, token in enumerate(tokens):
        if token.group() not in ('_', 'N_') or index + 2 >= len(tokens):
            continue
        if tokens[index + 1].group() != '(':
            continue
        cursor = index + 2
        parts = []
        while cursor < len(tokens) and tokens[cursor].group().startswith('"'):
            parts.append(ast.literal_eval(tokens[cursor].group()))
            cursor += 1
            if cursor < len(tokens) and tokens[cursor].group() == '+':
                cursor += 1
        if parts and cursor < len(tokens) and tokens[cursor].group() == ')':
            yield ''.join(parts), source.count('\n', 0, token.start()) + 1


def file_messages(path: Path):
    """Extract marked UI, desktop, Python and native source messages."""
    source = path.read_text(encoding='utf-8')
    if path.suffix == '.ui':
        for element in ET.fromstring(source).iter():
            if element.get('translatable') == 'yes' and element.text:
                if element.get('context'):
                    raise ValueError(f'Context extraction is not supported: {path}')
                yield element.text, 0
    elif path.suffix == '.desktop':
        for line_no, line in enumerate(source.splitlines(), 1):
            if line.startswith(('Name=', 'Comment=', 'Keywords=')):
                yield line.split('=', 1)[1], line_no
    elif path.suffix == '.py':
        for node in ast.walk(ast.parse(source)):
            if (isinstance(node, ast.Call) and isinstance(node.func, ast.Name)
                    and node.func.id == '_' and node.args
                    and isinstance(node.args[0], ast.Constant)
                    and isinstance(node.args[0].value, str)):
                yield node.args[0].value, node.lineno
    else:
        yield from literal_messages(source)


def inventory(root: Path) -> dict[str, set[str]]:
    """Collect unique messages and source references from POTFILES.in."""
    messages = defaultdict(set)
    for name in (root / 'po/POTFILES.in').read_text().splitlines():
        if not name or name.startswith('#'):
            continue
        for message, line in file_messages(root / name):
            messages[message].add(f'{name}:{line}' if line else name)
    return dict(messages)


def template(root: Path) -> str:
    """Serialize the project message inventory in standard gettext POT syntax."""
    header = ('Project-Id-Version: majoris-characters\n'
              'Report-Msgid-Bugs-To: https://github.com/vovechkin02/Majoris-Characters/issues\n'
              'MIME-Version: 1.0\nContent-Type: text/plain; charset=UTF-8\n'
              'Content-Transfer-Encoding: 8bit\n')
    lines = ['# Majoris Characters translation template.', 'msgid ""',
             'msgstr ""', *[json.dumps(line + '\n', ensure_ascii=False)
                             for line in header.splitlines()], '']
    for message, references in sorted(inventory(root).items()):
        lines.append('#: ' + ' '.join(sorted(references)))
        if PRINTF.search(message):
            lines.append('#, c-format')
        lines.extend([f'msgid {json.dumps(message, ensure_ascii=False)}',
                      'msgstr ""', ''])
    return '\n'.join(lines) + '\n'


def main() -> int:
    """Update the checked-in template or reject an outdated message inventory."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    path = root / 'po/majoris-characters.pot'
    content = template(root)
    if args.check:
        if path.read_text(encoding='utf-8') != content:
            parser.exit(1, 'Translation template is stale; regenerate it.\n')
    else:
        path.write_text(content, encoding='utf-8')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
