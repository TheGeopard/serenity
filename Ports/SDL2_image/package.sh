#!/usr/bin/env -S bash ../.port_include.sh
port=SDL2_image
useconfigure=true
version=2.0.5
depends="SDL2 libpng libjpeg libtiff"
files="https://www.libsdl.org/projects/SDL_image/release/SDL2_image-${version}.tar.gz SDL_image-${version}.tar.gz f26f3a153360a8f09ed5220ef7b07aea"
auth_type=md5

configure() {
    run ./configure \
        --host="${SERENITY_ARCH}-pc-serenity" \
        --with-sdl-prefix="${SERENITY_INSTALL_ROOT}/usr/local" \
        --enable-webp=false --enable-webp-shared=false     \
        LDFLAGS="-lgui -lgfx -lipc -lcore -lm"
}

build() {
    run make -k
}

install() {
    run make -k DESTDIR="${SERENITY_INSTALL_ROOT}" install
    ${CC} -shared -o ${SERENITY_INSTALL_ROOT}/usr/local/lib/libSDL2_image.so -Wl,-soname,libSDL2_image.so -Wl,--whole-archive ${SERENITY_INSTALL_ROOT}/usr/local/lib/libSDL2_image.a -Wl,--no-whole-archive -lpng -ljpeg -ltiff
    rm -f ${SERENITY_INSTALL_ROOT}/usr/local/lib/libSDL2_image.la
}
