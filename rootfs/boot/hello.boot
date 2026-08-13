[Unit]
Description=Boot hello unit for CortexOS
WantedBy=boot.target

[Service]
ExecStart=echo Boot completed successfully
