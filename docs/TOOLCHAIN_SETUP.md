# 🛠️ Toolchain, Companion CLI & USB Deployment Guide — `05_CryptoMD_Vault`

## 1. Host Unit Testing & Companion CLI (macOS / Linux)

The core cryptographic engine and Markdown parser are written in portable C99 and compile natively on macOS and Linux using `clang` or `gcc`.

### 1.1. Run the 60-Assertion Unit Test Suite
```bash
make clean_host && make test
```
This compiles and runs `tests/test_core`, validating:
1. **SHA-256** (NIST FIPS 180-4 vectors)
2. **HMAC-SHA256** (IETF RFC 4231 test cases)
3. **PBKDF2-HMAC-SHA256** (IETF RFC 6070 vectors)
4. **AES-256 Block & CBC-PKCS#7** (NIST FIPS 197 Appendix C.3 vectors)
5. **ChaCha20-256** (IETF RFC 8439 §2.4.2 vector)
6. **Structural Caesar & Autonomous Vigenère** (Spanish UTF-8 folding & Markdown preservation)
7. **PDOC Container & Security Hardening** (Tamper detection, wrong-key rejection, overflow & KDF downgrade defense)
8. **Markdown Layout Engine & Scroll Physics** (Inline formatting cleanup, badge filtering, bullet wrapping, inertial physics)
9. **Input FSM & Vault Zeroization** (L+R panic chord and automatic RAM scrubbing)

### 1.2. Build and Use the Host CLI (`tools/cryptodoc_cli`)
```bash
# Build the CLI tool
make cli

# Encrypt a Markdown file with Level 4 (ChaCha20-256) and 4-button combo (L, R, L, R)
./tools/cryptodoc_cli pack --level 4 --key LRLR ejemplo_nota.md secreto.pdoc

# Encrypt with Level 1 (Caesar shift 7)
./tools/cryptodoc_cli pack --level 1 --key 7 ejemplo_nota.md cesar.pdoc

# Inspect a .pdoc header without decrypting
./tools/cryptodoc_cli inspect secreto.pdoc

# Verify HMAC integrity and key validity
./tools/cryptodoc_cli verify --key LRLR secreto.pdoc

# Decrypt a .pdoc file back to plaintext
./tools/cryptodoc_cli unpack --key LRLR secreto.pdoc restaurado.md

# Securely shred a sensitive file (2-pass overwrite + unlink)
./tools/cryptodoc_cli shred archivo_sensible.txt
```

---

## 2. Building the PSP Executable (`EBOOT.PBP`)

### Option A: Local PSPDEV Toolchain
If you have the [PSPDEV](https://github.com/pspdev/pspdev) toolchain installed (in `$PSPDEV`, `/opt/pspdev`, `/usr/local/pspdev`, or inside your home directory):
```bash
source ./activate.sh
make clean && make
deactivate_psp
```

### Option B: Reproducible Docker Build
If you prefer not to install the MIPS cross-compiler locally:
```bash
docker build -t psp-cryptomd-builder .
docker run --rm -v "$PWD":/src psp-cryptomd-builder
```

---

## 3. Safe USB Deployment to Sony PSP Go (`PSP-N1000`)

1. Connect your PSP Go via USB and enable **USB Connection** on the XMB.
2. Copy `EBOOT.PBP` and your documents to the internal storage (`ef0:`, mounted as `/Volumes/PSP GO` on macOS):
   ```bash
   mkdir -p "/Volumes/PSP GO/PSP/GAME/CryptoMDVault"
   mkdir -p "/Volumes/PSP GO/DOCS"

   cp EBOOT.PBP "/Volumes/PSP GO/PSP/GAME/CryptoMDVault/EBOOT.PBP"
   cp ejemplo_nota.md prueba_cifrada.pdoc "/Volumes/PSP GO/DOCS/"
   ```
3. **Mandatory macOS FAT32 Cleanup:** Remove hidden AppleDouble (`._*`) resource-fork files and flush disk write buffers before ejecting to prevent *"Corrupted Data"* icons on the XMB:
   ```bash
   dot_clean -m "/Volumes/PSP GO"
   sync
   ```
4. Safely eject the volume and launch **CryptoMD Vault v1.0** from the PSP Go Game menu.
