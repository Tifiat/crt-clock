# AUR package: crt-clock-git

This folder contains the **Git / rolling** Arch User Repository package for CRT Clock.

The package builds `main` from `https://github.com/Tifiat/crt-clock`, not
a fixed tagged release. Its name must therefore carry the `-git` suffix.
It provides/conflicts with the locally installed package `crt-clock` so
pacman can replace the old manually built package cleanly.

## Publish (maintainer only)

AUR publication is separate from GitHub. You need:

1. An AUR account at https://aur.archlinux.org/register/.
2. An SSH public key added to that account's profile.
3. `git`, `base-devel` (including `makepkg`), and this repository.

From a clone of **this** GitHub repository on Arch Linux:

```bash
bash packaging/aur/publish.sh
```

The script clones `ssh://aur@aur.archlinux.org/crt-clock-git.git`
into `~/aur/crt-clock-git` (unless `AUR_CHECKOUT` is set),
copies `PKGBUILD`, generates `.SRCINFO` using
`makepkg --printsrcinfo`, commits both, and pushes to the
AUR's required `master` branch.

It cannot bypass AUR authentication or claim an existing package belonging
to another maintainer. Do not commit private SSH keys or passwords.

## Install (after AUR publication)

```bash
yay -S crt-clock-git
```

AUR helpers may ask for confirmation to remove an older `crt-clock`
package because both install `/usr/bin/crt-clock`.
The user's `~/.config/crt-clock/config.ini` is untouched.

For development-package updates, use `yay -Syu --devel` or manually
rebuild; ordinary AUR version metadata does not track every upstream commit.

## Local verification

```bash
cd packaging/aur
bash -n PKGBUILD
makepkg --printsrcinfo
makepkg -si
```

The GitHub CI validates syntax and builds CRT Clock; AUR packaging still
requires a real Arch build and runtime verification on XFCE/X11.
