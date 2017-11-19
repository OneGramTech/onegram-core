FROM ubuntu:16.04
MAINTAINER Jozef Knaperek <jozef.knaperek@01cryptohouse.com>

ARG uid=1000
ARG gid=1000

RUN groupadd --gid $gid -r onegram && useradd --uid $uid --create-home --system -g onegram onegram

RUN apt-get update \
	&& apt-get install -y --no-install-recommends \
		gcc-5 \
		g++-5 \
		cmake \
		make \
		ninja-build \
		libbz2-dev \
		libdb++-dev \
		libdb-dev \
		libssl-dev \
		openssl \
		libreadline-dev \
		autoconf \
		libtool \
		doxygen \
		uuid-dev \
		zip \
		build-essential \
		python-dev \
		autotools-dev \
		libicu-dev \
		automake \
		ncurses-dev \
		nodejs \
		nodejs-legacy \
		npm \
		git \
		libboost1.58-dev \
		libboost-thread-dev \
		libboost-date-time-dev \
		libboost-system-dev \
		libboost-filesystem-dev \
		libboost-program-options-dev \
		libboost-signals-dev \
		libboost-serialization-dev \
		libboost-chrono-dev \
		libboost-test-dev \
		libboost-context-dev \
		libboost-locale-dev \
		libboost-iostreams-dev \
		libboost-coroutine-dev \
	&& apt-get clean
# Note: the build requires access to .git (repo metadata) - fails otherwise!

COPY . /src

# Use BUILD=Release for production builds
ARG BUILD=Debug
# Use TARGET=witness_node to only build witness_node
ARG TARGET=

RUN cd /src \
	&& cmake -G Ninja -DCMAKE_BUILD_TYPE=$BUILD . \
	&& ninja $TARGET \

	&& cp ./programs/${TARGET:-witness_node}/${TARGET:-witness_node} /usr/local/bin/ \
	&& cp -n ./programs/${TARGET:-cli_wallet}/${TARGET:-cli_wallet} /usr/local/bin/ \

	&& mkdir -p /var/onegram/data \
	&& chown onegram:onegram /var/onegram/data

# Use the following to embed genesis config into the binary:
# cmake -DGRAPHENE_EGENESIS_JSON="/home/onegram/onegram-genesis.json"

COPY onegram/onegram-genesis.json /home/onegram/
COPY onegram/onegram-config.ini /var/onegram/data/config.ini
COPY onegram/onegram_witness /usr/local/bin/

WORKDIR /home/onegram

EXPOSE 8540 11011

USER onegram

VOLUME /var/onegram/data

CMD /usr/local/bin/onegram_witness
