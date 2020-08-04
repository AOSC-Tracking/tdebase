[Unit]
Description=Trinity Display Manager
Documentation=man:tdm-trinity(1)
Conflicts=getty@tty7.service plymouth-quit.service
After=systemd-user-sessions.service getty@tty7.service plymouth-quit.service

[Service]
ExecStart=@BIN_INSTALL_DIR@/tdm
Restart=always
IgnoreSIGPIPE=no

[Install]
Alias=display-manager.service
