# AUR packaging

`PKGBUILD` and `tokideli.install` here are the canonical, version-controlled
source for tokideli's [AUR](https://aur.archlinux.org/) package. This
directory is not the AUR repository itself — AUR is a separate git hosting
service (`aur.archlinux.org`), unrelated to GitHub, and only someone with
their own AUR account and SSH key can publish to it.

## Initial publication (one-time, requires an AUR account)

1. Create an AUR account and register an SSH public key at
   <https://aur.archlinux.org/>.
2. `git clone ssh://aur@aur.archlinux.org/tokideli.git` (this will be an
   empty repository).
3. Copy `PKGBUILD` and `tokideli.install` from here into that clone.
4. Generate the required metadata file: `makepkg --printsrcinfo > .SRCINFO`.
5. `git add PKGBUILD tokideli.install .SRCINFO && git commit && git push`.

## Updating for a new tokideli release

1. In this `PKGBUILD`, bump `pkgver` to the new version and reset `pkgrel=1`.
2. Update `sha256sums` to match the new release tarball:
   ```sh
   curl -sL -o /tmp/tokideli-X.Y.Z.tar.gz \
     https://github.com/ShellShoccar-jpn/tokideli/archive/refs/tags/vX.Y.Z.tar.gz
   sha256sum /tmp/tokideli-X.Y.Z.tar.gz
   ```
3. Verify locally with `makepkg -s` (e.g. in an `archlinux` container) before
   publishing.
4. Copy the updated `PKGBUILD` into your AUR clone, regenerate `.SRCINFO`,
   commit, and push.

(If only the packaging changes, not the upstream version — e.g. fixing this
PKGBUILD itself — bump `pkgrel` instead of `pkgver`.)

## Why "sleep" becomes "tdsleep"

tokideli's own `sleep` command shares its name with the `sleep` that ships as
part of `coreutils`, which is installed on every Arch system by default.
`pacman` refuses to install a package that would overwrite a file already
owned by another package, so this PKGBUILD installs tokideli's `sleep`
binary as `/usr/bin/tdsleep` instead. Every other tokideli command keeps its
upstream name, since none of them collide with anything in a base install.
The same rename is applied in the Homebrew Formula
(`ShellShoccar-jpn/homebrew-tap`), for consistency.
