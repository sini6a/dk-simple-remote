"""Build the bench firmware with the WCH compiler, or run portable C tests.

Target flags/libraries/script follow HID_Consumer/.cproject. No vendor substitutes.
"""
import argparse
import hashlib
import json
import os
import re
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def run(args):
    print(" ".join(str(a) for a in args), flush=True)
    subprocess.run([str(a) for a in args], cwd=ROOT, check=True)


def compiler(name):
    result = shutil.which(name)
    if not result and os.name == "nt":
        result = shutil.which(name + ".exe")
    if not result:
        sys.exit(f"Required compiler not found: {name}. See README.md. No firmware built.")
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=["test", "release", "debug", "diagnostic"])
    parser.add_argument("--board", choices=["rev1", "rev2"], default="rev2",
                        help="rev1: SW2 on PB0 (2026-10-28 boards); rev2: SW2 on PB22/BOOT")
    args = parser.parse_args()
    board_rev = args.board[-1]
    # rev2 keeps the historic build/<mode> paths; other revisions get a suffix.
    suffix = "" if args.board == "rev2" else "-" + args.board
    sdk_dir=ROOT/'vendor/ch583'
    if not (sdk_dir/'EVT/EXAM/BLE/LIB/libCH58xBLE.a').is_file():
        sys.exit('Official SDK missing. Run: python tools/setup_sdk.py')
    out = ROOT / "build" / (args.mode + ("" if args.mode == "test" else suffix))
    out.mkdir(parents=True, exist_ok=True)
    if args.mode == "test":
        cc = compiler(os.environ.get("HOST_CC", "gcc"))
        exe = out / ("buttons_test.exe" if os.name == "nt" else "buttons_test")
        run([cc, "-include", "config/app_config.h", "-std=c99", "-Wall", "-Wextra", "-Werror", "-pedantic",
             "-Iconfig", "-Isrc/drivers", "-Isrc/app", "tests/buttons_test.c",
             "src/drivers/buttons.c", "src/app/keymap.c", "-o", exe])
        run([exe])
        battery_exe=out / ("battery_test.exe" if os.name=="nt" else "battery_test")
        run([cc, "-include", "config/app_config.h","-std=c99","-Wall","-Wextra","-Werror","-Isrc/power",
             "tests/battery_test.c","src/power/battery_estimate.c","-o",battery_exe])
        run([battery_exe])
        adc_exe=out / ("battery_adc_test.exe" if os.name=="nt" else "battery_adc_test")
        run([cc, "-include", "config/app_config.h","-std=c99","-Wall","-Wextra","-Werror","-Itests/battery_mock","-Isrc/power",
             "tests/battery_adc_test.c","src/power/battery_adc.c","src/power/battery_estimate.c","-o",adc_exe])
        run([adc_exe])
        sleep_exe = out / ("sleep_test.exe" if os.name == "nt" else "sleep_test")
        run([cc, "-include", "config/app_config.h", "-std=c99", "-Wall", "-Wextra", "-Werror", "-Itests/power_mock",
             "-Iconfig", "-Isrc/drivers", "tests/sleep_test.c", "src/power/sleep.c", "-o", sleep_exe])
        run([sleep_exe])
        gpio_exe = out / ("gpio_test.exe" if os.name == "nt" else "gpio_test")
        run([cc, "-include", "config/app_config.h", "-std=c99", "-Wall", "-Wextra", "-Werror", "-Itests/power_mock",
             "-Iconfig", "-Isrc/drivers", "tests/gpio_test.c", "src/drivers/buttons_gpio.c", "-o", gpio_exe])
        run([gpio_exe])
        gpio1_exe = out / ("gpio_rev1_test.exe" if os.name == "nt" else "gpio_rev1_test")
        run([cc, "-include", "config/app_config.h", "-DBOARD_REV=1", "-std=c99", "-Wall", "-Wextra", "-Werror",
             "-Itests/power_mock", "-Iconfig", "-Isrc/drivers", "tests/gpio_test.c", "src/drivers/buttons_gpio.c",
             "-o", gpio1_exe])
        run([gpio1_exe])
        remote_exe = out / ("remote_test.exe" if os.name == "nt" else "remote_test")
        run([cc, "-include", "config/app_config.h", "-std=c99", "-Wall", "-Wextra", "-Werror", "-Itests/remote_mock",
             "-Iconfig", "-Isrc/drivers", "-Isrc/app", "-Isrc/ble", "tests/remote_test.c",
             "src/app/remote.c", "src/drivers/buttons.c", "src/app/keymap.c", "-o", remote_exe])
        run([remote_exe])
        reports_exe = out / ("hid_reports_test.exe" if os.name == "nt" else "hid_reports_test")
        run([cc, "-include", "config/app_config.h", "-std=c99", "-Wall", "-Wextra", "-Werror", "-pedantic",
             "-Iconfig", "-Isrc/app", "-Isrc/ble", "tests/hid_reports_test.c",
             "src/ble/hid_reports.c", "src/app/keymap.c", "-o", reports_exe])
        run([reports_exe])
        battery_service_exe=out / ("battery_service_test.exe" if os.name=="nt" else "battery_service_test")
        battery_sdk=["BLE/HAL/include","BLE/LIB","SRC/StdPeriphDriver/inc","SRC/RVMSIS","BLE/HID_Consumer/Profile/include"]
        run([cc, "-include", "config/app_config.h","-DAPP_DIAGNOSTICS=1","-std=gnu99","-Os","-Wall","-Wextra","-Werror","-Iconfig","-Isrc/ble","-Isrc/power","-Isrc/drivers",
             *["-Ivendor/ch583/EVT/EXAM/"+p for p in battery_sdk],
             "tests/battery_service_test.c","src/power/battery_estimate.c","-o",battery_service_exe])
        run([battery_service_exe])
        power_exe=out / ("power_manager_test.exe" if os.name=="nt" else "power_manager_test")
        run([cc,"-include","config/app_config.h","-std=gnu99","-Os","-Wall","-Wextra","-Werror",
             "-Iconfig","-Isrc/power",*["-Ivendor/ch583/EVT/EXAM/"+p for p in battery_sdk],
             "tests/power_manager_test.c","-o",power_exe])
        run([power_exe])
        run([sys.executable,"tests/check_radio_config.py",cc])

        run([cc, "-include", "config/app_config.h","-std=gnu99","-Os","-Wall","-Wextra","-Werror","-DAPP_DIAGNOSTICS=0",
             "-Iconfig","-Isrc/ble","-Isrc/power","-Isrc/drivers",
             *["-Ivendor/ch583/EVT/EXAM/"+p for p in battery_sdk],
             "tests/battery_service_test.c","src/power/battery_estimate.c","-o",battery_service_exe])
        run([battery_service_exe])
        manager_exe = out / ("ble_manager_test.exe" if os.name == "nt" else "ble_manager_test")
        sdk_includes = ["BLE/HAL/include", "BLE/LIB", "SRC/StdPeriphDriver/inc",
                        "SRC/RVMSIS", "BLE/HID_Consumer/Profile/include"]
        run([cc, "-include", "config/app_config.h", "-std=gnu99", "-Os", "-Wall", "-Wextra", "-Werror", "-Iconfig", "-Isrc/ble",
             *["-Ivendor/ch583/EVT/EXAM/" + p for p in sdk_includes],
             "tests/ble_manager_test.c", "-o", manager_exe])
        run([manager_exe])
        service_exe = out / ("hid_service_test.exe" if os.name == "nt" else "hid_service_test")
        run([cc, "-include", "config/app_config.h", "-std=gnu99", "-Os", "-Wall", "-Wextra", "-Werror", "-Iconfig", "-Isrc/app", "-Isrc/ble",
             *["-Ivendor/ch583/EVT/EXAM/" + p for p in sdk_includes],
             "tests/hid_service_test.c", "src/ble/hid_reports.c", "src/app/keymap.c", "-o", service_exe])
        run([service_exe])
        transport_exe = out / ("wch_hid_transport_test.exe" if os.name == "nt" else "wch_hid_transport_test")
        run([cc, "-include", "config/app_config.h", "-std=gnu99", "-Os", "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections",
             "-Iconfig", "-Isrc/ble", *["-Ivendor/ch583/EVT/EXAM/" + p for p in sdk_includes],
             "tests/wch_hid_transport_test.c", "-o", transport_exe])
        run([transport_exe])
        diagnostic_exe = out / ("diagnostic_service_test.exe" if os.name == "nt" else "diagnostic_service_test")
        run([cc, "-include", "config/app_config.h", "-DAPP_DIAGNOSTICS=1", "-std=gnu99", "-Os", "-Wall", "-Wextra", "-Werror", "-Iconfig", "-Isrc/ble",
             *["-Ivendor/ch583/EVT/EXAM/" + p for p in sdk_includes],
             "tests/diagnostic_service_test.c", "-o", diagnostic_exe])
        run([diagnostic_exe])
        run([sys.executable, "tests/test_battery_curve.py"])
        run([sys.executable, "tests/test_descriptor.py"])
        run([sys.executable, "tests/check_wch_baseline.py"])
        return
    run([sys.executable, "tools/check_hardware.py", "--board", args.board])
    prefix = os.environ.get("CROSS_COMPILE", "riscv-none-embed-")
    if "CROSS_COMPILE" not in os.environ and os.name == "nt" and not (
            shutil.which(prefix + "gcc") or shutil.which(prefix + "gcc.exe")):
        # MounRiver Studio 3 bundles the SDK-compatible GCC 8 alongside newer
        # toolchains. Select only this exact compiler; never substitute an ISA.
        matches = list(Path("C:/MounRiver").glob(
            "*/resources/app/resources/win32/components/WCH/Toolchain/"
            "RISC-V Embedded GCC/bin/riscv-none-embed-gcc.exe"))
        if len(matches) == 1:
            prefix = str(matches[0])[:-len("gcc.exe")]
    cc = compiler(prefix + "gcc")
    run([cc, "--version"])
    sdk = Path("vendor/ch583/EVT/EXAM")
    drv = sdk / "SRC/StdPeriphDriver"
    hal = sdk / "BLE/HAL"
    flags = ["-include", "config/app_config.h", "-march=rv32imac", "-mabi=ilp32", "-mcmodel=medany", "-Os",
             "-fsigned-char", "-ffunction-sections", "-fdata-sections", "-fno-common",
             "-std=gnu99", "-DCLK_OSC32K=1", "-DCENTRAL_MAX_CONNECTION=0",
             "-DHAL_SLEEP=1", "-DHAL_KEY=0", "-DHAL_LED=0", "-DBLE_MAC=0",
             "-DBOARD_REV=" + board_rev]
    if args.mode == "diagnostic":
        flags += ["-DAPP_DIAGNOSTICS=1"]
    if args.mode in ("debug", "diagnostic"):
        flags += ["-g3", "-DAPP_DEBUG=1"]  # No SDK DEBUG UART side effects.
    includes = ["config", "src/app", "src/drivers", "src/ble", "src/power",
                str(drv / "inc"), str(sdk / "SRC/RVMSIS"),
                str(hal / "include"), str(sdk / "BLE/LIB"),
                str(sdk / "BLE/HID_Consumer/Profile/include")]
    flags += ["-I" + p for p in includes]
    sources = sorted(Path("src").rglob("*.c"))
    sources += [hal / n for n in ("MCU.c", "RTC.c")]
    sources += [drv / ("CH58x_" + n + ".c")
                for n in ("sys", "clk", "gpio", "pwr", "flash", "adc", "uart1")]
    sources += [sdk / "SRC/RVMSIS/core_riscv.c", sdk / "SRC/Startup/startup_CH583.S"]
    objects = []
    for index, source in enumerate(sources):
        obj = out / f"{index}_{source.stem}.o"
        run([cc, *flags, "-c", source, "-o", obj])
        objects.append(obj)
    elf = out / "remote-bench.elf"
    run([cc, *flags, "-nostartfiles", "--specs=nano.specs", "--specs=nosys.specs",
         "-T" + str(sdk / "SRC/Ld/Link.ld"), "-Wl,--gc-sections",
         "-Wl,--print-memory-usage", "-Wl,-Map=" + str(out / "remote-bench.map"),
         *objects, "-L" + str(drv), "-L" + str(sdk / "BLE/LIB"),
         "-lISP583", "-lCH58xBLE", "-o", elf])
    run([prefix + "objcopy", "-O", "ihex", elf, out / "remote-bench.hex"])
    run([prefix + "objcopy", "-O", "binary", elf, out / "remote-bench.bin"])
    run([prefix + "size", elf])
    version = re.search(r'^#define APP_FIRMWARE_VERSION "([A-Za-z0-9_.-]+)"',
                        (ROOT / "config/app_config.h").read_text(), re.M).group(1)
    for extension in ("hex", "bin"):
        shutil.copy2(out / ("remote-bench." + extension),
                     out / ("ch583-remote-" + version + suffix + "." + extension))

    inputs=[*Path("src").rglob("*.c"),*Path("src").rglob("*.h"),
            *Path("config").rglob("*"),Path("sdk.lock.json"),Path("tools/build.py")]
    inputs={p.as_posix():hashlib.sha256(p.read_bytes()).hexdigest()
            for p in inputs if p.is_file()}
    artifacts={p.name:hashlib.sha256(p.read_bytes()).hexdigest()
               for p in out.glob("ch583-remote-"+version+suffix+".*") if p.suffix in (".hex",".bin")}
    (out/"firmware-manifest.json").write_text(json.dumps({"version":version,"mode":args.mode,
        "board":args.board,
        "sdk_revision":json.loads(Path("sdk.lock.json").read_text())["revision"],
        "inputs_sha256":inputs,"artifacts_sha256":artifacts},indent=2)+"\n")


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as error:
        sys.exit(error.returncode)
