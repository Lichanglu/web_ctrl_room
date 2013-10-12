#!/bin/sh
echo ""
echo "Self Extracting Installer"
echo ""
 
export TMPDIR=`mktemp -d /tmp/selfextract.XXXXXX`
ARCHIVE=`awk '/^__ARCHIVE_BELOW__/ {print NR + 1; exit 0; }' $0`
tail -n+$ARCHIVE $0 | tar xzv -C $TMPDIR
TAR_NAME=`ls $TMPDIR`
CDIR=`pwd`
cd $TMPDIR/$TAR_NAME/
if [ "$1" = "uninstall" ]; then
./uninstall
else
./install
fi
cd $CDIR
rm -rf $TMPDIR
exit 0
__ARCHIVE_BELOW__
