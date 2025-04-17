import subprocess
import gzip
import base64
import numpy

# Build the artifacts
subprocess.run(["zig", "build"])

libraries = {
    "LINUX_X86_64": "zig-out/lib/libclippy-linux-x86_64.so",
    "LINUX_AARCH64": "zig-out/lib/libclippy-linux-aarch64.so",
    "WINDOWS_X86_64": "zig-out/bin/clippy-windows-x86_64.dll",
    "WINDOWS_AARCH64": "zig-out/bin/clippy-windows-aarch64.dll",
    "MACOS_X86_64": "zig-out/lib/libclippy-macos-x86_64.dylib",
    "MACOS_AARCH64": "zig-out/lib/libclippy-macos-aarch64.dylib",
}

code = {}

for name, path in libraries.items():
    with open(path, "rb") as f:
        blob = f.read()
        blob_zipped = gzip.compress(blob)
        blob_encoded = base64.b64encode(blob_zipped)

        code[name] = blob_encoded.decode("utf-8")

weights_floats = numpy.loadtxt("python/o7.txt")
weights = [round(x) for x in weights_floats]

with open("python/clippy_shell/pre.py") as f:
    pre = f.read()

with open("python/clippy_shell/post.py") as f:
    post = f.read()

with open("python/clippy_agent.py", "w") as f:

    f.write(pre)

    f.write(f"WEIGHTS = {repr(weights)}\n\n")

    for name, enc in code.items():
        f.write(f"CODE_{name} = \"{enc}\"\n")

    f.write(post)