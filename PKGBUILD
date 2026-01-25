# Maintainer: 
pkgname=tempmon
pkgver=1.0.0
pkgrel=1
pkgdesc="Lightweight system tray temperature and power monitor for Linux"
arch=('x86_64')
url="https://github.com/infinityabundance/TempMon"
license=('MIT')
depends=('gtk3' 'libappindicator-gtk3')
makedepends=('gcc' 'make' 'pkg-config')
source=("tempmon.cpp"
        "Makefile"
        "tempmon.desktop")
sha256sums=('SKIP'
            'SKIP'
            'SKIP')

build() {
    cd "$srcdir"
    make
}

package() {
    cd "$srcdir"
    install -Dm755 tempmon "$pkgdir/usr/bin/tempmon"
    install -Dm644 tempmon.desktop "$pkgdir/usr/share/applications/tempmon.desktop"
}
