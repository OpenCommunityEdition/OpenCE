#!/usr/bin/env python3
"""Multi-lane decomp.dev-style treemap viewer for objdiff and semantic reports.

The viewer can combine:
  * the canonical worktree's build/report.json
  * every other local Git worktree's build/report.json
  * manually supplied reports
  * saved/"parked" report snapshots

It never merges source code or branches. It only compares objdiff reports and
builds a synthetic best-available progress view.

When a live ``report.json`` has a newer sibling ``semantic_report.json``, the
viewer overlays its accepted COFF-normalized exact set. Compiler-local ``$L``
continuation labels are not shown as standalone functions.

Typical use from the canonical Halo worktree:

    python tools/decomp_treemap_multilane.py build/report.json --all-worktrees

Include parked report snapshots:

    python tools/decomp_treemap_multilane.py build/report.json --all-worktrees \
        --parked-dir parked_reports

Add one report explicitly:

    python tools/decomp_treemap_multilane.py build/report.json --all-worktrees \
        --source object-types=C:/path/to/build/report.json \
        --parked old-best=C:/path/to/old/report.json

Only Python's standard library is used.
"""

from __future__ import annotations

import argparse
import json
import subprocess
import threading
import time
import webbrowser
from dataclasses import dataclass
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any, Callable, Iterable
from urllib.parse import parse_qs, urlparse

