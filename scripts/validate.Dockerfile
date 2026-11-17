FROM ubuntu:24.04
RUN apt-get update -qq && DEBIAN_FRONTEND=noninteractive apt-get install -y -qq build-essential gcc-14 clang clang-format libtcc-dev python3 git >/dev/null && rm -rf /var/lib/apt/lists/*
# tcc from source (branch mob), as the CI's Linux TCC job does: Ubuntu's package is older.
RUN git clone -q -b mob https://github.com/TinyCC/tinycc.git /tmp/tinycc && cd /tmp/tinycc && ./configure >/dev/null && make -s >/dev/null && make -s install >/dev/null && rm -rf /tmp/tinycc
WORKDIR /work
