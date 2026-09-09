# Claude-lane reconciliation 10K batch (2026-09-09)

## Published-base comparison

The frozen published base is
`105d9acaacda589e21a4389d7e6bd69db0b42aba`, with 6,638 accepted exact
functions and 1,048,212 meaningful exact code bytes. The final rebuilt
semantic ledger for this packet contains 6,686 accepted exact functions and
1,058,829 meaningful exact code bytes:

- **48 net accepted exact functions**;
- **10,617 net meaningful exact code bytes**;
- **zero lost accepted exact functions**;
- **zero unit errors**.

This is a canonical before/after result. Donor-lane headline totals are not
added to it.

| reconciled packet | accepted functions | meaningful exact bytes |
| --- | ---: | ---: |
| `hs_compile.obj` inspect parser | 1 | 191 |
| `sound_manager.obj` render/prioritization graph | 12 | 4,049 |
| `hud_draw.obj` bitmap retrieval pair | 2 | 753 |
| `ai_script.obj` coherent scripting graph | 33 | 5,624 |
| **published semantic-ledger total** | **48** | **10,617** |

The generated target was re-split after authenticating the private names
`get_sprite_clip_rect` and `ai_scripting_assess_status`. Both now compare
directly under their semantic names. No semantic waiver, alias-only published
credit, or byte exception was added.

## Reconciliation decisions

- The AI packet's first owner-header layout changed three unrelated exact
  functions through VC7 declaration context. Moving `ai_erase` to the existing
  narrow `ai_runtime.h` owner restored all three. The final global sweep has no
  losses.
- The saved-game lane's four exact-looking candidates were rejected. They
  expose uninitialized output or serialized padding, close a file through
  `NONE`, or leak an opened handle. The 1,090 meaningful-byte claim receives
  no credit and none of its source is integrated.
- Five AI Script bodies, four UI profile bodies, and Sound's
  `_refresh_listener` are retained as credible best-known fuzzy source. Their
  combined 5,881 target meaningful bytes receive **zero exact credit**.
- The UI fuzzy packet changes no previously exact function. The existing
  campaign's explicit fuzzy debits remain fully visible through the park
  ledger.
- The inherited Units `_point_from_line3d` surplus-COMDAT veto is unchanged;
  this batch neither claims Units object admission nor credits that helper.

## House-rule and provenance review

Typed tag/object access, project scalar types, named enums and flags, true
owner headers, explicit terminal returns, and multiline parameters are used.
Private functions have semantic names and file-local linkage. The accepted
source contains no fake dependencies, volatile/register steering, raw byte
emission, unrelated prototypes, guessed address names, or nonsensical logic.
The only changed-file fake-scan leads are the pre-existing, independently
justified naked `get_return_eip` helper in `hud_draw.c`; the new HUD functions
do not add assembly.

The supplied HCEA/PDB material and Claude donors were used as reconstruction
maps. January target code, relocations, strings, ABI and caller topology remain
the acceptance authority. Each packet has a dedicated object ledger with
function-level measurements and rejected paths.

## Publication gate

- Full `ninja all_source`: passed.
- Hardened semantic audit: 473 units, 7,172 functions evaluated, 6,686
  accepted exact, zero unit errors.
- Frozen accepted-ledger comparison: 48 gains / zero losses / +10,617
  meaningful bytes.
- Park validation: 347 active, zero stale, zero invalid.
- Object-admission audit: zero candidates and zero contradictions; six
  inherited explicit whole-object rejections remain visible.
- Test suite: 1,144 passed, 5 environment-dependent skipped, and 26 subtests
  passed.
- Changed-source fake-match scan: no new leads; two inherited HUD assembly
  leads remain documented.
- `git diff --check`: passed.

The canonical treemap is refreshed at this 10K boundary. Its next required
refresh threshold is **1,068,829 meaningful accepted exact bytes**.