HTML = r'''<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Multi-lane Decomp Treemap</title>
<style>
  :root { color-scheme: dark; font-family: Inter, ui-sans-serif, system-ui, sans-serif; }
  * { box-sizing: border-box; }
  body { margin: 0; background: #11151d; color: #e8edf5; }
  header { display: flex; flex-wrap: wrap; align-items: center; gap: 9px; padding: 11px 14px; background: #181c25; border-bottom: 1px solid #2d3442; }
  h1 { font-size: 17px; margin: 0 8px 0 0; }
  button, input, select { color: #e8edf5; background: #242b38; border: 1px solid #3a4558; border-radius: 5px; padding: 7px 9px; }
  button { cursor: pointer; }
  button:disabled { opacity: .45; cursor: default; }
  input { min-width: 245px; }
  #summary { font-size: 13px; color: #aeb8c8; margin-left: auto; }
  #status { padding: 7px 14px; font-size: 12px; color: #9da9ba; background: #151a23; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
  #legend { padding: 6px 14px; font-size: 12px; color: #aeb8c8; background: #131821; border-bottom: 1px solid #242c3a; }
  #wrap { position: relative; width: 100%; height: calc(100vh - 126px); min-height: 420px; }
  canvas { display: block; width: 100%; height: 100%; background: #181c25; }
  #tooltip { position: fixed; pointer-events: none; display: none; max-width: min(820px, 92vw); padding: 8px 10px; border-radius: 5px; background: #f4f6f8; color: #11151d; font-size: 13px; white-space: pre-line; box-shadow: 0 4px 18px #0009; z-index: 10; }
</style>
</head>
<body>
<header>
  <h1 id="title">Multi-lane Decomp Treemap</h1>
  <button id="back" disabled>Back to objects</button>
  <button id="reload">Reload</button>
  <select id="mode" title="Progress view">
    <option value="union">All-lanes union</option>
    <option value="best">Best single lane</option>
    <option value="canonical">Canonical only</option>
  </select>
  <input id="filter" placeholder="Filter: objects, >90%, is:external, source:lane…">
  <span id="summary"></span>
</header>
<div id="status">Loading reports…</div>
<div id="legend">Green = proven exact. Blue intensity = fuzzy match percentage; a blue 100% is not yet hardened. Diagonal lines = progress exists outside canonical. Double lines = parked report contributes.</div>
<div id="wrap"><canvas id="map"></canvas></div>
<div id="tooltip"></div>
<script>
'use strict';

const canvas = document.getElementById('map');
const ctx = canvas.getContext('2d');
const tooltip = document.getElementById('tooltip');
const filterInput = document.getElementById('filter');
const modeSelect = document.getElementById('mode');
const backButton = document.getElementById('back');
const reloadButton = document.getElementById('reload');
const title = document.getElementById('title');
const summary = document.getElementById('summary');
const statusLine = document.getElementById('status');

let envelope = null;
let reportStamp = null;
let rootUnits = [];
let displayed = [];
let currentUnit = null;
let hovered = null;
let cssWidth = 1;
let cssHeight = 1;

function formatPercent(n) {
  if (!Number.isFinite(n)) return '0.00%';
  if (n < 100 && n >= 99.995) return `${n.toFixed(4)}%`;
  return `${n.toFixed(2)}%`;
}

function formatSize(n) {
  const units = ['B', 'kB', 'MB', 'GB', 'TB'];
  let i = 0;
  let v = Number(n) || 0;
  while (v >= 1000 && i < units.length - 1) { v /= 1000; i++; }
  return `${v.toFixed(2)} ${units[i]}`;
}

function pctFor(item) {
  if (modeSelect.value === 'canonical') return item.canonicalPct;
  if (modeSelect.value === 'best') return item.bestPct;
  return item.unionPct;
}

function exactFor(item) {
  if (modeSelect.value === 'canonical') return item.canonicalExact;
  if (modeSelect.value === 'best') return item.bestExact;
  return item.unionExact;
}

function linkedFor(item) {
  if (modeSelect.value === 'canonical') return item.canonicalLinked;
  return item.anyLinked;
}

// Same basic binary treemap rule used by decomp.dev's Rust streemap::binary.
function binaryLayout(items, x, y, w, h) {
  if (items.length === 0) return;
  if (items.length === 1) {
    Object.assign(items[0], {x, y, w, h});
    return;
  }
  const total = items.reduce((s, item) => s + Math.max(0, item.total), 0);
  if (total <= 0) {
    const each = (w >= h ? w : h) / items.length;
    items.forEach((item, i) => Object.assign(item, w >= h
      ? {x: x + i * each, y, w: each, h}
      : {x, y: y + i * each, w, h: each}));
    return;
  }
  const half = total / 2;
  let running = items[0].total;
  let split = 1;
  let firstWeight = running;
  let best = Math.abs(half - running);
  for (let i = 2; i < items.length; i++) {
    running += items[i - 1].total;
    const diff = Math.abs(half - running);
    if (diff <= best) {
      best = diff;
      split = i;
      firstWeight = running;
    }
  }
  const ratio = Math.min(1, Math.max(0, firstWeight / total));
  const a = items.slice(0, split);
  const b = items.slice(split);
  if (w >= h) {
    const aw = w * ratio;
    binaryLayout(a, x, y, aw, h);
    binaryLayout(b, x + aw, y, w - aw, h);
  } else {
    const ah = h * ratio;
    binaryLayout(a, x, y, w, ah);
    binaryLayout(b, x, y + ah, w, h - ah);
  }
}

function hslToRgb(h, s, l) {
  h = ((h % 360) + 360) % 360 / 360;
  s /= 100; l /= 100;
  if (s === 0) return [l * 255, l * 255, l * 255];
  const hue2rgb = (p, q, t) => {
    if (t < 0) t += 1;
    if (t > 1) t -= 1;
    if (t < 1/6) return p + (q - p) * 6 * t;
    if (t < 1/2) return q;
    if (t < 2/3) return p + (q - p) * (2/3 - t) * 6;
    return p;
  };
  const q = l < .5 ? l * (1 + s) : l + s - l * s;
  const p = 2 * l - q;
  return [hue2rgb(p, q, h + 1/3) * 255, hue2rgb(p, q, h) * 255, hue2rgb(p, q, h - 1/3) * 255];
}

function mix(c1, c2, t) {
  t = Math.max(0, Math.min(1, t));
  return c1.map((v, i) => Math.round(v + (c2[i] - v) * t));
}
function rgb(c) { return `rgb(${c[0]} ${c[1]} ${c[2]})`; }
function colors(pct, exact) {
  if (exact) return [rgb(hslToRgb(120, 100, 39)), rgb(hslToRgb(120, 100, 17))];
  const t = Math.max(0, Math.min(100, pct)) / 100;
  return [
    rgb(mix(hslToRgb(200, 0, 21), hslToRgb(200, 100, 35), t)),
    rgb(mix(hslToRgb(200, 0, 15), hslToRgb(200, 100, 15), t)),
  ];
}

function matchesFilter(item, text) {
  if (!text.trim()) return true;
  const terms = text.toLowerCase().trim().split(/\s+/);
  const sizeUnits = {b:1, kb:1000, mb:1000000, gb:1000000000, tb:1000000000000};
  return terms.every(term => {
    if (term === 'is:linked') return linkedFor(item);
    if (term === 'is:unlinked') return !linkedFor(item);
    if (term === 'is:external') return item.unionPct > item.canonicalPct + 0.0001;
    if (term === 'is:parked') return item.parkedContribution;
    if (term === 'is:canonical') return item.unionPct <= item.canonicalPct + 0.0001;
    if (term.startsWith('source:')) {
      const needle = term.slice('source:'.length);
      return item.contributors.some(x => x.toLowerCase().includes(needle)) ||
             item.bestSource.toLowerCase().includes(needle);
    }
    const m = term.match(/^(>=|<=|==|!=|>|<|=)(\d+(?:\.\d+)?)(%|b|kb|mb|gb|tb)$/i);
    if (m) {
      const left = m[3] === '%' ? pctFor(item) : item.total;
      const right = Number(m[2]) * (m[3] === '%' ? 1 : sizeUnits[m[3].toLowerCase()]);
      return ({'>':left>right,'<':left<right,'>=':left>=right,'<=':left<=right,'=':left===right,'==':left===right,'!=':left!==right})[m[1]];
    }
    return item.name.toLowerCase().includes(term);
  });
}

function currentItems() { return currentUnit ? currentUnit.functions : rootUnits; }

function updateItems() {
  const source = currentItems();
  const filter = filterInput.value;
  displayed = source.map(item => ({...item, filtered: !matchesFilter(item, filter)}));
  binaryLayout(displayed, 0, 0, cssWidth, cssHeight);
  hovered = null;
  draw();
  updateSummary();
}

function updateSummary() {
  const visible = displayed.filter(x => !x.filtered);
  const total = visible.reduce((s, x) => s + x.total, 0);
  const weighted = total ? visible.reduce((s, x) => s + x.total * pctFor(x), 0) / total : 0;
  const exact = visible.filter(exactFor).length;
  const external = visible.filter(x => x.unionPct > x.canonicalPct + 0.0001).length;
  summary.textContent = `${visible.length}/${displayed.length} shown • ${formatSize(total)} • ${formatPercent(weighted)} • ${exact} exact • ${external} external`;
}

function resize() {
  const rect = canvas.getBoundingClientRect();
  cssWidth = Math.max(1, rect.width);
  cssHeight = Math.max(1, rect.height);
  const dpr = window.devicePixelRatio || 1;
  canvas.width = Math.round(cssWidth * dpr);
  canvas.height = Math.round(cssHeight * dpr);
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  updateItems();
}

function hatch(item, parked) {
  const delta = Math.max(0, pctFor(item) - item.canonicalPct);
  if (delta <= 0.0001 || modeSelect.value === 'canonical') return;
  ctx.save();
  ctx.beginPath();
  ctx.rect(item.x, item.y, item.w, item.h);
  ctx.clip();
  ctx.globalAlpha = Math.min(.72, .18 + delta / 140);
  ctx.strokeStyle = parked ? '#ffca5c' : '#f5f7fb';
  ctx.lineWidth = parked ? 1.7 : 1.1;
  const spacing = parked ? 8 : 11;
  for (let d = -item.h; d < item.w + item.h; d += spacing) {
    ctx.beginPath();
    ctx.moveTo(item.x + d, item.y + item.h);
    ctx.lineTo(item.x + d + item.h, item.y);
    ctx.stroke();
    if (parked) {
      ctx.beginPath();
      ctx.moveTo(item.x + d + 3, item.y + item.h);
      ctx.lineTo(item.x + d + item.h + 3, item.y);
      ctx.stroke();
    }
  }
  ctx.restore();
}

function draw() {
  ctx.clearRect(0, 0, cssWidth, cssHeight);
  ctx.fillStyle = '#181c25';
  ctx.fillRect(0, 0, cssWidth, cssHeight);
  ctx.lineWidth = 1;
  ctx.strokeStyle = '#000';
  for (const item of displayed) {
    const [inner, outer] = colors(pctFor(item), exactFor(item));
    const g = ctx.createRadialGradient(
      item.x + item.w * .4, item.y + item.h * .4, (item.w + item.h) * .1,
      item.x + item.w * .4, item.y + item.h * .4, (item.w + item.h) * .5
    );
    g.addColorStop(0, inner); g.addColorStop(1, outer);
    ctx.globalAlpha = item.filtered ? .1 : 1;
    ctx.fillStyle = g;
    ctx.fillRect(item.x, item.y, item.w, item.h);
    ctx.strokeRect(item.x, item.y, item.w, item.h);
    if (!item.filtered) hatch(item, item.parkedContribution);
  }
  ctx.globalAlpha = 1;
  if (hovered) {
    ctx.lineWidth = 2;
    ctx.strokeStyle = '#fff';
    ctx.strokeRect(hovered.x, hovered.y, hovered.w, hovered.h);
  }
}

function itemAt(clientX, clientY) {
  const rect = canvas.getBoundingClientRect();
  const x = clientX - rect.left, y = clientY - rect.top;
  let near = null;
  for (const item of displayed) {
    if (item.filtered) continue;
    if (x >= item.x && x <= item.x + item.w && y >= item.y && y <= item.y + item.h) return item;
    if (!near && x >= item.x - 3 && x <= item.x + item.w + 3 && y >= item.y - 3 && y <= item.y + item.h + 3) near = item;
  }
  return near;
}

function showTooltip(item, clientX, clientY) {
  if (!item) { tooltip.style.display = 'none'; return; }
  const lines = [
    `${item.name} • ${formatSize(item.total)}`,
    `canonical ${formatPercent(item.canonicalPct)} • best lane ${formatPercent(item.bestPct)} • union ${formatPercent(item.unionPct)}`,
    `proven exact: canonical ${item.canonicalExact ? 'yes' : 'no'} • best ${item.bestExact ? 'yes' : 'no'} • union ${item.unionExact ? 'yes' : 'no'}`,
    `best source: ${item.bestSource || 'canonical'}`,
  ];
  if (item.contributors.length) lines.push(`external contributors: ${item.contributors.join(', ')}`);
  if (item.anyLinked) lines.push(`linked somewhere${item.canonicalLinked ? ' (including canonical)' : ''}`);
  tooltip.textContent = lines.join('\n');
  tooltip.style.display = 'block';
  const pad = 12;
  const tw = tooltip.offsetWidth, th = tooltip.offsetHeight;
  tooltip.style.left = `${Math.min(window.innerWidth - tw - pad, Math.max(pad, clientX + 14))}px`;
  tooltip.style.top = `${Math.min(window.innerHeight - th - pad, Math.max(pad, clientY + 14))}px`;
}

canvas.addEventListener('mousemove', e => {
  const item = itemAt(e.clientX, e.clientY);
  if (item !== hovered) { hovered = item; draw(); }
  canvas.style.cursor = item ? 'pointer' : 'default';
  showTooltip(item, e.clientX, e.clientY);
});
canvas.addEventListener('mouseleave', () => { hovered = null; tooltip.style.display = 'none'; draw(); });
canvas.addEventListener('click', e => {
  const item = itemAt(e.clientX, e.clientY);
  if (!item || currentUnit || !item.functions || item.functions.length === 0) return;
  currentUnit = rootUnits[item.index];
  title.textContent = currentUnit.name;
  backButton.disabled = false;
  filterInput.value = '';
  updateItems();
});
backButton.addEventListener('click', () => {
  currentUnit = null;
  title.textContent = 'Multi-lane Decomp Treemap';
  backButton.disabled = true;
  filterInput.value = '';
  updateItems();
});
filterInput.addEventListener('input', updateItems);
modeSelect.addEventListener('change', updateItems);
reloadButton.addEventListener('click', () => loadReport(true));
new ResizeObserver(resize).observe(canvas);

async function loadReport(force = false) {
  try {
    const response = await fetch(force ? '/api/report?force=1' : '/api/report', {cache:'no-store'});
    if (!response.ok) throw new Error(await response.text());
    const next = await response.json();
    if (!force && reportStamp === next.stamp) return;
    reportStamp = next.stamp;
    envelope = next;
    rootUnits = next.units;
    currentUnit = null;
    title.textContent = 'Multi-lane Decomp Treemap';
    backButton.disabled = true;
    updateItems();
    const sourceNames = next.sources.map(s => s.label).join(', ');
    const semanticCount = next.sources.filter(s => s.semanticPath).length;
    const warningText = next.warnings.length ? ` • warnings: ${next.warnings.length}` : '';
    statusLine.textContent = `Loaded ${next.sources.length} reports (${semanticCount} normalized): ${sourceNames}${warningText}`;
  } catch (err) {
    statusLine.textContent = `Error: ${err.message}`;
  }
}

loadReport(false);
setInterval(() => loadReport(false), 30000);
</script>
</body>
</html>
'''


