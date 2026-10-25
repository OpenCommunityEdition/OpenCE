# nixpkgs with rust-overlay, for every cell
{ inputs, cell }:
inputs.nixpkgs.appendOverlays [ inputs.rust-overlay.overlays.default ]
