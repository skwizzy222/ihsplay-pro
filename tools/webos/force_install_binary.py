#!/usr/bin/env python3
"""Force-copy ihsplay binary from IPK onto the TV (hbchannel install can silently no-op)."""
import hashlib
import socket
import time
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from threading import Thread

TV = "192.168.2.41"
PC = "192.168.2.8"
PORT = 8080
IPK = Path(r"C:\Users\Michael\Projects\ihsplay-pro\dist\org.ihsplay.pro_0.3.0_arm.ipk")
APP = "/media/developer/apps/usr/palm/applications/org.ihsplay.pro"


def main():
    sha = hashlib.sha256(IPK.read_bytes()).hexdigest()
    print("IPK", IPK.stat().st_size, sha)

    class Handler(SimpleHTTPRequestHandler):
        def __init__(self, *a, **k):
            super().__init__(*a, directory=str(IPK.parent), **k)

        def log_message(self, *args):
            pass

    httpd = ThreadingHTTPServer(("0.0.0.0", PORT), Handler)
    Thread(target=httpd.serve_forever, daemon=True).start()

    s = socket.create_connection((TV, 23), 5)
    s.settimeout(5)
    time.sleep(0.6)
    try:
        s.recv(4096)
    except Exception:
        pass
    s.send(b"\n")
    time.sleep(0.3)
    try:
        s.recv(4096)
    except Exception:
        pass

    def run(cmd, wait=12):
        print(">>>", cmd[:140])
        s.send((cmd + "\n").encode())
        time.sleep(0.4)
        buf = b""
        end = time.time() + wait
        while time.time() < end:
            try:
                c = s.recv(65536)
                if c:
                    buf += c
                    if b"/ # " in buf or b"# " in buf[-8:]:
                        # keep reading a bit more for slow cmds
                        if time.time() > end - 1:
                            break
            except Exception:
                break
        text = buf.decode("utf-8", "replace")
        print(text[-1500:])
        return text

    run(f"wget -O /tmp/ihsplay.ipk http://{PC}:{PORT}/{IPK.name}", wait=20)
    run("rm -rf /tmp/ihs_ipk; mkdir -p /tmp/ihs_ipk; cd /tmp/ihs_ipk && ar x /tmp/ihsplay.ipk && tar xzf data.tar.gz", wait=25)
    run(f"cp -f /tmp/ihs_ipk/usr/palm/applications/org.ihsplay.pro/ihsplay {APP}/ihsplay && chmod 755 {APP}/ihsplay", wait=8)
    run(f"ls -le {APP}/ihsplay; grep -a 'BT_PAIR_VIA_LUNASEND_PUB' {APP}/ihsplay >/dev/null && echo MARKER_OK || echo MARKER_MISSING", wait=8)
    s.close()
    httpd.shutdown()
    print("FORCE_INSTALL_DONE — reopen the app from Home")


if __name__ == "__main__":
    main()