@dataclass(frozen=True)
class SourceSpec:
    label: str
    path: Path
    kind: str  # canonical, lane, parked, extra
    branch: str | None = None
    parked_policy_path: Path | None = None

    @property
    def id(self) -> str:
        return (
            f"{self.kind}:{self.label}:{self.path}:"
            f"parked-policy:{self.parked_policy_path or ''}"
        )

    @property
    def semantic_path(self) -> Path | None:
        """Return the live semantic companion, never a parked snapshot peer."""
        if self.kind == 'parked' or self.path.name != 'report.json':
            return None
        return self.path.with_name('semantic_report.json')


def value(obj: dict[str, Any] | None, snake: str, camel: str, default: Any = 0) -> Any:
    if not obj:
        return default
    if snake in obj:
        return obj[snake]
    if camel in obj:
        return obj[camel]
    return default


def parse_labeled_path(raw: str, option: str) -> tuple[str, Path]:
    if '=' not in raw:
        raise argparse.ArgumentTypeError(f"{option} must be LABEL=PATH")
    label, raw_path = raw.split('=', 1)
    label = label.strip()
    raw_path = raw_path.strip().strip('"')
    if not label or not raw_path:
        raise argparse.ArgumentTypeError(f"{option} must be LABEL=PATH")
    return label, Path(raw_path).expanduser().resolve()


def git_worktrees(repo_root: Path) -> list[dict[str, str]]:
    try:
        result = subprocess.run(
            ['git', '-C', str(repo_root), 'worktree', 'list', '--porcelain'],
            check=True,
            capture_output=True,
            text=True,
            encoding='utf-8',
            errors='replace',
        )
    except (OSError, subprocess.CalledProcessError):
        return []

    blocks: list[dict[str, str]] = []
    current: dict[str, str] = {}
    for raw_line in result.stdout.splitlines() + ['']:
        line = raw_line.strip()
        if not line:
            if current:
                blocks.append(current)
                current = {}
            continue
        key, _, val = line.partition(' ')
        current[key] = val
    return blocks


def safe_label(text: str) -> str:
    return text.replace('refs/heads/', '').replace('\\', '/').strip('/') or 'unnamed'


def discover_sources(args: argparse.Namespace) -> tuple[list[SourceSpec], list[str]]:
    canonical_path = Path(args.report).expanduser().resolve()
    repo_root = Path(args.repo).expanduser().resolve() if args.repo else canonical_path.parent.parent
    warnings: list[str] = []
    specs: list[SourceSpec] = []

    worktrees = git_worktrees(repo_root) if args.all_worktrees else []
    canonical_branch: str | None = None
    canonical_root = canonical_path.parent.parent.resolve()

    for wt in worktrees:
        try:
            wt_root = Path(wt.get('worktree', '')).resolve()
        except OSError:
            continue
        if wt_root == canonical_root:
            canonical_branch = safe_label(wt.get('branch', 'canonical'))
            break

    canonical_label = args.canonical_label or canonical_branch or 'canonical'
    specs.append(SourceSpec(
        canonical_label,
        canonical_path,
        'canonical',
        canonical_branch,
        (repo_root / 'config' / 'parked.json').resolve(),
    ))

    if args.all_worktrees:
        for wt in worktrees:
            raw_root = wt.get('worktree')
            if not raw_root:
                continue
            wt_root = Path(raw_root).resolve()
            report_path = (wt_root / args.worktree_report).resolve()
            if report_path == canonical_path:
                continue
            branch = safe_label(wt.get('branch', ''))
            if not branch or branch == 'unnamed':
                head = wt.get('HEAD', '')[:10]
                branch = f"detached-{head}" if head else wt_root.name
            label = branch
            if report_path.exists():
                specs.append(SourceSpec(label, report_path, 'lane', branch))
            else:
                warnings.append(f"No report for lane {label}: {report_path}")

    for raw in args.source:
        label, path = parse_labeled_path(raw, '--source')
        specs.append(SourceSpec(label, path, 'extra'))

    for raw in args.parked:
        label, path = parse_labeled_path(raw, '--parked')
        specs.append(SourceSpec(label, path, 'parked'))

    for raw_dir in args.parked_dir:
        directory = Path(raw_dir).expanduser().resolve()
        if not directory.exists():
            warnings.append(f"Parked directory not found: {directory}")
            continue
        for path in sorted(directory.rglob('*.json')):
            specs.append(SourceSpec(path.stem, path.resolve(), 'parked'))

    # Deduplicate exact paths. Canonical wins, then lane, extra, parked.
    priority = {'canonical': 0, 'lane': 1, 'extra': 2, 'parked': 3}
    deduped: dict[Path, SourceSpec] = {}
    for spec in sorted(specs, key=lambda s: priority[s.kind]):
        deduped.setdefault(spec.path, spec)

    final = list(deduped.values())
    for spec in final:
        if not spec.path.exists():
            warnings.append(f"Report not found for {spec.label}: {spec.path}")

    return final, warnings


