# Lane reconciliation 10K batch 3 (2026-09-09)

## Published-base result

The frozen published base is
`6f8644c0cc7b7748f98b9a4624e89678ff5263c4`, with 6,721 accepted exact
functions and 1,069,562 meaningful exact code bytes. The rebuilt semantic
ledger for this batch contains 6,748 accepted exact functions and 1,080,557
meaningful exact code bytes:

- **27 net accepted exact functions**;
- **10,995 net meaningful exact code bytes**;
- **11,178 net padded exact bytes**;
- **zero lost accepted exact functions**;
- **zero unit errors**.

These are whole-canonical accepted-ledger deltas. Fuzzy functions, helper
copies, and donor-lane headline totals receive no credit.

| reconciled packet | accepted functions | meaningful exact bytes |
| --- | ---: | ---: |
| Decals deletion owner | 1 | 401 |
| Weapons export and trigger owners | 2 | 1,037 |
| Devices debug owner | 1 | 426 |
| Lens-flare corona rotation | 1 | 584 |
| Transparent-geometry SDK owners | 8 | 1,161 |
| Environment diffuse draw and SDK owners | 4 | 1,801 |
| AI visibility, update, and spatial-effect owners | 5 | 2,090 |
| Model-animation frame and aiming owners | 2 | 1,676 |
| Actor-combat collateral-damage owner | 1 | 872 |
| ActionFlee position owners | 2 | 947 |
| **published semantic-ledger gain** | **27** | **10,995** |

## House-rule reconciliation

The accepted source uses semantic private/global names, project types, named
enums and flag macros, typed object/tag accessors, multiline parameters, and
explicit terminal returns. `ai_profile` now has one complete typed declaration
in `ai_profile.h`; ten Halo Script globals point to its named fields, and AI
meter writers use named meter indices instead of incompatible local structure
views and raw offsets. That ownership migration is correctness work and earns
no extra byte credit.

ActionFlee's three coherent callers remain fuzzy and are parked at zero
credit. Two genuine private position owners are exact. The full build caught a
declaration-position regression that the translation-unit gate could not see:
putting three ACTOR_PERCEPTION.C prototypes in the middle of `actors.h`
reopened `unit_preprocess_node_orientations`. The final packet moves those
prototypes to `actor_perception.h` and isolates two new PATH.C declarations at
the end of `path.h`. Units remains 189/189, while the closer pre-existing
`encounter_update_respawn` fuzzy schedule is also preserved.

The retained safe `ai_test_line_of_sight` initializes its collision fraction
before the fog path and therefore does not reproduce an undefined read merely
to gain similarity. It and the other documented AI/model/lens-flare residuals
remain in `config/parked.json` with current hashes and zero exact credit.

## Inline and fake-match boundaries

ActionFlee emits no `_point_from_line3d`. The inherited Units and AI Debug
candidate-only helper debt is unchanged and uncredited; this batch does not
claim fresh whole-object admission for either object. The two changed-source
fake-scan leads are the paired, function-scoped optimization pragmas around
`ai_handle_spatial_effect`. January's unique `/Od`-shaped body and the exact
1,130-byte/47-relocation result justify that narrow setting; no other function
is inside the region.

No volatile/register steering, fake dependency, raw byte emission,
address-derived name, forced inline, representation pun, or nonsensical
exactness carrier is admitted.

## Publication gate

- `ninja all_source semantic_progress`: passed.
- Hardened semantic audit: 473 units, 7,246 functions evaluated, 6,748
  accepted exact, zero unit errors.
- Frozen stable-verdict comparison: 27 gains, zero losses, +11,178 padded
  bytes.
- Accepted-ledger comparison: +10,995 meaningful exact bytes.
- Park validation: 360 active, zero stale, zero invalid.
- Object-admission audit: zero candidates and zero contradictions; six
  inherited explicit rejections remain visible.
- Test suite: 1,144 passed, 5 environment-dependent skipped, and 26 subtests
  passed.
- Changed-source fake-match scan: two reviewed paired pragma leads, no
  unreviewed findings.
- `git diff --check`: passed.

The canonical treemap is refreshed at this 10K boundary. Its next required
refresh threshold is **1,090,557 meaningful accepted exact bytes**.
