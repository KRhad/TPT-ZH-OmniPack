#!/usr/bin/env python3

"""Install crisp Simplified Chinese pixel glyphs into resources/font.bz2."""

from hashlib import sha256
from pathlib import Path
from tempfile import TemporaryDirectory
from urllib.request import Request, urlopen
from zipfile import ZipFile

from fonttool import BDFReader, FontTool


ROOT = Path(__file__).resolve().parent
FONT_RESOURCE = ROOT / "resources" / "font.bz2"
CACHE_DIR = ROOT / "tmp" / "font-cache"
ARCHIVE_NAME = "fusion-pixel-font-10px-monospaced-bdf-v2026.07.20.zip"
ARCHIVE_URL = (
	"https://github.com/TakWolf/fusion-pixel-font/releases/download/2026.07.20/"
	+ ARCHIVE_NAME
)
ARCHIVE_SHA256 = "2e695c27627bf09683df2afe69b086fa3cd3e52795bce39fded9cc509188b5fe"
BDF_MEMBER = "fusion-pixel-10px-monospaced-zh_hans.bdf"

# Fusion Pixel Font is 10 pixels high.  A y offset of -2 centres those rows in
# TPT's 12-pixel line box without scaling or antialiasing.
BDF_X_OFFSET = 0
BDF_Y_OFFSET = -2

# These are the ranges previously populated from the system CJK font.  Clear
# them before installing the OFL-licensed pixel glyphs so reruns are clean and
# deterministic.  The private-use icon range (U+E000...) is deliberately not
# touched.
RANGES = (
	(0x3000, 0x303F),
	(0x4E00, 0x9FFF),
	(0xFF01, 0xFF60),
	(0xFFE0, 0xFFEE),
)

# Fusion Pixel Font 10 px deliberately limits its ideograph repertoire.  This
# hand-drawn glyph covers the one built-in zh-CN string outside that repertoire
# (PLUT / plutonium) without falling back to a platform font.
MANUAL_GLYPHS = {
	0x949A: (
		"0000000000",
		"0100000000",
		"1010011111",
		"1000000100",
		"1110001100",
		"1000010100",
		"1100000110",
		"1010000101",
		"1010000100",
		"1100000100",
		"0000000000",
		"0000000000",
	),
}


def get_archive() -> Path:
	CACHE_DIR.mkdir(parents=True, exist_ok=True)
	archive = CACHE_DIR / ARCHIVE_NAME
	if not archive.is_file():
		request = Request(ARCHIVE_URL, headers={"User-Agent": "TPT-zh-CN-font-builder"})
		with urlopen(request) as response, archive.open("wb") as output:
			while chunk := response.read(1024 * 1024):
				output.write(chunk)
	digest = sha256(archive.read_bytes()).hexdigest()
	if digest != ARCHIVE_SHA256:
		raise SystemExit(
			f"font archive checksum mismatch: expected {ARCHIVE_SHA256}, got {digest}"
		)
	return archive


def main() -> None:
	target = FontTool(str(FONT_RESOURCE))
	archive = get_archive()
	with TemporaryDirectory(prefix="tpt-zh-font-") as temp_dir:
		with ZipFile(archive) as package:
			package.extract(BDF_MEMBER, temp_dir)
		source = BDFReader(
			str(Path(temp_dir) / BDF_MEMBER),
			BDF_X_OFFSET,
			BDF_Y_OFFSET,
		)

	installed = 0
	missing = []
	for first, last in RANGES:
		for code_point in range(first, last + 1):
			target.code_points[code_point] = False
			if source.code_points[code_point]:
				target.code_points[code_point] = source.code_points[code_point]
				installed += 1
			elif 0x4E00 <= code_point <= 0x9FFF:
				missing.append(code_point)

	for code_point, rows in MANUAL_GLYPHS.items():
		target.code_points[code_point] = FontTool.pack(
			[[3 if pixel == "1" else 0 for pixel in row] for row in rows]
		)
		if code_point in missing:
			missing.remove(code_point)

	target.commit()
	available = sum(bool(item) for item in target.code_points)
	icons = sum(bool(target.code_points[cp]) for cp in range(0xE000, 0xF900))
	print(f"installed {installed} OFL pixel glyphs; {available} total glyphs")
	print(f"preserved {icons} private-use icon glyphs")
	print(f"missing CJK unified ideographs: {len(missing)}")
	print(f"font resource size: {FONT_RESOURCE.stat().st_size} bytes")


if __name__ == "__main__":
	main()