def load_json(path: Path) -> dict[str, Any]:
    with path.open('r', encoding='utf-8') as handle:
        data = json.load(handle)
    if not isinstance(data, dict):
        raise ValueError(f"Report root must be an object: {path}")
    return data


def stat_signature(stat: Any) -> tuple[int, int, int]:
    return (int(stat.st_mtime_ns), int(stat.st_ctime_ns), int(stat.st_size))


def load_json_stable(path: Path, expected_signature: tuple[int, int, int]) -> dict[str, Any]:
    """Read one report only if it stays unchanged for the whole read."""
    data = load_json(path)
    if stat_signature(path.stat()) != expected_signature:
        raise OSError(f"Report changed while it was being read: {path}")
    return data


def function_key(function: dict[str, Any], index: int) -> str:
    address = value(function, 'address', 'address', None)
    size = int(function.get('size') or 0)
    raw_name = str(function.get('name') or '')
    if address is not None:
        return f"address:{int(address)}:size:{size}"
    if raw_name:
        return f"name:{raw_name}:size:{size}"
    return f"index:{index}:size:{size}"


def semantic_acceptance_index(
    semantic_report: dict[str, Any] | None,
) -> dict[str, dict[str, dict[str, int]]]:
    """Index strict semantic-COFF evidence by unit, name, and measured size."""
    candidates: dict[tuple[str, str], list[dict[str, int]]] = {}
    if not semantic_report:
        return {}
    entries = semantic_report.get('accepted_ledger')
    if not isinstance(entries, list):
        return {}
    for entry in entries:
        if not isinstance(entry, dict):
            continue
        unit_name = entry.get('unit')
        function_name = entry.get('function')
        if not isinstance(unit_name, str) or not isinstance(function_name, str):
            continue
        proof_sources = entry.get('proof_sources')
        if not isinstance(proof_sources, list) or 'semantic-coff' not in proof_sources:
            continue
        # Local continuations are portions of their owning COMDAT, not owners.
        if function_name.startswith('$'):
            continue
        try:
            code_bytes = int(entry['code_bytes'])
            padded_bytes = int(entry['padded_bytes'])
        except (KeyError, TypeError, ValueError):
            continue
        if code_bytes <= 0 or padded_bytes < code_bytes:
            continue
        candidates.setdefault((unit_name, function_name), []).append({
            'codeBytes': code_bytes,
            'paddedBytes': padded_bytes,
        })

    # A duplicated proof is ambiguous even when its values happen to agree.
    # Fail closed instead of allowing JSON order to select the winning record.
    accepted: dict[str, dict[str, dict[str, int]]] = {}
    for (unit_name, function_name), records in candidates.items():
        if len(records) == 1:
            accepted.setdefault(unit_name, {})[function_name] = records[0]
    return accepted


def ordinary_rejection_index(
    semantic_report: dict[str, Any] | None,
) -> dict[str, dict[str, int]]:
    """Index raw objdiff 100% results rejected by the hardened comparator."""
    candidates: dict[tuple[str, str], list[int]] = {}
    if not semantic_report:
        return {}
    entries = semantic_report.get('ordinary_rejected')
    if isinstance(entries, dict):
        entries = [entries]
    if not isinstance(entries, list):
        return {}
    for entry in entries:
        if not isinstance(entry, dict):
            continue
        unit_name = entry.get('unit')
        function_name = entry.get('function')
        try:
            code_bytes = int(entry['code_bytes'])
        except (KeyError, TypeError, ValueError):
            continue
        if isinstance(unit_name, str) and isinstance(function_name, str):
            candidates.setdefault((unit_name, function_name), []).append(code_bytes)

    rejected: dict[str, dict[str, int]] = {}
    for (unit_name, function_name), sizes in candidates.items():
        if len(sizes) == 1:
            rejected.setdefault(unit_name, {})[function_name] = sizes[0]
    return rejected


def parked_exactness_veto_index(
    parked_policy: dict[str, Any] | None,
) -> dict[str, set[str]]:
    """Index canonical fuzzy parks that must not receive external exact credit.

    Authenticated assembly is tracked in ``parked.json`` for provenance, but
    it is an accepted implementation policy rather than an unresolved C
    reconstruction.  Every other parked class remains a fail-closed exactness
    veto until canonical removes the park after review.
    """
    if not parked_policy:
        return {}
    entries = parked_policy.get('entries')
    if not isinstance(entries, list):
        return {}

    accepted_assembly_classes = {'asm-implemented', 'vendored-assembly'}
    vetoes: dict[str, set[str]] = {}
    for entry in entries:
        if not isinstance(entry, dict):
            continue
        if entry.get('class') in accepted_assembly_classes:
            continue
        unit_name = entry.get('unit')
        function_name = entry.get('function')
        if isinstance(unit_name, str) and isinstance(function_name, str):
            vetoes.setdefault(unit_name, set()).add(function_name)
    return vetoes


def exact_intervals_cover_text(intervals: list[tuple[int, int]], text_size: int) -> bool:
    """Require exact owners to tile the complete target .text range."""
    if text_size <= 0 or not intervals:
        return False
    cursor = 0
    for start, end in sorted(intervals):
        if start != cursor or end <= start:
            return False
        cursor = end
    return cursor == text_size


