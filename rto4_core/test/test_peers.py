"""Run SDK peers in separate processes so vendor/new symbols cannot collide."""

import argparse
import select
import socket
import subprocess


def exercise(server_binary, client_binary, ipc):
    flags = ["ipc"] if ipc else []
    with socket.socket() as reservation:
        reservation.bind(("127.0.0.1", 0))
        requested_port = reservation.getsockname()[1]
    server = subprocess.Popen(
        [server_binary, "server", str(requested_port), *flags],
        stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
    )
    try:
        ready, _, _ = select.select([server.stdout], [], [], 5)
        if not ready:
            raise RuntimeError("Server did not announce readiness")
        line = server.stdout.readline().strip()
        if not line.startswith("READY "):
            raise RuntimeError(f"Invalid server readiness: {line}")
        port = line.split()[1]
        client = subprocess.run(
            [client_binary, "client", port, *flags],
            capture_output=True, text=True, timeout=12,
        )
        if client.returncode or "PASS snapshot publish RPC error" not in client.stdout:
            raise RuntimeError(
                f"Peer failed ({client.returncode}): {client.stdout}\n{client.stderr}"
            )
        print(f"PASS server={server_binary} client={client_binary} ipc={ipc}")
    finally:
        try:
            _, errors = server.communicate(input="stop\n", timeout=3)
        except subprocess.TimeoutExpired:
            server.kill()
            _, errors = server.communicate(timeout=3)
            raise RuntimeError(f"Server did not shut down cleanly: {errors}")
        if errors:
            print(f"Server diagnostics: {errors}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("peer")
    parser.add_argument("--vendor")
    args = parser.parse_args()
    if args.vendor:
        source_bytes = subprocess.check_output([args.peer, "bytes"], timeout=3)
        vendor_bytes = subprocess.check_output([args.vendor, "bytes"], timeout=3)
        if source_bytes != vendor_bytes:
            raise RuntimeError(f"Wire bytes differ: {source_bytes} != {vendor_bytes}")
        print(f"PASS independent serialization bytes: {source_bytes.decode().strip()}")
    for use_ipc in (False, True):
        exercise(args.peer, args.peer, use_ipc)
        if args.vendor:
            exercise(args.vendor, args.peer, use_ipc)
            exercise(args.peer, args.vendor, use_ipc)
