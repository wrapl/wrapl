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
	gir1.2-rsvg-2.0 \
	gobject-introspection \
	lv2-dev \
	libgtksourceview-3.0-dev \
	liblmdb-dev \
	libkrb5-dev \
	libsoup2.4-dev \
	librsvg2-dev \
	libvips-dev \
	libpq-dev \
	libgoocanvas-2.0-dev \
	liblz4-dev \
	flex \
	bison \
	libreadline-dev \
	unzip \
	libcurl4-openssl-dev \
	libesmtp-dev \
	python-yaml \
	gettext \
	libtool \
	autopoint \
	flex \
	bison \
	libssh-dev \
	libpocketsphinx-dev \
	libsphinxbase-dev \
	texinfo \
	libmad0-dev \
	libmp3lame-dev \
	libid3tag0-dev \
	libmagic-dev \
	libopencore-amrnb-dev \
	libopencore-amrwb-dev \
	libc-dev

RUN apt-get clean

ADD . /tmp/wrapl

WORKDIR /tmp/wrapl

RUN rm -rf dev/src/rabs
RUN rm -rf dev/src/rlink/minilang
RUN git submodule update --init --recursive

RUN linux32 make -C dev/src/rabs install

WORKDIR /tmp/wrapl

RUN ls -lah

RUN linux32 rabs -p8 -c

RUN mv /tmp/wrapl/lib /usr/lib/riva
RUN mv /tmp/wrapl/bin/riva /usr/bin/riva
RUN mv /tmp/wrapl/dev/bin/rlink /usr/bin/rlink
RUN mv /tmp/wrapl/dev/bin/riva.conf.debian /usr/bin/riva.conf
RUN mv /tmp/wrapl/dev/bin/wrapl.debian /usr/bin/wrapl
RUN mv /tmp/wrapl/dev/bin/wrpp.debian /usr/bin/wrpp
RUN mv /tmp/wrapl/dev/inc/gcc /usr/include/riva-dev
RUN cp -R /tmp/wrapl/dev/gen/inc/gcc/* /usr/include/riva-dev/
RUN mv /tmp/wrapl/dev/lib /usr/lib/riva-dev
RUN cp -R /tmp/wrapl/dev/gen/lib/* /usr/lib/riva-dev/
RUN rm -rf /tmp/wrapl