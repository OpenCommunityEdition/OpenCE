# action_uncover.obj — opus5 150K lane, wave w2 (2026-09-14)

## Baseline and final

| gate | result |
| --- | --- |
| baseline real file | `== exact 8 residual 1 unwritten 0`, guard passes |
| final real file (unchanged) | `== exact 8 residual 1 unwritten 0`, guard passes, park drift 0, fake scan 0 leads |

`git diff --stat -- source/ai/action_uncover.c` is empty. No credit is claimed.

## `_action_uncover_perform` (parked, instruction-scheduling): SKIPPED-EXHAUSTED

Target 544 / 19 / `ddd3ed044dc0de47`; candidate 544 / 19 / `f6c5eb3588f3efcc`.

The residual was re-disassembled. The only differing window is +0xa2..+0xd3:

- January: `lea edx,[esi+0xb0]`, then loads x/y/z into eax/ecx/edx, then stores x, (surface load), y, (cluster load), enable=1, z, surface, cluster.
- VC7: alternates each point load with its store.

No lever from brief section 9 applies:

- no call inside the window (store/call order);
- equal frame;
- no x87;
- no inline helper;
- one return;
- no accessor-binding difference (same compact lea).

The do-not-repeat lists already cover the remaining candidates: E01-E20, the 25 probe variants of 2026-08-28, and 45 defined-C families. No shape was compiled.

Reopen criterion (unchanged): authentic January source or local records, or a same-compiler donor for the load-all/store-all copy schedule.
