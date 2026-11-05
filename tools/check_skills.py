"""Check this repository's skill catalog without third-party dependencies.

Frontmatter intentionally uses a YAML-compatible subset: one JSON-quoted string
per key (name, description, license). This is not a general YAML validator.
Run from any directory with Python 3.9+; no files or network state are modified.
"""

import json
import re
import sys
from pathlib import Path
from urllib.parse import unquote, urlsplit


def read_frontmatter(text):
    match = re.match(r"\A---\n(.*?)\n---\n", text, re.S)
    if not match:
        raise ValueError("missing frontmatter")
    fields = {}
    for line in match[1].splitlines():
        key, sep, value = line.partition(":")
        if not sep or key not in {"name", "description", "license"} or key in fields:
            raise ValueError("unexpected or duplicate frontmatter field")
        fields[key] = json.loads(value.strip())
        if not isinstance(fields[key], str) or not fields[key].strip():
            raise ValueError("frontmatter values must be nonempty quoted strings")
    if set(fields) != {"name", "description", "license"}:
        raise ValueError("name, description and license are required")
    name = fields["name"]
    if not re.fullmatch(r"[a-z0-9]+(?:-[a-z0-9]+)*", name) or len(name) > 64:
        raise ValueError("invalid skill name")
    desc = fields["description"]
    if len(desc) > 1024 or any(c in desc for c in "<>"):
        raise ValueError("invalid description")
    if re.search(r"^\s*\[TODO:.*\]\s*$", text[match.end():], re.M):
        raise ValueError("unfinished scaffold")
    return fields


def without_fences(text):
    return re.sub(r"^```[^\n]*\n.*?^```\s*$", "", text, flags=re.M | re.S)


def anchors(text):
    result = set()
    counts = {}
    for heading in re.findall(r"^#{1,6}\s+(.+)$", without_fences(text), re.M):
        slug = re.sub(r"[^\w\- ]", "", heading.lower()).replace(" ", "-")
        count = counts.get(slug, 0)
        result.add(slug if count == 0 else f"{slug}-{count}")
        counts[slug] = count + 1
    return result


def check(root):
    root = root.resolve()
    errors = []
    links = 0
    manifest = json.loads((root / "docs/skill-sources/manifest.json").read_text(encoding="utf-8"))
    entries = manifest["skills"]
    expected = {s["name"] for s in entries}
    if len(expected) != len(entries):
        errors.append("duplicate skill in manifest")
    paths = []
    by_client = {}
    for client in (".agents", ".claude"):
        found = {p.parent.name: p for p in (root / client / "skills").glob("*/SKILL.md")}
        by_client[client] = found
        if set(found) != expected:
            errors.append(f"{client}: missing={sorted(expected-set(found))}, extra={sorted(set(found)-expected)}")
        for name, path in found.items():
            paths.append(path)
            try:
                fields = read_frontmatter(path.read_text(encoding="utf-8"))
                if fields["name"] != name:
                    errors.append(f"{path.relative_to(root)}: directory/name mismatch")
                entry = next((s for s in entries if s["name"] == name), None)
                if entry and any(fields[k] != entry[k] for k in ("description", "license")):
                    errors.append(f"{name}: metadata differs from manifest")
            except (ValueError, KeyError) as exc:
                errors.append(f"{path.relative_to(root)}: {exc}")
    for name in expected:
        a, b = (by_client[c].get(name) for c in (".agents", ".claude"))
        if a and b and a.read_bytes() != b.read_bytes():
            errors.append(f"{name}: client copies differ")

    sources = manifest["sources"]
    for entry in entries:
        name = entry["name"]
        if not entry["sources"]:
            errors.append(f"{name}: no provenance")
        for key in entry["sources"]:
            source = sources.get(key)
            if not source:
                errors.append(f"{name}: missing source {key}")
                continue
            if name not in source["used_by"]:
                errors.append(f"{name}: source reverse mapping missing")
            if not re.fullmatch(r"[0-9a-f]{40}", source["commit"]) or not re.fullmatch(r"[0-9a-f]{64}", source["sha256"]):
                errors.append(f"{key}: invalid source digest")
            pinned = f"https://github.com/{source['repo']}/blob/{source['commit']}/{source['path']}"
            if source["url"] != pinned:
                errors.append(f"{key}: source URL is not pinned")
            license_path = source.get("license_file")
            if not license_path or not (root / license_path).is_file():
                errors.append(f"{key}: missing license file")
            for client in (".agents", ".claude"):
                path = by_client[client].get(name)
                if path and source["url"] not in path.read_text(encoding="utf-8"):
                    errors.append(f"{client}/{name}: source link missing")
    for key, source in sources.items():
        actual = {s["name"] for s in entries if key in s["sources"]}
        if set(source["used_by"]) != actual:
            errors.append(f"{key}: source consumers differ from manifest")

    docs = ["AGENTS.md", "CLAUDE.md", "docs/SKILLS.md", "docs/CODE_CONVENTIONS.md",
            "docs/에이전트_워크플로_가이드.md", "docs/skill-sources/README.md"]
    paths += [root / p for p in docs if (root / p).exists()]
    index = (root / "docs/SKILLS.md").read_text(encoding="utf-8")
    for name in expected:
        if f"../.agents/skills/{name}/SKILL.md" not in index:
            errors.append(f"{name}: missing catalog link")
    for path in paths:
        text = path.read_text(encoding="utf-8")
        if any(line.rstrip() != line for line in text.splitlines()):
            errors.append(f"{path.relative_to(root)}: trailing whitespace")
        for target in re.findall(r"\[[^\]\n]*\]\(([^)\n]+)\)", without_fences(text)):
            parts = urlsplit(target.strip("<>"))
            if parts.scheme or parts.netloc:
                continue
            destination = (path.parent / unquote(parts.path)).resolve() if parts.path else path
            links += 1
            if not destination.exists():
                errors.append(f"{path.relative_to(root)}: missing link {target}")
            elif parts.fragment and destination.suffix == ".md":
                if unquote(parts.fragment) not in anchors(destination.read_text(encoding="utf-8")):
                    errors.append(f"{path.relative_to(root)}: missing anchor {target}")
    return errors, len(expected), links


def main():
    try:
        errors, count, links = check(Path(__file__).resolve().parents[1])
    except (OSError, ValueError, KeyError, TypeError) as exc:
        print(f"FAIL: {exc}")
        return 1
    for error in errors:
        print(f"FAIL: {error}")
    print(f"{'FAIL' if errors else 'PASS'}: {count} skills, {count} mirror pairs, {links} local links; metadata and provenance checked.")
    return int(bool(errors))


if __name__ == "__main__":
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
    raise SystemExit(main())
