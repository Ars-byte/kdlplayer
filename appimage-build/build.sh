#!/bin/bash
rm -rf AppDir
rm -f kdlplayer.desktop



printf "[Desktop Entry]\nType=Application\nName=kdlplayer\nExec=kdlplayer\nIcon=kdlplayer\nCategories=AudioVideo;Player;\nTerminal=false\n" > kdlplayer.desktop

cat kdlplayer.desktop

./linuxdeploy-x86_64.AppImage \
    --appdir AppDir \
    --executable ./kdlplayer \
    --desktop-file kdlplayer.desktop \
    --icon-file icon/kdlplayer.png \
    --library libs/libraylib.so.550 \
    --library libs/libdiscord-rpc.so \
    --output appimage