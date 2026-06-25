# Post-build: stage "<PIOENV>.bin" (+ ".bin.md5") into .devtmp/ota/ so the local dev HTTP server can
# host several boards' firmware CONCURRENTLY under distinct filenames. Run one server on .devtmp/ota/
# and point each device's OTA Custom URL at its own <env>.bin -- no firmware.bin name collision.
#
# Also stages a short alias when the env sets `custom_ota_alias` (e.g. `custom_ota_alias = c35` ->
# c35.bin), so the dev OTA URL is easy to type (http://<host>:8000/c35.bin) and stable across builds.
import hashlib, os, shutil

Import("env")

def stage(source, target, env):
    bin_path = env.subst("$BUILD_DIR/${PROGNAME}.bin")
    out_dir = os.path.join(env["PROJECT_DIR"], ".devtmp", "ota")
    names = [env["PIOENV"]]
    alias = env.GetProjectOption("custom_ota_alias", "")
    if alias:
        names.append(alias)
    try:
        os.makedirs(out_dir, exist_ok=True)
        h = hashlib.md5()
        with open(bin_path, "rb") as f:
            for chunk in iter(lambda: f.read(65536), b""):
                h.update(chunk)
        digest = h.hexdigest()
        for n in names:
            dst = os.path.join(out_dir, n + ".bin")
            shutil.copyfile(bin_path, dst)
            with open(dst + ".md5", "w") as f:
                f.write(digest)
            print("dev_ota_stage: %s (md5 %s)" % (dst, digest))
    except OSError as e:
        print("dev_ota_stage: skipped (%s)" % e)

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", stage)