def normalize_source(
    spec: SourceSpec,
    report: dict[str, Any],
    semantic_report: dict[str, Any] | None = None,
    parked_policy: dict[str, Any] | None = None,
) -> dict[str, Any]:
    units_out: dict[str, dict[str, Any]] = {}
    semantic_index = semantic_acceptance_index(semantic_report)
    rejection_index = ordinary_rejection_index(semantic_report)
    parked_veto_index = (
        parked_exactness_veto_index(parked_policy)
        if spec.kind == 'canonical'
        else {}
    )
    units = report.get('units') if isinstance(report.get('units'), list) else []
    for unit_index, raw_unit in enumerate(units):
        if not isinstance(raw_unit, dict):
            continue
        name = str(raw_unit.get('name') or f'unit-{unit_index}')
        measures = raw_unit.get('measures') if isinstance(raw_unit.get('measures'), dict) else {}
        metadata = raw_unit.get('metadata') if isinstance(raw_unit.get('metadata'), dict) else {}
        functions = raw_unit.get('functions') if isinstance(raw_unit.get('functions'), list) else []
        sections = raw_unit.get('sections') if isinstance(raw_unit.get('sections'), list) else []
        total = int(value(measures, 'total_code', 'totalCode', 0) or 0)
        raw_pct = float(value(measures, 'fuzzy_match_percent', 'fuzzyMatchPercent', 0) or 0)
        configured_linked = bool(value(metadata, 'complete', 'complete', False))
        accepted_records = semantic_index.get(name, {})
        rejected_names = rejection_index.get(name, {})
        policy_parked_names = parked_veto_index.get(name, set())
        raw_name_counts: dict[str, int] = {}
        for raw_fn in functions:
            if not isinstance(raw_fn, dict):
                continue
            raw_name = str(raw_fn.get('name') or '')
            if raw_name and not raw_name.startswith('$'):
                raw_name_counts[raw_name] = raw_name_counts.get(raw_name, 0) + 1
        text_size = 0
        for section in sections:
            if isinstance(section, dict) and section.get('name') == '.text':
                try:
                    text_size = int(section.get('size') or 0)
                except (TypeError, ValueError):
                    text_size = 0
                break

        semantic_accepted_indices: set[int] = set()
        semantic_intervals: list[tuple[int, int]] = []
        for function_index, raw_fn in enumerate(functions):
            if not isinstance(raw_fn, dict):
                continue
            raw_name = str(raw_fn.get('name') or '')
            if raw_name_counts.get(raw_name) != 1:
                continue
            record = accepted_records.get(raw_name)
            if record is None:
                continue
            try:
                size = int(raw_fn.get('size') or 0)
                address = int(raw_fn.get('address'))
            except (TypeError, ValueError):
                continue
            interval_end = address + record['paddedBytes']
            if (
                size != record['codeBytes']
                or address < 0
                or text_size <= 0
                or interval_end > text_size
            ):
                continue
            semantic_accepted_indices.add(function_index)
            semantic_intervals.append((address, interval_end))

        semantic_complete = exact_intervals_cover_text(semantic_intervals, text_size)
        fn_out: dict[str, dict[str, Any]] = {}
        fn_order: list[str] = []
        local_continuation_bytes = 0
        has_ordinary_rejection = False
        for function_index, raw_fn in enumerate(functions):
            if not isinstance(raw_fn, dict):
                continue
            size = int(raw_fn.get('size') or 0)
            if size <= 0:
                continue
            raw_name = str(raw_fn.get('name') or '')
            # objdiff exposes some compiler-local continuation labels as
            # pseudo-functions. Hide one only when an exact owner interval
            # proves that the label is contained inside that COMDAT.
            if raw_name.startswith('$'):
                try:
                    local_start = int(raw_fn.get('address'))
                except (TypeError, ValueError):
                    local_start = -1
                local_end = local_start + size
                if local_start >= 0 and any(
                    start <= local_start and local_end <= end
                    for start, end in semantic_intervals
                ):
                    local_continuation_bytes += size
                    continue
            if semantic_complete and function_index not in semantic_accepted_indices:
                continue
            key = function_key(raw_fn, function_index)
            fn_meta = raw_fn.get('metadata') if isinstance(raw_fn.get('metadata'), dict) else {}
            display_name = str(value(fn_meta, 'demangled_name', 'demangledName', '') or raw_fn.get('name') or f'function-{function_index}')
            raw_function_pct = float(value(raw_fn, 'fuzzy_match_percent', 'fuzzyMatchPercent', 0) or 0)
            semantic_accepted = function_index in semantic_accepted_indices
            ordinary_rejected = (
                raw_name_counts.get(raw_name) == 1
                and rejected_names.get(raw_name) == size
            )
            policy_parked = raw_name in policy_parked_names
            has_ordinary_rejection = has_ordinary_rejection or ordinary_rejected
            effective_pct = 100.0 if semantic_accepted else max(0.0, min(100.0, raw_function_pct))
            function_exact = not policy_parked and (
                semantic_accepted or (
                    raw_function_pct >= 100.0 and not ordinary_rejected
                )
            )
            fn_out[key] = {
                'key': key,
                'name': display_name,
                'rawName': raw_name,
                'total': size,
                'pct': effective_pct,
                'rawPct': raw_function_pct,
                'linked': configured_linked,
                'address': value(raw_fn, 'address', 'address', None),
                'semanticAccepted': semantic_accepted,
                'ordinaryRejected': ordinary_rejected,
                'policyParked': policy_parked,
                'exact': function_exact,
            }
            fn_order.append(key)
        function_total = sum(int(function['total']) for function in fn_out.values())
        has_semantic_adjustment = bool(
            semantic_accepted_indices
            or local_continuation_bytes
            or has_ordinary_rejection
        )
        if semantic_complete:
            effective_unit_pct = 100.0
        elif has_semantic_adjustment and function_total:
            effective_unit_pct = sum(
                int(function['total']) * float(function['pct'])
                for function in fn_out.values()
            ) / function_total
        else:
            effective_unit_pct = max(0.0, min(100.0, raw_pct))

        unit_exact = not policy_parked_names and (
            semantic_complete or (
                effective_unit_pct >= 100.0
                and bool(fn_out)
                and all(bool(function['exact']) for function in fn_out.values())
            )
        )
        linked = configured_linked and (
            not policy_parked_names
            and (
                not fn_out
                or all(bool(function['exact']) for function in fn_out.values())
            )
        )
        for function in fn_out.values():
            function['linked'] = linked
        units_out[name] = {
            'name': name,
            'total': (
                sum(
                    int(functions[index].get('size') or 0)
                    for index in semantic_accepted_indices
                )
                if semantic_complete
                else function_total if has_semantic_adjustment else max(0, total)
            ),
            'pct': effective_unit_pct,
            'rawPct': raw_pct,
            'linked': linked,
            'configuredLinked': configured_linked,
            'exact': unit_exact,
            'semanticComplete': semantic_complete and not policy_parked_names,
            'semanticCoverageComplete': semantic_complete,
            'policyParkedNames': sorted(policy_parked_names),
            'textSize': text_size,
            'functions': fn_out,
            'functionOrder': fn_order,
        }
    return {
        'spec': spec,
        'version': report.get('version'),
        'semanticPath': str(spec.semantic_path) if semantic_report is not None else None,
        'parkedPolicyPath': (
            str(spec.parked_policy_path) if parked_policy is not None else None
        ),
        'units': units_out,
    }


def weighted_union_pct(total: int, best_single_pct: float, functions: list[dict[str, Any]]) -> float:
    if total <= 0:
        return max(0.0, min(100.0, best_single_pct))
    function_bytes = sum(max(0, int(fn['total'])) for fn in functions)
    matched_equivalent = sum(max(0, int(fn['total'])) * float(fn['unionPct']) / 100.0 for fn in functions)
    # Some code bytes may not be represented as normal functions. Treat the
    # unexplained remainder at the exact best-single-lane unit percentage.
    residual = max(0, total - function_bytes)
    matched_equivalent += residual * best_single_pct / 100.0
    denominator = max(total, function_bytes)
    function_union = 100.0 * matched_equivalent / denominator if denominator else 0.0
    return max(best_single_pct, min(100.0, function_union))


