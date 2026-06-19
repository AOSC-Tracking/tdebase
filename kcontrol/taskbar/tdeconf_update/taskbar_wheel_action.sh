#!/bin/sh

find "$TDEHOME/share/config" -mindepth 1 -maxdepth 1 -type f \
    '(' -name 'ktaskbarrc' -o -name 'taskbar_panelapplet_*_rc' ')' -print0 |
    xargs -0 grep -l 'CycleWheel' |
    xargs sed -i '
s/^CycleWheel=true$/CycleWindowsWAction=ModNone/
s/^CycleWheel=false$/CycleWindowsWAction=ActionDisabled/
/^CycleWheel=/d
'
