#! /bin/bash

export PACKAGE_NAME="libaedile-dev"
export VERSION="0.0.2"

export SRC_ROOT_DIR=$(dirname $(readlink -f ${0}))
export DEB_PACKAGE_ROOT="${SRC_ROOT_DIR}/deb"
export INSTALL_DIR="${DEB_PACKAGE_ROOT}/opt/${PACKAGE_NAME}/usr"


rm -rf $INSTALL_DIR
mkdir -p $INSTALL_DIR

cd "${SRC_ROOT_DIR}"
cmake --preset=linux
cmake --build build/linux
cmake --install build/linux

rm -rf ${DEB_PACKAGE_ROOT}/usr/

mkdir -p ${DEB_PACKAGE_ROOT}/usr/include/aedile/client
mkdir -p ${DEB_PACKAGE_ROOT}/usr/lib

ln -s ${INSTALL_DIR}/include/nostr.h ${DEB_PACKAGE_ROOT}/usr/include/aedile/nostr.h
ln -s ${INSTALL_DIR}/include/nostr.hpp ${DEB_PACKAGE_ROOT}/usr/include/aedile/nostr.hpp
ln -s ${INSTALL_DIR}/include/client/web_socket_client.hpp ${DEB_PACKAGE_ROOT}/usr/include/aedile/client/web_socket_client.hpp
ln -s ${INSTALL_DIR}/lib/libaedile.a ${DEB_PACKAGE_ROOT}/usr/lib/libaedile.a

cd "${SRC_ROOT_DIR}"

dpkg-deb --build deb deb/$PACKAGE_NAME-${VERSION}.deb