def merge_reports(loaded: list[dict[str, Any]], warnings: list[str]) -> dict[str, Any]:
    if not loaded:
        raise ValueError('No readable reports were found')

    canonical = next((x for x in loaded if x['spec'].kind == 'canonical'), loaded[0])
    canonical_units = canonical['units']
    source_positions = {id(source): index for index, source in enumerate(loaded)}

    unit_order = list(canonical_units.keys())
    seen_units = set(unit_order)
    for source in loaded:
        for name in source['units']:
            if name not in seen_units:
                seen_units.add(name)
                unit_order.append(name)

    merged_units: list[dict[str, Any]] = []
    incompatible_by_source: dict[str, int] = {}

    for unit_name in unit_order:
        variants: list[tuple[dict[str, Any], dict[str, Any]]] = []
        for source in loaded:
            unit = source['units'].get(unit_name)
            if unit:
                variants.append((source, unit))
        if not variants:
            continue

        canonical_unit = canonical_units.get(unit_name)
        reference_unit = canonical_unit or max((u for _, u in variants), key=lambda u: u['total'])
        reference_text_size = int(reference_unit.get('textSize') or 0)
        if reference_text_size:
            compatible_variants = []
            for source, unit in variants:
                if int(unit.get('textSize') or 0) == reference_text_size:
                    compatible_variants.append((source, unit))
                else:
                    label = source['spec'].label
                    incompatible_by_source[label] = incompatible_by_source.get(label, 0) + 1
            variants = compatible_variants
        if not variants:
            continue

        total = int(reference_unit['total'] or max(u['total'] for _, u in variants))
        canonical_ordinary_rejections = {
            key for key, function in (canonical_unit or {}).get('functions', {}).items()
            if function.get('ordinaryRejected')
        }
        canonical_policy_vetoes = set(
            (canonical_unit or {}).get('policyParkedNames', [])
        )

        def policy_vetoes(function: dict[str, Any]) -> bool:
            return str(function.get('rawName') or '') in canonical_policy_vetoes

        def function_metrics(
            source: dict[str, Any],
            key: str,
            function: dict[str, Any],
        ) -> tuple[float, bool]:
            pct = float(function['pct'])
            exact = bool(function.get('exact'))
            semantic_accepted = bool(function.get('semanticAccepted'))
            if policy_vetoes(function):
                exact = False
            elif key in canonical_ordinary_rejections and not semantic_accepted:
                exact = False
            # A stale or missing lane sidecar cannot turn an external raw 100%
            # into a proven exact result.  It remains useful fuzzy evidence.
            if source is not canonical and pct >= 100.0 and not semantic_accepted:
                exact = False
            return pct, exact

        def unit_metrics(
            source: dict[str, Any],
            unit: dict[str, Any],
        ) -> tuple[float, bool]:
            if unit.get('semanticComplete'):
                return 100.0, not canonical_policy_vetoes
            if not canonical_ordinary_rejections and not canonical_policy_vetoes:
                pct = max(0.0, min(100.0, float(unit['pct'])))
                exact = bool(unit.get('exact'))
                if source is not canonical and pct >= 100.0:
                    exact = False
                return pct, exact

            function_bytes = 0
            matched_equivalent = 0.0
            blocked_rejection = bool(canonical_policy_vetoes)
            for key in unit['functionOrder']:
                function = unit['functions'].get(key)
                if function is None:
                    continue
                function_pct, _ = function_metrics(source, key, function)
                size = max(0, int(function['total']))
                function_bytes += size
                matched_equivalent += size * function_pct / 100.0
            for key in canonical_ordinary_rejections:
                function = unit['functions'].get(key)
                if function is None or not function.get('semanticAccepted'):
                    blocked_rejection = True

            unit_total = max(0, int(unit['total']))
            pct = float(unit['pct'])
            if function_bytes:
                residual = max(0, unit_total - function_bytes)
                matched_equivalent += residual * pct / 100.0
                denominator = max(unit_total, function_bytes)
                pct = 100.0 * matched_equivalent / denominator if denominator else pct

            exact = bool(unit.get('exact')) and not blocked_rejection
            if source is not canonical and pct >= 100.0:
                exact = False
            return max(0.0, min(100.0, pct)), exact

        variant_metrics = [
            (source, unit, *unit_metrics(source, unit))
            for source, unit in variants
        ]
        canonical_pct, canonical_exact = (
            unit_metrics(canonical, canonical_unit)
            if canonical_unit else (0.0, False)
        )
        canonical_linked = bool(
            canonical_unit
            and canonical_unit['linked']
            and not canonical_policy_vetoes
        )

        def effective_semantic_complete(unit: dict[str, Any]) -> bool:
            return bool(unit.get('semanticComplete')) and not canonical_policy_vetoes

        best_source_obj, best_unit, best_pct, best_exact = max(
            variant_metrics,
            key=lambda item: (
                item[2],
                effective_semantic_complete(item[1]),
                item[3],
                item[0] is canonical,
                -source_positions[id(item[0])],
            ),
        )
        best_source = best_source_obj['spec'].label
        any_linked = not canonical_policy_vetoes and any(
            bool(u['linked']) for _, u in variants
        )

        function_order = list(canonical_unit['functionOrder']) if canonical_unit else list(reference_unit['functionOrder'])
        # A fully covered canonical semantic census is authoritative. Raw lane
        # reports may still expose compiler-local $L continuations as extra
        # pseudo-functions; re-adding those would reopen an exact drill-down.
        if not (
            canonical_unit
            and canonical_unit.get('semanticCoverageComplete')
        ):
            seen_functions = set(function_order)
            for _, unit in variants:
                for key in unit['functionOrder']:
                    if key not in seen_functions:
                        seen_functions.add(key)
                        function_order.append(key)

        merged_functions: list[dict[str, Any]] = []
        unit_contributors: set[str] = set()
        parked_contribution = False

        for function_key_value in function_order:
            fn_variants: list[tuple[dict[str, Any], dict[str, Any]]] = []
            for source, unit in variants:
                fn = unit['functions'].get(function_key_value)
                if fn:
                    fn_variants.append((source, fn))
            if not fn_variants:
                continue

            canonical_fn = canonical_unit['functions'].get(function_key_value) if canonical_unit else None
            reference_fn = canonical_fn or max((fn for _, fn in fn_variants), key=lambda fn: fn['total'])
            fn_total = int(reference_fn['total'])
            fn_canonical_pct, fn_canonical_exact = (
                function_metrics(canonical, function_key_value, canonical_fn)
                if canonical_fn else (0.0, False)
            )
            fn_canonical_linked = canonical_linked and canonical_fn is not None
            fn_variant_metrics = [
                (source, function, *function_metrics(source, function_key_value, function))
                for source, function in fn_variants
            ]
            best_fn_source_obj, best_fn, fn_best_pct, fn_best_exact = max(
                fn_variant_metrics,
                key=lambda item: (
                    item[2],
                    bool(item[1].get('semanticAccepted')),
                    item[3],
                    item[0] is canonical,
                    -source_positions[id(item[0])],
                ),
            )
            fn_best_source = best_fn_source_obj['spec'].label
            fn_any_linked = not policy_vetoes(reference_fn) and any(
                bool(unit['linked']) and function_key_value in unit['functions']
                for _, unit in variants
            )

            contributors = sorted({
                source['spec'].label
                for source, fn, pct, _ in fn_variant_metrics
                if source is not canonical
                and pct > fn_canonical_pct + 0.0001
                and abs(pct - fn_best_pct) < 0.0001
            })
            fn_parked = any(
                source['spec'].kind == 'parked'
                and source['spec'].label in contributors
                for source, _, _, _ in fn_variant_metrics
            )
            unit_contributors.update(contributors)
            parked_contribution = parked_contribution or fn_parked

            merged_functions.append({
                'index': len(merged_functions),
                'key': function_key_value,
                'name': canonical_fn['name'] if canonical_fn else best_fn['name'],
                'total': fn_total,
                'canonicalPct': fn_canonical_pct,
                'bestPct': fn_best_pct,
                'unionPct': fn_best_pct,
                'canonicalExact': fn_canonical_exact,
                'bestExact': fn_best_exact,
                'unionExact': fn_best_exact,
                'bestSource': fn_best_source,
                'contributors': contributors,
                'parkedContribution': fn_parked,
                'canonicalLinked': fn_canonical_linked,
                'anyLinked': fn_any_linked,
                'policyParked': policy_vetoes(reference_fn),
                'canonicalSemanticAccepted': bool(canonical_fn and canonical_fn.get('semanticAccepted')),
                'anySemanticAccepted': any(bool(fn.get('semanticAccepted')) for _, fn in fn_variants),
            })

        union_pct = weighted_union_pct(total, best_pct, merged_functions)
        represented_bytes = sum(int(function['total']) for function in merged_functions)
        union_exact = not canonical_policy_vetoes and (
            best_exact or bool(
                merged_functions
                and represented_bytes == total
                and all(bool(function['unionExact']) for function in merged_functions)
            )
        )
        if best_pct > canonical_pct + 0.0001:
            unit_contributors.add(best_source)
            if best_source_obj['spec'].kind == 'parked':
                parked_contribution = True

        merged_units.append({
            'index': len(merged_units),
            'name': unit_name,
            'total': total,
            'canonicalPct': canonical_pct,
            'bestPct': best_pct,
            'unionPct': union_pct,
            'canonicalExact': canonical_exact,
            'bestExact': best_exact,
            'unionExact': union_exact,
            'bestSource': best_source,
            'contributors': sorted(unit_contributors),
            'parkedContribution': parked_contribution,
            'canonicalLinked': canonical_linked,
            'anyLinked': any_linked,
            'canonicalSemanticComplete': bool(
                canonical_unit and effective_semantic_complete(canonical_unit)
            ),
            'anySemanticComplete': any(
                effective_semantic_complete(u) for _, u in variants
            ),
            'functions': merged_functions,
        })

    for label, count in sorted(incompatible_by_source.items()):
        warnings.append(
            f"Ignored {count} target-layout-incompatible unit(s) from {label}"
        )

    # Detect obvious wrong-version/wrong-baseline reports without rejecting them.
    canonical_total = sum(int(u['total']) for u in canonical_units.values())
    for source in loaded:
        if source is canonical:
            continue
        source_total = sum(int(u['total']) for u in source['units'].values())
        if canonical_total and abs(source_total - canonical_total) / canonical_total > 0.03:
            warnings.append(
                f"{source['spec'].label} total code differs from canonical by more than 3% "
                f"({source_total} vs {canonical_total}); verify it is the same Halo build/version"
            )

    source_rows = []
    for source in loaded:
        stat = source['spec'].path.stat()
        source_rows.append({
            'label': source['spec'].label,
            'kind': source['spec'].kind,
            'branch': source['spec'].branch,
            'path': str(source['spec'].path),
            'mtime': stat.st_mtime,
            'version': source['version'],
            'semanticPath': source['semanticPath'],
            'parkedPolicyPath': source['parkedPolicyPath'],
        })

    return {
        'units': merged_units,
        'sources': source_rows,
        'warnings': warnings,
    }


