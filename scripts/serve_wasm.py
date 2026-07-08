#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Minimal static file server for the OpenRTX WebAssembly build.
#
# pthreads in the browser are backed by a SharedArrayBuffer, which the browser
# only exposes to cross-origin-isolated pages. That requires two response
# headers that Python's default http.server does not send:
#
#     Cross-Origin-Opener-Policy:   same-origin
#     Cross-Origin-Embedder-Policy: require-corp
#
# This server adds them (and the correct .wasm MIME type) so the emulator can
# actually spin up its worker threads.
#
# Usage:
#     python scripts/serve_wasm.py [build_dir] [--port N]
#
# then open http://localhost:8000/openrtx_wasm.html

import argparse
import functools
import http.server
import os
import socketserver


class IsolatedHandler(http.server.SimpleHTTPRequestHandler):
    """Serves files with the cross-origin isolation headers pthreads need."""

    extensions_map = {
        **http.server.SimpleHTTPRequestHandler.extensions_map,
        ".wasm": "application/wasm",
        ".js": "text/javascript",
    }

    def end_headers(self):
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        # Emscripten output is regenerated on every build; don't cache it.
        self.send_header("Cache-Control", "no-store")
        super().end_headers()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", nargs="?", default="build_wasm",
                        help="directory to serve (default: build_wasm)")
    parser.add_argument("--port", type=int, default=8000, help="TCP port")
    args = parser.parse_args()

    directory = os.path.abspath(args.directory)
    handler = functools.partial(IsolatedHandler, directory=directory)

    with socketserver.ThreadingTCPServer(("", args.port), handler) as httpd:
        print(f"Serving {directory} at http://localhost:{args.port}/")
        print(f"Open   http://localhost:{args.port}/openrtx_wasm.html")
        print("Press Ctrl+C to stop.")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nStopped.")


if __name__ == "__main__":
    main()
