#!/bin/sh

find "$HOME/.trinity/share/apps/remoteview" -type f -iname "*.desktop" -print0 2>/dev/null | \
    xargs -r0 sed -ri "s|Icon=applications-internet|Icon=server|g"