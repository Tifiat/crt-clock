# Maintainer: Tifiat
pkgname=crt-clock
pkgver=0.4.1.r1
pkgrel=1
pkgdesc="Lightweight Qt6 perspective clock and animated ASCII companion for XFCE/X11"
arch=('x86_64')
url='https://github.com/Tifiat/crt-clock'
license=('MIT')
depends=('qt6-base' 'libx11')
makedepends=('cmake' 'gcc' 'git')
source=("crt-clock::git+https://github.com/Tifiat/crt-clock.git#branch=main")
sha256sums=('SKIP')

pkgver() {
  cd "$srcdir/crt-clock"
  printf '0.4.1.r%s.g%s' "$(git rev-list --count HEAD)" "$(git rev-parse --short HEAD)"
}

build() {
  cmake -S "$srcdir/crt-clock" -B "$srcdir/build" \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_TESTING=OFF
  cmake --build "$srcdir/build" --parallel 2
}

package() {
  DESTDIR="$pkgdir" cmake --install "$srcdir/build"
  install -Dm644 "$srcdir/crt-clock/LICENSE" "$pkgdir/usr/share/licenses/crt-clock/LICENSE"
}
