"""Import pinned Unicode 17 Unihan and CLDR 48 release archives offline."""

import argparse
import gzip
import hashlib
import json
from pathlib import Path
import re
import shutil
import tempfile
from zipfile import ZipFile

import generate_emoji_annotations as emoji
from generate_emoji_sequences import read_catalog
import generate_unihan as han

ROOT = Path(__file__).resolve().parents[2]
UNIHAN_URL = 'https://www.unicode.org/Public/17.0.0/ucd/Unihan.zip'
CLDR_URL = 'https://www.unicode.org/Public/cldr/48/cldr-common-48.zip'
CLDR_SHA512 = (
    '07c69a6472b6446f2cf940fb2b22e48718e53ec458dc9733487869a851519f77df9'
    'f393f797191c04285b2f6532dd1521315bde2b25a237b664da96e152d6b4c'
)
CLDR_SOURCE = 'CLDR 48 annotations and derived annotations (verified release archive)'


def digest(path: Path, algorithm: str = 'sha256') -> str:
    """Hash a file without loading the complete archive into memory."""
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, algorithm).hexdigest()


def load_json(path: Path) -> dict:
    """Read a compressed UTF-8 snapshot."""
    with gzip.open(path, 'rt', encoding='utf-8') as stream:
        return json.load(stream)


def write_json(path: Path, data: dict) -> None:
    """Write deterministic human-readable metadata."""
    path.write_text(json.dumps(data, ensure_ascii=False, indent=2, sort_keys=True) + '\n',
                    encoding='utf-8')


def extract_cldr(path: Path, directory: Path) -> Path:
    """Extract only expected data files after verifying the pinned release digest."""
    if digest(path, 'sha512') != CLDR_SHA512:
        raise ValueError('CLDR archive does not match the published CLDR 48 SHA-512')
    with ZipFile(path) as archive:
        seen = set()
        for entry in archive.infolist():
            name = entry.filename
            if not (re.fullmatch(r'common/annotations(?:Derived)?/[A-Za-z0-9_]+\.xml', name)
                    or name in ('common/dtd/ldml.dtd',
                                'common/supplemental/supplementalData.xml')):
                continue
            if name in seen:
                raise ValueError(f'Duplicate CLDR member: {name}')
            seen.add(name)
            target = directory / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(archive.read(entry))
    root = directory / 'common'
    dtd = (root / 'dtd/ldml.dtd').read_text(encoding='utf-8')
    if not re.search(r'<!ATTLIST\s+version\s+cldrVersion\s+CDATA\s+#FIXED\s+"48"', dtd):
        raise ValueError('Incorrect CLDR version in LDML DTD')
    for component in ('annotations', 'annotationsDerived'):
        if not (root / component / 'en.xml').is_file():
            raise ValueError(f'Missing English CLDR {component}')
    return root


