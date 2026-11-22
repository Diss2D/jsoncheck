#!/bin/sh
set -eu

cd "$(dirname "$0")"

NAME=jsoncheck
VERSION=$(cat VERSION)
ARCH=$(dpkg --print-architecture)

ROOT="build/pkg/${NAME}_${VERSION}_${ARCH}"

# Флаги защиты, предоставляемые Ubuntu
export CFLAGS="$(dpkg-buildflags --get CFLAGS)"
export CPPFLAGS="$(dpkg-buildflags --get CPPFLAGS)"
export LDFLAGS="$(dpkg-buildflags --get LDFLAGS)"

# Чистая выпускная сборка
rm -rf build/release "$ROOT"

cmake -S . -B build/release \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr

cmake --build build/release

#Установка файлов во временный каталог пакета
DESTDIR="$PWD/$ROOT" cmake --install build/release --strip

# Определение зависимостей исполняемого файла
ROOT_ABS="$PWD/$ROOT"

mkdir -p build/package-meta/debian
: > build/package-meta/debian/control

DEPS_OUTPUT=$(
    cd build/package-meta
    dpkg-shlibdeps -O -e"$ROOT_ABS/usr/bin/$NAME"
)
case "$DEPS_OUTPUT" in
    shlibs:Depends=*)
        DEPENDS=${DEPS_OUTPUT#shlibs:Depends=}
        ;;
    *)
        echo "Failed to determine package dependencies" >&2
        exit 1
        ;;
esac

if [ -z "$DEPENDS" ]; then
    echo "Package dependencies are empty" >&2
    exit 1
fi

printf '\nRuntime dependencies: %s\n' "$DEPENDS"

# Подготовка управляющих файлов deb-пакета
mkdir -p "$ROOT/DEBIAN" dist

# Подстановка версии, архитектуры и зависимостей
sed \
    -e "s|@VERSION@|$VERSION|g" \
    -e "s|@ARCH@|$ARCH|g" \
    -e "s|@DEPENDS@|$DEPENDS|g" \
    packaging/DEBIAN/control > "$ROOT/DEBIAN/control"

# Регистрация конфигурационного файла
install -m 0644 \
    packaging/DEBIAN/conffiles \
    "$ROOT/DEBIAN/conffiles"

# Сборка установочного пакета
dpkg-deb --root-owner-group --build "$ROOT" dist/

printf '\nPackage created successfully.\n'

# Проверка подготовленных файлов
printf '\nFiles staged for packaging:\n'
find "$ROOT" -type f | sort
