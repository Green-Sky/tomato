FROM toxchat/c-toxcore:sources AS sources
FROM alpine:3.19.0

RUN ["apk", "add", "--no-cache", \
 "bash", \
 "benchmark-dev", \
 "clang", \
 "gtest-dev", \
 "libconfig-dev", \
 "libsodium-dev", \
 "libvpx-dev", \
 "linux-headers", \
 "opus-dev", \
 "pkgconfig", \
 "python3"]

WORKDIR /work
COPY --from=sources /src/ /work/

COPY toxcore/BUILD.bazel /work/toxcore/
COPY other/docker/modules/check /work/other/docker/modules/
RUN ["other/docker/modules/check"]
