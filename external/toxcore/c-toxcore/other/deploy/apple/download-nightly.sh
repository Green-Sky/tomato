#!/bin/bash
# Download the nightly builds of Tox for iOS, iPhone simulator, and macOS.

set -eux -o pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

cd "$SCRIPT_DIR"

if [ ! -d "toxcore-ios-arm64" ]; then
  tar -zxf \
    <(curl -L "https://github.com/TokTok/c-toxcore/releases/download/nightly/toxcore-nightly-ios-arm64.tar.gz")
fi

for arch in arm64 x86_64; do
  if [ ! -d "toxcore-iphonesimulator-$arch" ]; then
    tar -zxf \
      <(curl -L "https://github.com/TokTok/c-toxcore/releases/download/nightly/toxcore-nightly-iphonesimulator-$arch.tar.gz")
  fi
done

for arch in arm64 x86_64; do
  if [ ! -d "toxcore-macos-$arch" ]; then
    tar -zxf \
      <(curl -L "https://github.com/TokTok/c-toxcore/releases/download/nightly/toxcore-nightly-macos-$arch.tar.gz")
  fi
done
