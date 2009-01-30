#!/bin/bash
VER=`grep PACKAGE_VERSION config.h | sed -e "s/#define PACKAGE_VERSION \"\(.*\)\"/\1/"`
PACK_NAME="raop_play-$VER"
PACK_FILE="$PACK_NAME.tar.gz"
PACK_DIR="/tmp/$PACK_NAME"
echo "Removing packaging dir $PACK_DIR"
rm -rf $PACK_DIR
rm $PACK_FILE
echo "Creating packaging dir $PACK_DIR"
mkdir $PACK_DIR
echo "Copying everything to packaging dir $PACK_DIR"
cp -r * $PACK_DIR
echo "Packaging file $PACK_FILE"
tar -cC /tmp -vzf $PACK_FILE --exclude CVS $PACK_NAME
