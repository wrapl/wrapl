FROM i386/debian:sid-slim AS build

RUN apt-get update && apt-get install -y \
	build-essential \
	git \
	libsqlite3-dev \
	libgc-dev \
	wget \
	automake \
	autoconf \
	libgtk-3-dev \
	libboost-dev \
	nasm \
	python \
	binutils-dev \
	libc6-dev \
	nettle-dev \
	lua5.1 \
	gperf \
	libncurses-dev \
	cmake \
	libssl-dev \
	libjansson-dev \
	libgcc-8-dev \
	gir1.2-freedesktop \
	gir1.2-gtk-3.0 \
	gir1.2-gtksource-3.0 \
	gobject-introspection \
	lv2-dev \
	libgtksourceview-3.0-dev \
	liblmdb-dev \
	libkrb5-dev \
	libsoup2.4-dev \
	libpq-dev \
	libgoocanvas-2.0-dev \
	liblz4-dev \
	flex \
	bison \
	libreadline-dev

RUN apt-get clean

ADD . /tmp/wrapl

WORKDIR /tmp/wrapl

RUN rm -rf dev/src/rabs
RUN rm -rf dev/src/rlink/minilang
RUN git submodule update --init --recursive

RUN linux32 make -C dev/src/rabs install

WORKDIR /tmp/wrapl

RUN ls -lah

RUN linux32 rabs -c

FROM debian:sid-slim

RUN dpkg --add-architecture i386

RUN apt-get update && apt-get install -y \
	libgssapi-krb5-2:i386 \
	libgtk-3-0:i386 \
	libgtksourceview-3.0-1:i386 \
	libncursesw6:i386 \
	libjansson4:i386 \
	libexpat1:i386 \
	libpq5:i386 \
	libssl1.1:i386 \
	libgcrypt20:i386 \
	libesmtp6:i386

RUN apt-get clean

COPY --from=build /tmp/wrapl/lib /usr/lib/riva
COPY --from=build /tmp/wrapl/bin/riva /usr/bin/riva
COPY --from=build /tmp/wrapl/dev/bin/riva.conf.debian /usr/bin/riva.conf
COPY --from=build /tmp/wrapl/dev/bin/wrapl.debian /usr/bin/wrapl
COPY --from=build /tmp/wrapl/dev/bin/wrpp.debian /usr/bin/wrpp