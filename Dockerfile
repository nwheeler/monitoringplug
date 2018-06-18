FROM debian:jessie

RUN /bin/bash -c 'apt-get update && apt-get dist-upgrade -y -q && \
                  apt-get install -y -q \
                  autoconf \
                  devscripts \
                  bison \
                  flex \
                  libtool \
                  pkg-config \
                  check \
                  libcups2-dev \
                  libexpat1-dev \
                  libldns-dev \
                  libfcgi-dev \
                  libgnutls28-dev \
                  libopenipmi-dev \
                  libjson0-dev \
                  libldap2-dev \
                  libvirt-dev \
                  libmysqld-dev \
                  liboping-dev \
                  libpq-dev \
                  libhiredis-dev \
                  libselinux1-dev \
                  libsmbclient-dev \
                  libsnmp-dev \
                  libvarnishapi-dev \
                  libxmlrpc-core-c3-dev \
                  docbook-xml \
                  docbook-xsl \
                  libcurl4-gnutls-dev \
                  ldnsutils \
                  libncurses5-dev \
                  libtinfo-dev \
                  libxslt1.1 \
                  xsltproc \
                  libtinfo-dev \
                  libxslt1.1 \
                  libjemalloc1 \
                  libedit-dev'

# install varnish3 for check_varnish (which is probably rather deprecated at this point)
RUN /bin/bash -c 'mkdir /varnish && cd /varnish && \
                  curl https://varnish-cache.org/_downloads/varnish-3.0.7.tgz | tar xz && \
                  cd varnish-3.0.7 && \
                  ./autogen.sh && \
                  ./configure && \
                  make && \
                  make install'

ENV WORKSPACE=/workspace/
RUN mkdir -p $WORKSPACE
WORKDIR $WORKSPACE

COPY . $WORKSPACE
