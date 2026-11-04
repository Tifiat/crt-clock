#!/usr/bin/env bash
# Publish or update the AUR crt-clock-git package as the logged-in AUR maintainer.
# Requires an AUR account and an SSH public key configured in its profile.
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
DEST="${AUR_CHECKOUT:-$HOME/aur/crt-clock-git}"
REMOTE='ssh://aur@aur.archlinux.org/crt-clock-git.git'

command -v makepkg >/dev/null || { echo 'makepkg is required (run on Arch Linux).' >&2; exit 1; }
command -v git >/dev/null || { echo 'git is required.' >&2; exit 1; }
if [[ -e "$DEST" && ! -d "$DEST/.git" ]]; then
    echo "Destination exists but is not a Git checkout: $DEST" >&2
    exit 1
fi
if [[ ! -d "$DEST/.git" ]]; then
    mkdir -p "$(dirname -- "$DEST")"
    git clone "$REMOTE" "$DEST"
fi

cd "$DEST"
if [[ "$(git remote get-url origin)" != "$REMOTE" ]]; then
    echo 'Refusing to publish: checkout origin is not the expected AUR repository.' >&2
    exit 1
fi
cp -- "$ROOT/packaging/aur/PKGBUILD" PKGBUILD
makepkg --printsrcinfo > .SRCINFO

git add -- PKGBUILD .SRCINFO
if git diff --cached --quiet; then
    echo 'AUR PKGBUILD and .SRCINFO are already up to date.'
    exit 0
fi

git commit -m 'Package CRT Clock Git version for Arch Linux'
git push origin HEAD:master
echo 'Published (or updated) crt-clock-git in AUR.'
