# `player_queues_new.obj` Jonas public delete pair

## Result

This bounded wave starts from verified commit
`c8c40b2ceaaee631f11abbed03447257c83f9a36` and freezes exactly four public
HCEA bodies against the January PC object. The sole code-producing candidate
object made both delete bodies strict independently. The other two bodies had
residuals and were removed immediately without tuning.

| Retained function | January RVA | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | ---: | --- |
| `_update_client_delete` | `0xA8170` | 56 | 64 | 6 | `8188e5f4efd424d68ffb4b138d53d613c77695658d96f829592a93142eb08d32` |
| `_update_server_delete` | `0xA84C0` | 87 | 96 | 11 | `0e6719b86d942d9fcb7cb66b447f3249bf17d261ee60999c76464afd51a0704c` |

The strict gain is two functions, 143 meaningful bytes, 160 padded bytes,
and 17 exact relocations. Together with the prior public septet, the object
advances from 7/23 and 353/3,404 meaningful bytes to 9/23 and 496/3,404.

## Evidence and ABI audit

- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split object SHA-256:
  `80cbbe975ff6661a2453e9affd906575a08cdea1788ddc6d9cf2b9ae80205bda`.
- Authenticated HCEA donor commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`.
- Read-only comparison repositories were pinned at Stian
  `fc11d861865735877d5b8b4c408b1a0c06dde3cf` and Pastudan
  `918af885935ec470a31256ecce9a977b12b01f80`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1`, using
  the unchanged XDK 3911 command and flags.

January COFF and the retained prefix layout place the server prefix at `+0x0`,
then preserve the baseline `0x4104`-byte unknown span so the client prefix begins
at `+0x4110`. Queues remain at server `+0x8` and client `+0x90`. The retained
delete bodies access only these authenticated prefix fields. Candidate-only
expanded `updates` arrays and their four-byte aggregate gap were removed with
the rejected new bodies; they create no retained layout claim or storage
ownership.

The retained delete bodies reproduce exact grouped-BSS addends, call sites,
padding, and relocation order. `_update_client_delete` has six relocations;
`_update_server_delete` has eleven. Both are ordinary typed C with explicit
`void`, terminal returns, and no assembly, volatile/register scheduling,
pragma, intrinsic, barrier, undefined behavior, raw address, pun, private
ABI, or object-byte forcing.

## Pruned first-shot residuals

- `_update_client_new`: target 144 padded bytes and 16 relocations; candidate
  was also 144/16 but had normalized hash
  `7f2e9dfa3403ec5062dfcf2351fb1f08981fb1b13f875b9e25e53adc563873d1`
  versus target
  `c731eb28b35a8984100ec676f6fc1e4c2fee9f24ed90445407966a70138f9c49`.
- `_update_server_new`: target 128 padded bytes and 15 relocations; candidate
  was 144/15 with normalized hash
  `1a45080c9c45dbc78691f5a38ce33139b7f13e5e7fc92a2f229325ec2bb6c782`
  versus target
  `3fe8955eb36b046709e1ae9cda8fb4ebb957f1abcc7729f2e92cfd40df20ae29`.

There was one pre-emission candidate-only layout assertion correction after
expanding the server aggregate. It emitted no object and exposed no code bytes.
The next compile was the sole code-producing candidate. All expanded layout
support was then pruned with the rejected new bodies, restoring the baseline
unknown span.

## Scope

Only `source/game/player_queues_new.c` and this ledger change. No Units,
Vehicles, Claude-active source, configuration, semantic exception, parked
record, tooling file, header, storage definition, or tracked deletion is in
scope. The branch is intentionally uncommitted and unpushed.
