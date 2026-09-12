#!/usr/bin/env python3
"""
flybrain-nx: Release Builder & SD Card Packager
Packages genuine native Nintendo Switch AArch64 libnx production binaries
and generates the consolidated SD card release archive:
- flybrain-nx.nro (executable for Nintendo Switch Homebrew Menu)
- flybrain-nx.xml (Atmosphère / HBL metadata)
- icon.jpg (Homebrew Menu application icon)
- release/flybrain-nx-switch-v1.0.0.zip (SD card layout ready for sdmc:/switch/flybrain-nx/)
"""

import os
import shutil
import struct
import subprocess
import zipfile

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
RELEASE_DIR = os.path.join(BASE_DIR, "release")
ROMFS_DIR = os.path.join(BASE_DIR, "romfs")
NRO_PATH = os.path.join(BASE_DIR, "flybrain-nx.nro")
XML_PATH = os.path.join(BASE_DIR, "flybrain-nx.xml")
ICON_JPG = os.path.join(BASE_DIR, "icon.jpg")

APP_TITLE = "flybrain-nx"
APP_AUTHOR = "vibecoderanon"
APP_VERSION = "1.0.0"

def try_compile_with_devkitpro() -> bool:
    """Attempts to run make if devkitA64 toolchain is detected in the environment."""
    if not shutil.which("make"):
        return False
    if "DEVKITPRO" not in os.environ and not os.path.exists("/opt/devkitpro"):
        return False
    print("[*] devkitPro toolchain detected, compiling flybrain-nx with make...")
    try:
        res = subprocess.run(["make", "-j4"], cwd=BASE_DIR, capture_output=True, text=True)
        if res.returncode == 0 and os.path.exists(NRO_PATH):
            print(f" [+] Successfully compiled native NRO via devkitA64: {NRO_PATH}")
            return True
        else:
            print(f" [!] Make returned {res.returncode}:\n{res.stderr}")
            return False
    except Exception as e:
        print(f" [!] Compilation attempt failed: {e}")
        return False

def generate_xml_metadata(xml_path: str):
    """Generates Homebrew Menu XML metadata file."""
    xml_content = f"""<?xml version="1.0" encoding="utf-8"?>
<config>
  <name>{APP_TITLE}</name>
  <author>{APP_AUTHOR}</author>
  <version>{APP_VERSION}</version>
  <category>Neuroscience / Simulation</category>
  <description>Adult Drosophila Melanogaster Connectome SNN Homebrew &amp; 3D Brain Activity Visualizer</description>
</config>
"""
    with open(xml_path, "w", encoding="utf-8") as f:
        f.write(xml_content)
    print(f"[+] Generated {xml_path}")

def main():
    print(f"[*] Packaging {APP_TITLE} v{APP_VERSION}...")
    os.makedirs(RELEASE_DIR, exist_ok=True)

    # 1. Generate XML metadata
    generate_xml_metadata(XML_PATH)

    # 2. Compile if devkitPro is available locally
    try_compile_with_devkitpro()

    # 3. Check if production NRO is present
    if not os.path.exists(NRO_PATH):
        print(
            f"\n[!] Note: Compiled NRO not found at {NRO_PATH}.\n"
            "    Production binaries are compiled natively via containerized CI\n"
            "    (devkitpro/devkita64:latest) in .github/workflows/build.yml."
        )
        return

    # 4. Copy production binaries into release/
    shutil.copy2(NRO_PATH, os.path.join(RELEASE_DIR, "flybrain-nx.nro"))
    if os.path.exists(XML_PATH):
        shutil.copy2(XML_PATH, os.path.join(RELEASE_DIR, "flybrain-nx.xml"))
    if os.path.exists(ICON_JPG):
        shutil.copy2(ICON_JPG, os.path.join(RELEASE_DIR, "icon.jpg"))

    # 5. Create SD Card distribution zip bundle: switch/flybrain-nx/
    zip_name = f"{APP_TITLE}-switch-v{APP_VERSION}.zip"
    zip_path = os.path.join(RELEASE_DIR, zip_name)
    print(f"[*] Packaging consolidated SD Card zip bundle: {zip_path}...")

    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as zf:
        zf.write(NRO_PATH, "switch/flybrain-nx/flybrain-nx.nro")
        if os.path.exists(XML_PATH):
            zf.write(XML_PATH, "switch/flybrain-nx/flybrain-nx.xml")
        if os.path.exists(ICON_JPG):
            zf.write(ICON_JPG, "switch/flybrain-nx/icon.jpg")

    print(f"\n[+] Production NRO: {NRO_PATH} ({os.path.getsize(NRO_PATH):,} bytes)")
    print(f"[+] Release SD Card Archive: {zip_path} ({os.path.getsize(zip_path):,} bytes)")
    print("[+] Release build completed successfully!")

if __name__ == "__main__":
    main()
