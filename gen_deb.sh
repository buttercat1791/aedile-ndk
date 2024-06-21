#! /bin/bash

export PACKAGE_NAME="libaedile-dev"
export VERSION="0.0.2"

export SRC_ROOT_DIR=$(dirname $(readlink -f ${0}))
export PACK_DIR="${SRC_ROOT_DIR}/deb/opt/${PACKAGE_NAME}/usr"
echo $PACK_DIR


rm -rf $PACK_DIR
mkdir -p $PACK_DIR

cd "${SRC_ROOT_DIR}"
cmake --preset=linux
cmake --build build/linux
cmake --install build/linux

cd "${SRC_ROOT_DIR}"

dpkg-deb --build deb deb/$PACKAGE_NAME-${VERSION}.deb
