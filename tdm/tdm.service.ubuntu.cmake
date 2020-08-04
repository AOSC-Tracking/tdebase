[Unit]
Description=Trinity Display Manager
Documentation=man:tdm-trinity(1)
Conflicts=getty@tty7.service plymouth-quit.service
After=systemd-user-sessions.service getty@tty7.service plymouth-quit.service

[Service]
# temporary safety check until all DMs are converted to correct
# display-manager.service symlink handling
ExecStartPre=/bin/sh -c '[ "$(basename $(cat /etc/X11/default-display-manager 2>/dev/null))" = "tdm" ]'
ExecStart=@BIN_INSTALL_DIR@/tdm
Restart=always
