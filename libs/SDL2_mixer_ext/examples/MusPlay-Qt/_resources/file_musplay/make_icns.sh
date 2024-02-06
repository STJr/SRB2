#!/bin/bash

TYPE="${PWD##*/}"
if [[ "$TYPE" == "" ]]; then
echo "Error: empty name!"
exit 1
fi

mkdir $TYPE".iconset"
cp $TYPE"_16x16.png" $TYPE".iconset/icon_16x16.png"
sips -Z 32 $TYPE".iconset/icon_16x16.png" --out $TYPE".iconset/icon_16x16@2x.png"
cp $TYPE"_32x32.png" $TYPE".iconset/icon_32x32.png"
sips -Z 64 $TYPE".iconset/icon_32x32.png" --out $TYPE".iconset/icon_32x32@2x.png"
cp $TYPE"_48x48.png" $TYPE".iconset/icon_48x48.png"
sips -Z 96 $TYPE".iconset/icon_48x48.png" --out $TYPE".iconset/icon_48x48@2x.png"
cp $TYPE"_256x256.png" $TYPE".iconset/icon_256x256.png"
cp $TYPE"_256x256.png" $TYPE".iconset/icon_128x128@2x.png"
sips -Z 512 $TYPE".iconset/icon_256x256.png" --out $TYPE".iconset/icon_256x256@2x.png"
#sips -Z 512 $TYPE".iconset/icon_256x256.png" --out $TYPE".iconset/icon_512x512.png"
sips -Z 128 $TYPE".iconset/icon_256x256.png" --out $TYPE".iconset/icon_128x128.png"

echo "makeIcon..."

iconutil -c icns "$TYPE.iconset"
mv $TYPE".icns" ../

rm -Rf $TYPE".iconset"