class ViewerServer(ThreadingHTTPServer):
    daemon_threads = True

    def __init__(
        self,
        address: tuple[str, int],
        sources: list[SourceSpec],
        startup_warnings: list[str],
        source_discovery: Callable[[], tuple[list[SourceSpec], list[str]]] | None = None,
    ):
        self.sources = sources
        self.startup_warnings = startup_warnings
        self.source_discovery = source_discovery
        self._source_lock = threading.Lock()
        self._build_lock = threading.Lock()
        self._last_probe_at = 0.0
        self.probe_interval_seconds = 30.0
        self._cache_stamp: str | None = None
        self._cache_body: bytes | None = None
        self._cache_lock = threading.Lock()
        super().__init__(address, ViewerHandler)

    def build_envelope(self, force: bool = False) -> bytes:
        with self._build_lock:
            now = time.monotonic()
            if (
                not force
                and self._cache_body is not None
                and now - self._last_probe_at < self.probe_interval_seconds
            ):
                return self._cache_body
            self._last_probe_at = now
            return self._refresh_envelope()

    def _refresh_envelope(self) -> bytes:
        if self.source_discovery is not None:
            with self._source_lock:
                sources, discovery_warnings = self.source_discovery()
                self.sources = sources
        else:
            sources = self.sources
            discovery_warnings = self.startup_warnings

        readable_specs: list[SourceSpec] = []
        semantic_paths: dict[str, Path] = {}
        parked_policy_paths: dict[str, Path] = {}
        report_signatures: dict[str, tuple[int, int, int]] = {}
        semantic_signatures: dict[str, tuple[int, int, int]] = {}
        parked_policy_signatures: dict[str, tuple[int, int, int]] = {}
        stamp_parts: list[str] = []
        warnings = list(discovery_warnings)
        for spec in sources:
            try:
                stat = spec.path.stat()
            except FileNotFoundError:
                continue
            except OSError as exc:
                warnings.append(f"Cannot stat {spec.label}: {exc}")
                continue
            readable_specs.append(spec)
            report_signature = stat_signature(stat)
            report_signatures[spec.id] = report_signature
            stamp_parts.append(f"{spec.id}:{report_signature}")
            parked_policy_path = spec.parked_policy_path
            if spec.kind == 'canonical' and parked_policy_path is not None:
                try:
                    parked_policy_stat = parked_policy_path.stat()
                except FileNotFoundError as exc:
                    raise OSError(
                        f"Canonical parked policy not found: {parked_policy_path}"
                    ) from exc
                except OSError as exc:
                    raise OSError(
                        f"Cannot stat canonical parked policy {parked_policy_path}: {exc}"
                    ) from exc
                else:
                    parked_policy_signature = stat_signature(parked_policy_stat)
                    parked_policy_paths[spec.id] = parked_policy_path
                    parked_policy_signatures[spec.id] = parked_policy_signature
                    stamp_parts.append(
                        f"{spec.id}:parked-policy:{parked_policy_signature}"
                    )
            semantic_path = spec.semantic_path
            if semantic_path is None:
                continue
            try:
                semantic_stat = semantic_path.stat()
            except FileNotFoundError:
                continue
            except OSError as exc:
                warnings.append(f"Cannot stat semantic report for {spec.label}: {exc}")
                continue
            semantic_signature = stat_signature(semantic_stat)
            semantic_signatures[spec.id] = semantic_signature
            stamp_parts.append(f"{spec.id}:semantic:{semantic_signature}")
            if semantic_stat.st_mtime_ns < stat.st_mtime_ns:
                warnings.append(
                    f"Semantic report older than objdiff report for {spec.label}; ignored"
                )
                continue
            semantic_paths[spec.id] = semantic_path
        stamp = '|'.join(stamp_parts)

        with self._cache_lock:
            if stamp == self._cache_stamp and self._cache_body is not None:
                return self._cache_body

            loaded: list[dict[str, Any]] = []
            for spec in readable_specs:
                try:
                    report = load_json_stable(spec.path, report_signatures[spec.id])
                except (OSError, json.JSONDecodeError, ValueError) as exc:
                    if spec.kind == 'canonical':
                        raise OSError(f"Cannot load canonical report {spec.label}: {exc}") from exc
                    warnings.append(f"Cannot load {spec.label}: {exc}")
                    continue
                semantic_report = None
                parked_policy = None
                semantic_path = semantic_paths.get(spec.id)
                if semantic_path is not None:
                    try:
                        semantic_report = load_json_stable(
                            semantic_path, semantic_signatures[spec.id]
                        )
                    except (OSError, json.JSONDecodeError, ValueError) as exc:
                        if spec.kind == 'canonical':
                            raise OSError(
                                f"Cannot load canonical semantic report {spec.label}: {exc}"
                            ) from exc
                        warnings.append(f"Cannot load semantic report for {spec.label}: {exc}")
                parked_policy_path = parked_policy_paths.get(spec.id)
                if parked_policy_path is not None:
                    try:
                        parked_policy = load_json_stable(
                            parked_policy_path,
                            parked_policy_signatures[spec.id],
                        )
                    except (OSError, json.JSONDecodeError, ValueError) as exc:
                        raise OSError(
                            f"Cannot load canonical parked policy for {spec.label}: {exc}"
                        ) from exc
                if stat_signature(spec.path.stat()) != report_signatures[spec.id]:
                    if spec.kind == 'canonical':
                        raise OSError(f"Canonical report changed during paired read: {spec.path}")
                    warnings.append(f"Report changed during paired read for {spec.label}; ignored")
                    continue
                if (
                    semantic_path is not None
                    and stat_signature(semantic_path.stat()) != semantic_signatures[spec.id]
                ):
                    if spec.kind == 'canonical':
                        raise OSError(
                            f"Canonical semantic report changed during paired read: {semantic_path}"
                        )
                    warnings.append(
                        f"Semantic report changed during paired read for {spec.label}; ignored"
                    )
                    continue
                if (
                    parked_policy_path is not None
                    and stat_signature(parked_policy_path.stat())
                    != parked_policy_signatures[spec.id]
                ):
                    raise OSError(
                        f"Canonical parked policy changed during paired read: "
                        f"{parked_policy_path}"
                    )
                loaded.append(normalize_source(
                    spec,
                    report,
                    semantic_report,
                    parked_policy,
                ))

            merged = merge_reports(loaded, warnings)
            merged['stamp'] = stamp
            merged['generatedAt'] = time.time()
            body = json.dumps(merged, separators=(',', ':')).encode('utf-8')
            self._cache_stamp = stamp
            self._cache_body = body
            return body


