FROM i386/debian:sid

RUN apt-get update && apt-get install -y \
	libgssapi-krb5-2 \
	libgtk-3-0 \
	libgtksourceview-3.0-1 \
	libfluidsynth1 \
	libncursesw6 \
	libjack0 \
	libjansson4 \
	libsndfile1 \
	libexpat1 \
	libpq5 \
	libssl1.1 \
	libgcrypt20 \
	libesmtp6 \
	mda-lv2 \
	swh-lv2

RUN apt-get clean

ADD usr /usr

CMD /bin/bash


