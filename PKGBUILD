# Maintainer: rbm78bln <rbm78bln(at)github(dot)com>
# Contributor: rbm78bln <rbm78bln(at)github(dot)com>

_pkgbase=cw2217b
pkgname=kmod_cw2217b-dkms
pkgver=$(grep MODULE_VERSION ${_pkgbase}.c | cut '-d"' -f2)
pkgrel=0
pkgdesc="Linux kernel power_supply subsystem driver for the Cellwise CW2217B I²C fuel gauge exposing AC and battery class devices (DKMS)"
arch=('aarch64' 'x86_64')
url="https://github.com/rbm78bln/kmod_cw2217b"
license=('GPL2')
depends=(
	'binutils'
	'coreutils'
	'curl'
	'dkms'
	'dtc'
	'gawk'
	'gcc'
	'git'
	'grep'
	'kmod'
	'libarchive'
	'linux-headers'
	'make'
	'sed'
)
install=${_pkgbase}.install
source=(
	'.gitignore'
	'LICENSE'
	'Makefile'
	'PKGBUILD'
	'README.md'
	'cw2217b.c'
	'cw2217b.dts'
	'cw2217b.install'
	'dkms_conf.template'
)
sha256sums=(
	'SKIP'
	'SKIP'
	'SKIP'
	'SKIP'
	'SKIP'
	'SKIP'
	'SKIP'
	'SKIP'
	'SKIP'
)

pkgver () {
	cd "${srcdir}/${pkgname}"
	grep MODULE_VERSION ${_pkgbase}.c | cut '-d"' -f2
}

prepare() {
	cd "${srcdir}"
	mkdir "${pkgname}"
	for FILE in "${source[@]}"; do
	   cat "${FILE}" >"${pkgname}/${FILE}"
	done
}

build() {
	cd "${srcdir}/${pkgname}"
	make dkms.conf
}

package() {
	mkdir -p "${pkgdir}/usr/src/${pkgname}"
	[ -d "${srcdir}/../.git" ] && cp --archive "${srcdir}/../.git" "${pkgdir}"/usr/src/${pkgname}/.git
	for FILE in "${source[@]}"; do
	   install -Dm644 ${srcdir}/${pkgname}/${FILE} "${pkgdir}"/usr/src/${pkgname}/${FILE}
	done
	install -Dm644 ${srcdir}/${pkgname}/dkms.conf "${pkgdir}"/usr/src/${pkgname}/dkms.conf
}

