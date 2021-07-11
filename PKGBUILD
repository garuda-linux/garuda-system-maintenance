# Maintainer: TNE <tne@garudalinux.org>

pkgname=garuda-system-maintenance
pkgver=1.1.0
pkgrel=2
pkgdesc="Automated Garuda system maintenance tool"
arch=('x86_64')
url="https://gitlab.com/garuda-linux/applications/$pkgname"
license=('GPL')
depends=('qt5-base' 'garuda-hotfixes')
makedepends=('qt5-tools' 'cmake' 'git' 'knotifications' 'polkit')
source=("$pkgname-$pkgver.tar.gz::$url/-/archive/$pkgver/$pkgname-$pkgver.tar.gz")
md5sums=('SKIP')

build() {
    cmake -B build -S "$pkgname-$pkgver" \
        -DCMAKE_BUILD_TYPE='Release' \
        -DCMAKE_INSTALL_PREFIX='/usr' \
        -Wno-dev
    make -C build
}

package() {
    make -C build DESTDIR="$pkgdir" install
    
    cd "$pkgname-$pkgver"
    install -Dm0755 update-packages "$pkgdir/usr/lib/$pkgname/update-packages"
    install -Dm0644 $pkgname.rules "$pkgdir/usr/share/polkit-1/rules.d/$pkgname.rules"
    install -Dm0644 $pkgname@.service "$pkgdir/etc/systemd/system/$pkgname@.service"
    install -Dm0644 $pkgname.notifyrc "$pkgdir/usr/share/knotifications5/$pkgname.notifyrc"
    install -Dm0644 $pkgname.desktop "$pkgdir/etc/xdg/autostart/$pkgname.desktop"
    install -Dm0644 $pkgname-settings.desktop "$pkgdir/usr/share/applications/$pkgname-settings.desktop"
    install -Dm0644 $pkgname.svg "$pkgdir/usr/share/icons/hicolor/scalable/apps/$pkgname.svg"

    # Fix permissions
    chmod -R 750 $pkgdir/usr/share/polkit-1/rules.d/
    chown -R root:polkitd $pkgdir/usr/share/polkit-1/rules.d/
}
