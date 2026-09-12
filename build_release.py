#!/usr/bin/env python3
"""
flybrain-nx: Release Builder & SD Card Packager
Packages genuine native Nintendo Switch AArch64 libnx production binaries,
embeds standard ASET + NACP metadata and application icons,
and generates the consolidated SD card release archive:
- flybrain-nx.nro (executable for Nintendo Switch Homebrew Menu with embedded ASET icon)
- flybrain-nx.xml (Atmosphère / HBL metadata)
- icon.jpg & icon.png (Homebrew Menu application icon)
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
ICON_PNG = os.path.join(BASE_DIR, "icon.png")

APP_TITLE = "flybrain-nx"
APP_AUTHOR = "vibecoderanon"
APP_VERSION = "1.0.0"

def ensure_icon_png():
    """Generates icon.png from icon.jpg if not present."""
    if os.path.exists(ICON_PNG):
        return
    if os.path.exists(ICON_JPG):
        try:
            from PIL import Image
            img = Image.open(ICON_JPG)
            img.save(ICON_PNG, "PNG")
            print(f"[+] Generated {ICON_PNG}")
        except Exception as e:
            print(f"[!] Could not convert icon.jpg to icon.png: {e}")

def attach_aset_metadata(nro_path: str, icon_path: str) -> None:
    """Ensures a compiled NRO has format-compliant ASET and NACP embedded metadata and icon."""
    if not os.path.exists(nro_path):
        return

    with open(nro_path, "rb") as f:
        data = f.read()

    if len(data) < 0x80 or data[0x10:0x14] != b"NRO0":
        return

    nro_body_size = struct.unpack_from("<I", data, 0x18)[0]
    # Check if ASET is already present
    if len(data) > nro_body_size and data[nro_body_size:nro_body_size+4] == b"ASET":
        print(f" [*] ASET metadata already attached to {nro_path}")
        return

    print(f" [*] Attaching authentic ASET & NACP metadata to {nro_path}...")
    nro_body = data[:nro_body_size]

    # NACP metadata struct (0x4000 bytes)
    nacp = bytearray(0x4000)
    t_bytes = APP_TITLE.encode("utf-8")
    a_bytes = APP_AUTHOR.encode("utf-8")
    for i in range(16):
        base = i * 0x300
        nacp[base:base + len(t_bytes)] = t_bytes
        nacp[base + 0x200:base + 0x200 + len(a_bytes)] = a_bytes
    v_bytes = f"{APP_VERSION}\x00".encode("utf-8")
    nacp[0x3060:0x3060 + len(v_bytes)] = v_bytes

    # Read icon bytes
    icon_bytes = b""
    if os.path.exists(icon_path):
        with open(icon_path, "rb") as f:
            icon_bytes = f.read()

    # ASET Asset Section Header (0x38 bytes)
    aset_hdr_size = 0x38
    icon_offset = aset_hdr_size
    nacp_offset = icon_offset + len(icon_bytes)
    nacp_padding = (8 - (nacp_offset % 8)) % 8
    nacp_offset += nacp_padding

    aset_hdr = bytearray(aset_hdr_size)
    aset_hdr[0:4] = b"ASET"
    struct.pack_into("<I", aset_hdr, 4, 0)  # version 0
    struct.pack_into("<QQ", aset_hdr, 8, icon_offset, len(icon_bytes))
    struct.pack_into("<QQ", aset_hdr, 24, nacp_offset, len(nacp))
    struct.pack_into("<QQ", aset_hdr, 40, 0, 0)  # RomFS

    full_nro = nro_body + aset_hdr + icon_bytes + (b"\x00" * nacp_padding) + nacp
    with open(nro_path, "wb") as f:
        f.write(full_nro)
    print(f" [+] Successfully attached ASET to {nro_path} ({len(full_nro):,} bytes)")

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

    # 1. Ensure icon PNG exists
    ensure_icon_png()

    # 2. Generate XML metadata
    generate_xml_metadata(XML_PATH)

    # 3. Compile if devkitPro is available locally
    try_compile_with_devkitpro()

    # 4. Check if production NRO is present (in BASE_DIR or RELEASE_DIR)
    source_nro = None
    if os.path.exists(NRO_PATH):
        source_nro = NRO_PATH
    elif os.path.exists(os.path.join(RELEASE_DIR, "flybrain-nx.nro")):
        source_nro = os.path.join(RELEASE_DIR, "flybrain-nx.nro")

    if not source_nro:
        print(
            f"\n[!] Note: Compiled NRO not found at {NRO_PATH}.\n"
            "    Production binaries are compiled natively via containerized CI\n"
            "    (devkitpro/devkita64:latest) in .github/workflows/build.yml."
        )
        return

    # 5. Embed authentic ASET metadata (Title, Author, Icon)
    attach_aset_metadata(source_nro, ICON_JPG)

    # 6. Copy production binaries into release/
    dest_nro = os.path.join(RELEASE_DIR, "flybrain-nx.nro")
    if source_nro != dest_nro:
        shutil.copy2(source_nro, dest_nro)

    if os.path.exists(XML_PATH):
        shutil.copy2(XML_PATH, os.path.join(RELEASE_DIR, "flybrain-nx.xml"))
    if os.path.exists(ICON_JPG):
        shutil.copy2(ICON_JPG, os.path.join(RELEASE_DIR, "icon.jpg"))
    if os.path.exists(ICON_PNG):
        shutil.copy2(ICON_PNG, os.path.join(RELEASE_DIR, "icon.png"))

    # 7. Create SD Card distribution zip bundle: switch/flybrain-nx/
    zip_name = f"{APP_TITLE}-switch-v{APP_VERSION}.zip"
    zip_path = os.path.join(RELEASE_DIR, zip_name)
    print(f"[*] Packaging consolidated SD Card zip bundle: {zip_path}...")

    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as zf:
        zf.write(dest_nro, "switch/flybrain-nx/flybrain-nx.nro")
        if os.path.exists(XML_PATH):
            zf.write(XML_PATH, "switch/flybrain-nx/flybrain-nx.xml")
        if os.path.exists(ICON_JPG):
            zf.write(ICON_JPG, "switch/flybrain-nx/icon.jpg")
        if os.path.exists(ICON_PNG):
            zf.write(ICON_PNG, "switch/flybrain-nx/icon.png")

    print(f"\n[+] Production NRO: {dest_nro} ({os.path.getsize(dest_nro):,} bytes)")
    print(f"[+] Release SD Card Archive: {zip_path} ({os.path.getsize(zip_path):,} bytes)")
    print("[+] Release build and ASET embedding completed successfully!")

if __name__ == "__main__":
    main()