class ViewerHandler(BaseHTTPRequestHandler):
    server: ViewerServer

    def log_message(self, fmt: str, *args: object) -> None:
        if args and len(args) > 1 and str(args[1]).startswith(('4', '5')):
            super().log_message(fmt, *args)

    def send_bytes(self, data: bytes, content_type: str, status: HTTPStatus = HTTPStatus.OK) -> None:
        self.send_response(status)
        self.send_header('Content-Type', content_type)
        self.send_header('Content-Length', str(len(data)))
        self.send_header('Cache-Control', 'no-store')
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self) -> None:
        parsed = urlparse(self.path)
        path = parsed.path
        if path in ('/', '/index.html'):
            self.send_bytes(HTML.encode('utf-8'), 'text/html; charset=utf-8')
            return
        if path == '/api/report':
            try:
                force = parse_qs(parsed.query).get('force') == ['1']
                self.send_bytes(
                    self.server.build_envelope(force=force),
                    'application/json; charset=utf-8',
                )
            except (OSError, ValueError, json.JSONDecodeError) as exc:
                self.send_bytes(str(exc).encode('utf-8'), 'text/plain; charset=utf-8', HTTPStatus.INTERNAL_SERVER_ERROR)
            return
        self.send_bytes(b'Not found', 'text/plain; charset=utf-8', HTTPStatus.NOT_FOUND)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description='View and combine objdiff reports as a decomp.dev-style treemap.')
    parser.add_argument('report', nargs='?', default='build/report.json', help='Canonical objdiff report.json')
    parser.add_argument('--canonical-label', help='Label for the canonical report (default: current branch or canonical)')
    parser.add_argument('--repo', help='Canonical repository root used for git worktree discovery')
    parser.add_argument('--all-worktrees', action='store_true', help='Include build/report.json from every local Git worktree')
    parser.add_argument('--worktree-report', default='build/report.json', help='Report path inside each worktree')
    parser.add_argument('--source', action='append', default=[], metavar='LABEL=PATH', help='Include an additional live report')
    parser.add_argument('--parked', action='append', default=[], metavar='LABEL=PATH', help='Include a saved parked report')
    parser.add_argument('--parked-dir', action='append', default=[], metavar='DIR', help='Include every *.json under a parked-report directory')
    parser.add_argument('--host', default='127.0.0.1', help='Host to bind (default: 127.0.0.1)')
    parser.add_argument('--port', type=int, default=8765, help='Port to bind (default: 8765)')
    parser.add_argument('--no-browser', action='store_true', help='Do not open the browser automatically')
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    sources, warnings = discover_sources(args)
    canonical = next((s for s in sources if s.kind == 'canonical'), None)
    if canonical is None or not canonical.path.exists():
        print(f"Canonical report not found: {Path(args.report).expanduser().resolve()}")
        return 2

    print('Reports:')
    for source in sources:
        state = 'OK' if source.path.exists() else 'MISSING'
        print(f"  [{state:7}] {source.kind:9} {source.label}: {source.path}")
    for warning in warnings:
        print(f"Warning: {warning}")

    server = ViewerServer(
        (args.host, args.port),
        sources,
        warnings,
        source_discovery=lambda: discover_sources(args),
    )
    url = f'http://{args.host}:{args.port}/'
    print(f'Open: {url}')
    print('Press Ctrl+C to stop.')
    if not args.no_browser:
        threading.Timer(0.35, lambda: webbrowser.open(url)).start()
    try:
        server.serve_forever(poll_interval=0.25)
    except KeyboardInterrupt:
        print('\nStopped.')
    finally:
        server.server_close()
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
