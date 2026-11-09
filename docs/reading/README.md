# MCC support reading guide

MCC compatibility uses independent modules and the `mcc_maps` namespace.
Xbox and Custom Edition retain their existing loaders and resource roots.

## Scope and provenance

This documentation-only contribution targets OpenCE's `testing` branch. The
companion implementation is [opence-mcc-support](https://github.com/chestahh/GulchCE/tree/1131d624b5c4e1f6e4d12174c4929452acf65285);
these documents do not enable MCC support without that implementation.
They carry forward the MCC documentation from
[GET-YOUR-MCC-HERE at 1db929c5](https://github.com/chestahh/GulchCE/tree/1db929c5e34101a45f644436e031016b5584c7bf),
with the upstream setup, protocol and validation context clarified.

Implementation and test links absent from this documentation branch point to
exact source commits. The full archive contains more tests than the focused
subset submitted with `opence-mcc-support`; references to those tests describe
archived coverage, not files or CI jobs added by this documentation contribution.
Historical logs retain original branch names, protocols, test commands and
limitations. OpenCE's MCC contribution uses the shared protocol **42**.

The [validation record](../mcc_upstream_validation.md) separates the current
testing-based contribution's checks from the earlier extraction and refactor
runs. No gameplay validation is claimed for this documentation-only revision.

## Reading order

- [Loading, resources and host sharing](maps_and_resources.md)
- [Console syntax and checkpoint lifetime](console_checkpoints.md)
- [Map formats, installation and compatibility evidence](../mcc_maps.md)
- [Stock-menu package previews and completion](mcc_level_previews.md)
- [Browser imports, storage ownership and recovery](browser_imports.md)
- [Host-only package sharing and protocol agreement](../../port/linux/NETCODE.md#mcc-package-sharing)
- [Upstream contribution and historical validation](../mcc_upstream_validation.md)

The format reference separates inspected structures from inferred behavior.
The validation record distinguishes compilation, automated checks and bounded
runtime observations; it does not claim complete campaign playthroughs.

- [Third-person camera](third_person.md): optional shoulder view, controls, host policy, cover-aware firing and lifecycle.

- [Third-person camera](third_person.md): optional shoulder view, controls, host policy, cover-aware firing and lifecycle.
