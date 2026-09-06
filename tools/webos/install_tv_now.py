#!/usr/bin/env python3
import hashlib
import socket
import time
from pathlib import Path

TV = "192.168.2.41"
PC = "192.168.2.8"
IPK_PATH = Path(r"C:\Users\Michael\Projects\ihsplay-pro\dist\org.ihsplay.pro_0.3.0_arm.ipk")
IPK = IPK_PATH.name
SHA = hashlib.sha256(IPK_PATH.read_bytes()).hexdigest()
URL = f"http://{PC}:8080/{IPK}"
print("SHA", SHA, "bytes", IPK_PATH.stat().st_size)


def main():
    s = socket.create_connection((TV, 23), 5)
    s.settimeout(4)
    time.sleep(0.8)
    try:
        s.recv(8192)
    except Exception:
        pass
    s.send(b"\n")
    time.sleep(0.3)
    try:
        s.recv(8192)
    except Exception:
        pass

    def run(cmd, wait=8):
        print(">>>", cmd[:160])
        s.send((cmd + "\n").encode())
        time.sleep(0.5)
        buf = b""
        end = time.time() + wait
        while time.time() < end:
            try:
                c = s.recv(65536)
                if c:
                    buf += c
            except Exception:
                pass
        text = buf.decode("utf-8", "replace")
        print(text)
        print("====")
        return text

    run(f"wget -O /tmp/ihsplay.ipk {URL}; ls -la /tmp/ihsplay.ipk", 25)
    # Homebrew install — keep subscription long enough to finish
    payload = '{"ipkUrl":"%s","ipkHash":"%s"}' % (URL, SHA)
    run("luna-send -i -f luna://org.webosbrew.hbchannel.service/install '" + payload + "'", 45)
    run("ls -la /media/developer/apps/usr/palm/applications/org.ihsplay.pro/ihsplay", 3)
    run("luna-send -n 1 -f luna://com.webos.applicationManager/launch '{\"id\":\"org.ihsplay.pro\"}'", 5)
    s.close()


if __name__ == "__main__":
    main()