def stage_import(root: Path, unihan: Path, cldr: Path, stage: Path) -> dict[str, Path]:
    """Validate both archives and stage all generated outputs before publishing any file."""
    data = root / 'data/unicode'
    generated = root / 'native/unicode/generated'
    version = (generated / 'catalog-version.h').read_text(encoding='utf-8')
    if f'#define CATALOG_VERSION "{han.VERSION}"' not in version:
        raise ValueError('Official data import requires the Unicode 17 catalog')
    fallback = load_json(data / 'han/fallback.json.gz')
    previous = load_json(data / 'emoji/annotations.json.gz')
    catalog = read_catalog(root / 'native/unicode/vendor/gnome/emoji.h')
    official = han.read_official_archive(unihan)
    cldr_root = extract_cldr(cldr, stage)
    locales = emoji.resolve_cldr(cldr_root, set(previous['locales']))
    if set(previous['locales']) - set(locales) or 'en' not in locales:
        raise ValueError('CLDR import would remove a previously supported locale')
    counts = {field: sum(field in row for row in official.values())
              for field in han.FIELDS[:10]}
    if not all(counts.values()):
        raise ValueError('Verified Unihan data are missing a supported formal property')
    report = emoji.coverage(locales, catalog)
    if not report['en']['direct']:
        raise ValueError('CLDR English annotations do not cover the emoji catalog')
    snapshot = {'source': CLDR_SOURCE, 'locales': locales}
    han.generate(fallback, official, stage / 'unihan.h')
    emoji.generate(locales, CLDR_SOURCE, stage / 'emoji-annotations.h')
    encoded = json.dumps(snapshot, ensure_ascii=False, separators=(',', ':')).encode('utf-8')
    (stage / 'annotations.json.gz').write_bytes(gzip.compress(encoded, mtime=0))
    # Match the existing coverage report schema and ordering used by regeneration.
    (stage / 'coverage.json').write_text(
        json.dumps({'source': CLDR_SOURCE, 'locales': report}, indent=2) + '\n',
        encoding='utf-8')
    shutil.copyfile(unihan, stage / 'official-unihan.zip')
    outputs = {
        'unihan.h': generated / 'unihan.h',
        'emoji-annotations.h': generated / 'emoji-annotations.h',
        'official-unihan.zip': data / 'han/official-unihan.zip',
        'annotations.json.gz': data / 'emoji/annotations.json.gz',
        'coverage.json': data / 'emoji/coverage.json',
    }
    manifest = {
        'status': 'imported',
        'unihan': {'version': han.VERSION, 'url': UNIHAN_URL, 'sha256': digest(unihan),
                   'records': len(official), 'property_counts': counts},
        'cldr': {'version': '48', 'url': CLDR_URL, 'sha512': digest(cldr, 'sha512'),
                 'locale_count': len(locales), 'catalog_entries': len(catalog),
                 'english_missing': report['en']['missing']},
        'outputs_sha256': {str(destination.relative_to(root)): digest(stage / name)
                           for name, destination in outputs.items()},
    }
    write_json(stage / 'official-data.json', manifest)
    outputs['official-data.json'] = data / 'official-data.json'
    return outputs


def import_archives(root: Path, unihan: Path, cldr: Path) -> None:
    """Publish validated outputs from a same-filesystem temporary staging directory."""
    with tempfile.TemporaryDirectory(dir=root / 'data/unicode') as directory:
        stage = Path(directory)
        outputs = stage_import(root, unihan, cldr, stage)
        for name, destination in outputs.items():
            (stage / name).replace(destination)
    print('Imported pinned Unihan and CLDR archives; see data/unicode/official-data.json.')


def verify_installed(root: Path) -> None:
    """Fail if either official dataset is absent or published outputs have drifted."""
    data = root / 'data/unicode'
    manifest = json.loads((data / 'official-data.json').read_text(encoding='utf-8'))
    if manifest['status'] != 'imported':
        raise ValueError('Official data are not imported; supply both pinned archives')
    if (manifest['unihan']['sha256'] != han.OFFICIAL_SHA256 or
            manifest['cldr']['sha512'] != CLDR_SHA512):
        raise ValueError('Manifest source digests do not match the pinned archives')
    expected = {
        'native/unicode/generated/unihan.h',
        'native/unicode/generated/emoji-annotations.h',
        'data/unicode/han/official-unihan.zip',
        'data/unicode/emoji/annotations.json.gz',
        'data/unicode/emoji/coverage.json',
    }
    if set(manifest['outputs_sha256']) != expected:
        raise ValueError('Official data output manifest is incomplete')
    for name, checksum in manifest['outputs_sha256'].items():
        if digest(root / name) != checksum:
            raise ValueError(f'Official data output has changed: {name}')
    if digest(data / 'han/official-unihan.zip') != han.OFFICIAL_SHA256:
        raise ValueError('Bundled Unihan archive has changed')
    print('Verified installed official data outputs and pinned source digests.')


def main() -> int:
    """Run explicit maintainer-only import; never download during normal builds."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--unihan', type=Path)
    parser.add_argument('--cldr', type=Path)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        if args.unihan or args.cldr:
            parser.error('--verify cannot be combined with archive inputs')
        try:
            verify_installed(ROOT)
        except (OSError, ValueError) as error:
            parser.error(str(error))
    elif args.unihan and args.cldr:
        try:
            import_archives(ROOT, args.unihan.resolve(), args.cldr.resolve())
        except (OSError, ValueError) as error:
            parser.error(str(error))
    else:
        parser.error('Both --unihan and --cldr are required for import')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
