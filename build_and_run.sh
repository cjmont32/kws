#!/usr/bin/bash

export KWS_MAKE_RELEASE_BUILD=1

make clean
make db
make all

mkdir -p bin/run/document_root
mkdir -p bin/run/db

cp -f bin/db/kws.db bin/run/db
cp -f bin/cgi/* bin/run/document_root
cp -rf static bin/run/document_root

DB=$PWD/bin/run/db/kws.db

pushd bin/run/document_root

KWS_DB_PATH=$DB npx easy-cgi

popd