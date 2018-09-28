FROM i386/debian:sid-slim

RUN apt-get update && apt-get install -y \
	libgssapi-krb5-2 \
	libgtk-3-0 \
	libgtksourceview-3.0-1 \
	libncursesw6 \
	libjansson4 \
	libexpat1 \
	libpq5 \
	libssl1.1 \
	libgcrypt20 \
	libesmtp6

RUN apt-get clean

ADD usr /usr

LABEL maintainer="rajamukherji@gmail.com"

CMD ["/bin/bash"]
