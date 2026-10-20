#!/usr/bin/env python3
"""Bundle the whole dashboard into one self-contained HTML file.

The output inlines the stylesheet, D3, the generated data (site/data/data.js),
and every page, so it opens with a double-click and needs no server or network.
Run build_data.py first.

    python scripts/build_standalone.py --site-dir site --output apex-labs-dashboard.html
"""

from __future__ import annotations

import argparse
import re
from pathlib import Path

# Default paths are relative to the repository root (the folder above scripts/),
# so the script works no matter which directory it is run from.
REPO_ROOT = Path(__file__).resolve().parent.parent

PAGES = {"overview": "index.html", "comparison": "comparison.html", "drivers": "drivers.html", "circuits": "circuits.html", "constructors": "constructors.html", "sources": "sources.html"}


def inline_script(text: str) -> str:
    # A literal "</script" inside inlined code would end the tag early.
    return "<script>\n" + re.sub(r"</(script)", r"<\\/\1", text, flags=re.IGNORECASE) + "\n</script>"


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--site-dir", type=Path, default=REPO_ROOT / "site")
    parser.add_argument("--output", type=Path, default=REPO_ROOT / "apex-labs-dashboard.html")
    args = parser.parse_args()
    site = Path(args.site_dir)
    data_js = site / "data" / "data.js"
    if not data_js.exists():
        raise SystemExit(f"{data_js} not found. Run scripts/build_data.py first.")

    index = (site / "index.html").read_text(encoding="utf-8")
    head_script = re.search(r"<script>.*?</script>", index, flags=re.DOTALL).group(0)  # theme bootstrap
    templates = []
    for key, filename in PAGES.items():
        html = (site / filename).read_text(encoding="utf-8")
        main_inner = re.search(r"<main[^>]*>(.*?)</main>", html, flags=re.DOTALL).group(1)
        templates.append(f'<template data-page-template="{key}">{main_inner}</template>')

    document = "\n".join([
        "<!doctype html>",
        '<html lang="en">',
        "<head>",
        '<meta charset="utf-8">',
        '<meta name="viewport" content="width=device-width, initial-scale=1">',
        '<meta name="color-scheme" content="dark light">',
        "<title>Apex Labs | Race Overview</title>",
        head_script,
        "<style>\n" + (site / "assets/css/styles.css").read_text(encoding="utf-8") + "\n</style>",
        "</head>",
        '<body data-page="overview" class="is-loading">',
        '<div class="app-shell"><aside class="sidebar" data-shell-nav></aside><main class="content" data-page-root></main></div>',
        *templates,
        inline_script((site / "assets/js/vendor/d3.min.js").read_text(encoding="utf-8")),
        inline_script(data_js.read_text(encoding="utf-8")),
        inline_script((site / "assets/js/dashboard.js").read_text(encoding="utf-8")),
        "</body>",
        "</html>",
    ])
    output = Path(args.output)
    output.write_text(document, encoding="utf-8")
    print(f"Wrote {output} ({output.stat().st_size / 1_000_000:.1f} MB)")


if __name__ == "__main__":
    main()
