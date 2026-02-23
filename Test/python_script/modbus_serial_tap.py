#!/usr/bin/env python3
import os, pty, sys, time, select, termios, tty, pwd

def set_raw(fd):
    tty.setraw(fd, when=termios.TCSANOW)

def set_serial_19200_8E1(fd):
    attrs = termios.tcgetattr(fd)

    # iflag, oflag, cflag, lflag, ispeed, ospeed, cc
    iflag, oflag, cflag, lflag, ispeed, ospeed, cc = attrs

    # Raw-ish on the serial side
    iflag &= ~(termios.IGNBRK | termios.BRKINT | termios.PARMRK |
               termios.ISTRIP | termios.INLCR | termios.IGNCR |
               termios.ICRNL | termios.IXON | termios.IXOFF | termios.IXANY)
    oflag &= ~termios.OPOST
    lflag &= ~(termios.ECHO | termios.ECHONL | termios.ICANON |
               termios.ISIG | termios.IEXTEN)

    # 8 data bits
    cflag &= ~termios.CSIZE
    cflag |= termios.CS8

    # Even parity, 1 stop bit
    cflag |= termios.PARENB
    cflag &= ~termios.PARODD
    cflag &= ~termios.CSTOPB

    # No HW flow control (CRTSCTS may not exist on all platforms)
    if hasattr(termios, "CRTSCTS"):
        cflag &= ~termios.CRTSCTS

    # Enable receiver, ignore modem control lines
    cflag |= (termios.CREAD | termios.CLOCAL)

    # Speed
    ispeed = termios.B19200
    ospeed = termios.B19200

    # Read returns as soon as any byte is available (no interbyte timeout here)
    cc[termios.VMIN] = 1
    cc[termios.VTIME] = 0

    termios.tcsetattr(fd, termios.TCSANOW, [iflag, oflag, cflag, lflag, ispeed, ospeed, cc])

def hexdump_line(prefix, data):
    hx = " ".join(f"{b:02X}" for b in data)
    asc = "".join(chr(b) if 32 <= b <= 126 else "." for b in data)
    return f"{prefix} {hx:<48}  |{asc}|"

def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} /dev/ttyUSB0 [pty_link=/tmp/mbus]", file=sys.stderr)
        sys.exit(2)

    dev = sys.argv[1]
    pty_link = sys.argv[2] if len(sys.argv) >= 3 else "/tmp/mbus"

    # Create PTY for the app
    master_fd, slave_fd = pty.openpty()
    slave_name = os.ttyname(slave_fd)

    user = os.environ.get("SUDO_USER") or os.environ.get("USER") or "krzysztof"
    uid = pwd.getpwnam(user).pw_uid
    gid = pwd.getpwnam(user).pw_gid

    os.chown(slave_name, uid, gid)
    os.chmod(slave_name, 0o660)

    # Make a stable symlink path for the app
    try:
        if os.path.islink(pty_link) or os.path.exists(pty_link):
            os.unlink(pty_link)
        os.symlink(slave_name, pty_link)
    except PermissionError:
        print(f"Permission denied creating symlink {pty_link}. Try a path in /tmp or run with sudo.", file=sys.stderr)
        raise

    # Set PTY to raw
    set_raw(master_fd)
    set_raw(slave_fd)

    # Open real serial device
    ser_fd = os.open(dev, os.O_RDWR | os.O_NOCTTY)
    set_serial_19200_8E1(ser_fd)

    tx_path = "/tmp/modbus_tx.bin"   # app -> device
    rx_path = "/tmp/modbus_rx.bin"   # device -> app

    with open(tx_path, "ab", buffering=0) as f_tx, open(rx_path, "ab", buffering=0) as f_rx:
        print(f"PTY for app: {pty_link} -> {slave_name}", file=sys.stderr)
        print(f"Real device : {dev} (19200 8E1)", file=sys.stderr)
        print(f"Logging TX  : {tx_path}", file=sys.stderr)
        print(f"Logging RX  : {rx_path}", file=sys.stderr)
        print("Press Ctrl+C to stop.", file=sys.stderr)

        poll = select.poll()
        poll.register(master_fd, select.POLLIN)
        poll.register(ser_fd, select.POLLIN)

        try:
            while True:
                for fd, ev in poll.poll(1000):
                    if ev & select.POLLIN:
                        if fd == master_fd:
                            data = os.read(master_fd, 4096)
                            if not data:
                                continue
                            os.write(ser_fd, data)
                            f_tx.write(data)
                            sys.stderr.write(hexdump_line("TX>", data) + "\n")
                        elif fd == ser_fd:
                            data = os.read(ser_fd, 4096)
                            if not data:
                                continue
                            os.write(master_fd, data)
                            f_rx.write(data)
                            sys.stderr.write(hexdump_line("<RX", data) + "\n")
        except KeyboardInterrupt:
            pass
        finally:
            os.close(ser_fd)
            os.close(master_fd)
            os.close(slave_fd)

if __name__ == "__main__":
    main()
