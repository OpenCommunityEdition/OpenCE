# `nix fmt`
{ inputs, cell }:
{
  default = cell.pkgs.nixfmt;
}
