#!/usr/bin/env python3
"""
To run this server:
- Go to the root of the project and create a Python virtual env:
    python3 -m venv venv
- Activate the virtual env:
    source ./venv/bin/activate
- Install the requirements:
    pip install -r webdev/requirements.txt
- Start the web server:
    cd webdev/
    ./app.py

If you need to run on an alternate port, just set a "PORT" environment variable, eg:
    PORT=8000 ./app.py

"""
import eventlet

eventlet.monkey_patch()

from flask import Flask, request, send_from_directory, jsonify
import flask_socketio
import os
import logging
import time
import pathlib

DATA_DIR = pathlib.Path("../data/")
DATADEV_DIR = pathlib.Path("../data-dev/")

logging.basicConfig(
    level=logging.INFO, format="%(asctime)s: %(levelname)s:%(name)s: %(message)s"
)
log = logging.getLogger(__name__)

app = Flask(__name__)
app.config["SECRET_KEY"] = "secret"
socketio = flask_socketio.SocketIO(app)

REF_CONFIG = {
    "heap": 36520,
    "version": "v0.0.0",
    "ssid": "SomeSSID",
    "colors": ["202010", "420C37", "030205"],
    "brightness": [3000, 676, 88, 0, 567],
    "total": 1800000,
    "used": 1000000,
    "free": 800000,
    "flashrealsize": 4000000,
}


@app.route("/edit", methods=["GET", "POST"])
def edit():
    if request.files:
        f = request.files["data"]
        fn = f.filename
        while fn.startswith("/"):
            fn = fn[1:]
        while ".." in fn:
            fn = fn.replace("..", ".")
        dstfn = DATADEV_DIR / fn
        os.makedirs(dstfn.parent, exist_ok=True)
        print("Saving:", dstfn)
        fh = open(dstfn, "wb")
        # time.sleep(.1)  # XXX
        f.save(fh)
    return "OK"


@app.route("/<path:path>")
@app.route("/")
def serve_static(path="/"):
    if path.endswith("/"):
        path += "index.htm"
    if path.startswith("/"):
        path = path[1:]
    if (DATA_DIR / path).is_file():
        fpath = DATA_DIR / path
    else:
        fpath = DATADEV_DIR / path
    # time.sleep(3)  # XXX
    print("Sending from:", fpath)
    return send_from_directory(fpath.parent, fpath.name)


@app.route("/reload")
def reload():
    # return "", 404  # XXX
    idx = DATA_DIR / "index.htm"
    mtime = idx.stat().st_mtime
    while 1:
        time.sleep(0.5)
        if mtime != idx.stat().st_mtime:
            log.info("File changed!")
            return "Change", 505
    return "Should never get here"


@app.route("/reboot", methods=["POST"])
def handlereboot():
    time.sleep(1)
    return "OK"


@app.route("/config")
def config():
    return jsonify(REF_CONFIG)


def main():
    kwargs = {"log_output": True}
    kwargs["debug"] = True
    kwargs["use_reloader"] = True
    port = int(os.environ.get("PORT", "80"))
    socketio.run(app, host="0.0.0.0", port=port, **kwargs)


@app.errorhandler(404)
def page_not_found(e):
    return "", 404


if __name__ == "__main__":
    main()
