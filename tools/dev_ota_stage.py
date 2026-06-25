# Post-build: stage "<PIOENV>.bin" (+ ".bin.md5") into .devtmp/ota/ so the local dev HTTP server can
# host several boards' firmware CONCURRENTLY under distinct filenames. Run one server on .devtmp/ota/
# and point each device's OTA Custom URL at its own <env>.bin -- no firmware.bin name collision.
#
# Also stages a short alias when the env sets `custom_ota_alias` (e.g. `custom_ota_alias = c35` ->
# c35.bin), so the dev OTA URL is easy to type (http://<host>:8000/c35.bin) and stable across builds.
#
# CRASH DECODABILITY: a coredump can only be decoded against the EXACT elf that built the running
# binary, and Arduino embeds a build timestamp so the app-SHA changes every build (and .pio/build's elf
# is overwritten by the next build). So we ARCHIVE every build's elf under sha256(elf)[:16] -- which is
# exactly the `app_sha256` the on-device crash report prints. A crash on ANY past build is then
# decodable (look up archive/<app_sha256>.elf), so development never has to stall waiting for a crash.
# archive/index.tsv maps each hash -> git rev / env / time. Pruned to the most recent ARCHIVE_KEEP elfs.
import hashlib, os, shutil, subprocess, time

Import("env")  # noqa: F821

ARCHIVE_KEEP = 24   # elfs are ~46MB each; keep the most recent N (covers "the last few versions/test runs")

def _sha256_16(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()[:16]

def _git_rev(project_dir):
    try:
        return subprocess.check_output(["git", "-C", project_dir, "describe", "--always", "--dirty", "--tags"],
                                       stderr=subprocess.DEVNULL).decode().strip()
    except Exception:
        return "?"

def _prune(arch):
    elfs = [os.path.join(arch, f) for f in os.listdir(arch) if f.endswith(".elf")]
    elfs.sort(key=os.path.getmtime, reverse=True)
    for p in elfs[ARCHIVE_KEEP:]:
        try: os.remove(p)
        except OSError: pass

def stage(source, target, env):
    bin_path = env.subst("$BUILD_DIR/${PROGNAME}.bin")
    elf_path = env.subst("$BUILD_DIR/${PROGNAME}.elf")
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

        # Archive the elf under its app_sha256 (= what the crash report shows) so it's always findable.
        esha = None
        if os.path.isfile(elf_path):
            esha = _sha256_16(elf_path)
            arch = os.path.join(out_dir, "archive")
            os.makedirs(arch, exist_ok=True)
            adst = os.path.join(arch, esha + ".elf")
            if not os.path.exists(adst):
                shutil.copyfile(elf_path, adst)
                with open(os.path.join(arch, "index.tsv"), "a") as ix:
                    ix.write("%s\t%s\t%s\t%s\n" % (
                        esha, _git_rev(env["PROJECT_DIR"]), env["PIOENV"],
                        time.strftime("%Y-%m-%d %H:%M:%S")))
            _prune(arch)

        for n in names:
            dst = os.path.join(out_dir, n + ".bin")
            shutil.copyfile(bin_path, dst)
            with open(dst + ".md5", "w") as f:
                f.write(digest)
            if os.path.isfile(elf_path):   # also keep a "latest" elf beside each alias for convenience
                shutil.copyfile(elf_path, os.path.join(out_dir, n + ".elf"))
            print("dev_ota_stage: %s (md5 %s, app_sha256 %s)" % (dst, digest, esha or "?"))
    except OSError as e:
        print("dev_ota_stage: skipped (%s)" % e)

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", stage)
