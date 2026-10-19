# 🔐 PSP Go CryptoMD Vault (`05_CryptoMD_Vault`)

[![Platform](https://img.shields.io/badge/Platform-Sony%20PSP%20Go%20%7C%20PSP--N1000-003791?style=for-the-badge&logo=playstation&logoColor=white)](https://en.wikipedia.org/wiki/PlayStation_Portable_Go)
[![Architecture](https://img.shields.io/badge/Architecture-MIPS%20Allegrex%20R4000%20%40%20333MHz-blue?style=for-the-badge)](https://en.wikipedia.org/wiki/PlayStation_Portable_hardware)
[![Language](https://img.shields.io/badge/Language-C99%20Pure-orange?style=for-the-badge&logo=c)](https://en.wikipedia.org/wiki/C99)
[![Crypto](https://img.shields.io/badge/Crypto-ChaCha20%20%7C%20AES--256%20%7C%20HMAC--SHA256-critical?style=for-the-badge)](docs/ARCHITECTURE.md)
[![Graphics](https://img.shields.io/badge/Graphics-Direct%20VRAM%20%40%2060%20FPS-green?style=for-the-badge)](https://github.com/pspdev/pspsdk)
[![License](https://img.shields.io/badge/License-MIT-brightgreen?style=for-the-badge)](LICENSE)

> **An air-gapped cryptographic document station and native 60 FPS Markdown reader engineered in pure C99 for the Sony PlayStation Portable Go (PSP-N1000).**  
> Encrypts, authenticates, and renders `.md` and `.txt` documents directly in volatile RAM using a 4-tier cryptographic engine (**ChaCha20-256**, **AES-256-CBC**, **Autonomous Vigenère**, and **Structural Caesar**), physical button-combo key derivation (**PBKDF2-HMAC-SHA256**), and a 2-pass **FAT32 forensic shredder**.

---

### 🌐 Language / Idioma
* **English** (Current Document)
* 🇪🇸 **[Documentación Completa en Español (README_ES.md)](README_ES.md)**
* 📐 **[Technical Architecture & Security Audit (docs/ARCHITECTURE.md)](docs/ARCHITECTURE.md)**
* 🛠️ **[Toolchain & Deployment Guide (docs/TOOLCHAIN_SETUP.md)](docs/TOOLCHAIN_SETUP.md)**

---

## 📑 Table of Contents
1. [Project Overview](#-project-overview)
2. [The 4 Cryptographic Tiers](#-the-4-cryptographic-tiers)
3. [Key Features](#-key-features)
4. [User Interface & Ergonomic Key Entry](#-user-interface--ergonomic-key-entry)
5. [Controls & Interaction Matrix](#-controls--interaction-matrix)
6. [Binary Container Format (`.pdoc`)](#-binary-container-format-pdoc)
7. [Security Architecture & Vulnerability Hardening](#-security-architecture--vulnerability-hardening)
8. [Project Structure](#-project-structure)
9. [Building, Testing & Companion CLI](#-building-testing--companion-cli)
10. [Hardware Safety & USB Deployment](#-hardware-safety--usb-deployment)
11. [Technical Specifications](#-technical-specifications)
12. [License](#-license)

---

## 🎯 Project Overview

**CryptoMD Vault** turns the Sony PSP Go into a pocket-sized, physically isolated (*air-gapped*) cryptographic terminal for confidential notes and technical documentation. Designed from scratch in strict **C99** with **zero external library dependencies**, it implements NIST and IETF cryptographic primitives natively for the 32-bit MIPS Allegrex R4000 processor.

When opening an encrypted `.pdoc` container, **CryptoMD Vault** verifies the **HMAC-SHA256** tag in constant time (*Verify-then-Decrypt*) before decrypting a single byte. Plaintext is reconstructed exclusively inside static volatile RAM (`g_app.plain_buf`), formatted by a streaming Markdown engine, and rendered at a locked **60 FPS** without ever writing temporary files to the flash storage.

```
+-----------------------------------------------------------------------+
| BOVEDA RAM: ejemplo_nota.pdoc                            14:30   98%  |
+-----------------------------------------------------------------------+
|                                                                       |
|  # Documento de Prueba: CryptoMD Vault                                |
|                                                                       |
|  Este documento demuestra las capacidades de lectura y cifrado        |
|  nativo en la consola Sony PSP Go a 60 fotogramas por segundo.        |
|                                                                       |
|  ■ 4 Niveles de Cifrado: Cesar, Vigenere, AES-256 y ChaCha20          |
|  ■ Boveda Efimera en RAM: Sin rastro en la memoria flash              |
|  ■ Modo Rayos X: Mantener SELECT para inspeccionar el cifrado         |
|                                                                       |
+-----------------------------------------------------------------------+
| [O] Salir  [START] Guardar Descifrado  [HOLD SELECT] Rayos X          |
+-----------------------------------------------------------------------+
```

---

## 🛡️ The 4 Cryptographic Tiers

Users can choose the exact balance between classical educational ciphers and modern authenticated encryption when protecting any `.md` or `.txt` file:

| Tier | Algorithm | Key Input Method | Key Derivation & Integrity | Cryptographic Profile |
| :--- | :--- | :--- | :--- | :--- |
| **Level 1** | **Caesar (`ROT-N`)** | 7-Segment Rotary Dial (`1..25`) | SHA-256 Keyed MAC + CRC32 | **Structural Obfuscation:** Rotates letters while preserving Markdown syntax (`#`, `-`, `` ` ``) and folding Spanish UTF-8 diacritics. |
| **Level 2** | **Autonomous Vigenère** | Arcade Letter Drum (`A..Z`) | SHA-256 Keyed MAC + CRC32 | **Polyalphabetic Cipher:** Key stream advances only on alphabetic characters to prevent whitespace/punctuation key-length leakage. |
| **Level 3** | **AES-256-CBC** | Physical Button Combo (`2`, `4`, or `6` keys) | **PBKDF2-HMAC-SHA256** (`10,000` iters) + **HMAC-SHA256** | **Corporate Standard (FIPS 197):** 14-round Rijndael with PKCS#7 padding and constant-time padding validation. |
| **Level 4** | **ChaCha20-256** | Physical Button Combo (`2`, `4`, or `6` keys) | **PBKDF2-HMAC-SHA256** (`15,000` iters) + **HMAC-SHA256** | **Military-Grade ARX (RFC 8439):** 20-round Add-Rotate-XOR stream cipher optimized for the 32-bit MIPS ALU, immune to cache-timing attacks. |

---

## ✨ Key Features

* 🧠 **Zero-Footprint Ephemeral RAM Vault:**
  * Decrypted documents exist only in volatile SDRAM (`g_app.plain_buf`).
  * Closing a document (`O`), triggering the **Panic Lock** (`L + R` held for 1 second), or reaching the **120-second inactivity timeout** immediately scrubs all sensitive buffers via `crypto_zeroize()` using `volatile uint8_t *` pointers to prevent compiler Dead-Store Elimination (DSE).
* 🩻 **Real-Time X-Ray Mode (*Hold-to-See-Ciphertext*):**
  * Hold `SELECT` inside the Vault Reader to instantaneously switch between the decrypted Markdown document and the raw underlying ciphertext (formatted structural ciphertext for Levels 1–2, or live hexadecimal memory dump for Levels 3–4).
* 🎮 **Shoulder-Surfing-Proof Physical Key Entry:**
  * Instead of a slow on-screen QWERTY keyboard, Levels 3 and 4 allow selecting a key length of **2, 4, or 6 physical button presses** (`L`, `R`, D-Pad, and geometric buttons) confirmed with `SELECT`.
  * Dedicated **Caesar Decryption Dial** featuring a giant, centered 7-segment display (`01`–`25`) that resets to `01` before every operation.
* 🔄 **Clean File Lifecycle & Extension Preservation:**
  * Encrypting `notes.md` or `secret.txt` produces `notes.pdoc` / `secret.pdoc` while recording the original format (`PDOC_FMT_MD` or `PDOC_FMT_TXT`) inside the authenticated header.
  * Pressing `START` (*Save & Restore*) inside the Vault Reader atomically restores the plaintext file with its **original filename and extension** (`.md` or `.txt`) and automatically shreds/removes the `.pdoc` file so duplicates never accumulate.
* 🔥 **2-Pass FAT32 Forensic Shredder:**
  * After encrypting a file, a confirmation modal lets you either keep the original plaintext or destroy it via a 2-pass physical cluster overwrite (Pass 1: CSPRNG entropy stream; Pass 2: zero-fill) before unlinking the FAT32 directory entry.
* 📝 **60 FPS Direct-VRAM Markdown Renderer:**
  * Writes directly to uncached embedded VRAM (`0x44000000`) with hardware buffer swapping (`sceGuSwapBuffers()`), rendering `# H1`, `## H2`, `### H3`, fenced code blocks, blockquotes, wrapped bullet lists, and Spanish accents (`á, é, í, ó, ú, ñ, ¿, ¡`) at 60 FPS with analog inertial scrolling.
* 💻 **Cross-Platform Host CLI (`tools/cryptodoc_cli`):**
  * Companion command-line utility for macOS and Linux to pack, unpack, verify, inspect, and shred `.pdoc` containers from a computer.

---

## 🖥️ User Interface & Ergonomic Key Entry

### 1. Caesar Decryption Screen (Centered Giant 7-Segment Dial)
When unlocking a Level 1 `.pdoc` container, the interface presents a dedicated challenge screen with a large 7-segment digital dial centered in the viewport:

```
+-----------------------------------------------------------------------+
| DESCIFRAR DOCUMENTO (CESAR)                              14:30   98%  |
+-----------------------------------------------------------------------+
|  ¿Que numero de clave quieres utilizar para descifrarlo?              |
|  Usa Izquierda/Derecha o L/R para elegir el numero (1 a 25):          |
|                                                                       |
|                        +---------------------+                        |
|                        |      ___   ___      |                        |
|                        |  <  |   | |   |  >  |                        |
|                        |     |___| |___|     |                        |
|                        +---------------------+                        |
|                                                                       |
|     Pulsa [X] para descifrar el archivo con este numero.              |
+-----------------------------------------------------------------------+
| [Izq/Der o L/R] Elegir Numero  [X] Descifrar  [O] Volver              |
+-----------------------------------------------------------------------+
```

### 2. Physical Button Combination Selector (`2`, `4`, or `6` Keys)
For Levels 3 (AES-256) and 4 (ChaCha20), the wizard first asks the user to choose the combination length (**2, 4, or 6 buttons**), then displays centered progress slots and a prominent confirmation banner instructing the user to press **`SELECT`** to save/unlock:

```
+-----------------------------------------------------------------------+
| CONFIGURAR CLAVE DE BOTONES                              14:30   98%  |
+-----------------------------------------------------------------------+
|  Introduce una combinacion secreta de 4 teclas:                       |
|                                                                       |
|                     [ ■ ]   [ ■ ]   [ ■ ]   [ ■ ]                     |
|                                                                       |
|  Controles admitidos: L, R, Cruceta y Botones geometricos             |
|  +-----------------------------------------------------------------+  |
|  | ¡Combinacion completa! Pulsa [SELECT] para guardar combinacion  |  |
|  | O pulsa [O] si deseas borrar y corregir la ultima tecla.        |  |
|  +-----------------------------------------------------------------+  |
+-----------------------------------------------------------------------+
| [Botones] Ingresar  [O] Borrar/Salir  [SELECT] Guardar Combinacion    |
+-----------------------------------------------------------------------+
```

---

## 🎮 Controls & Interaction Matrix

| Control | Document Explorer | Plaintext Reader | Vault RAM Reader | Key Entry Screens |
| :--- | :--- | :--- | :--- | :--- |
| **Analog Stick** | — | Inertial smooth scroll | Inertial smooth scroll | — |
| **D-Pad Up / Down** | Select file | Step scroll ($\pm 18\text{ px}$) | Step scroll ($\pm 18\text{ px}$) | Change Vigenère letter / Combo input |
| **D-Pad Left / Right** | — | — | — | Rotate Caesar dial / Move drum slot / Combo input |
| **Triggers `L` / `R`** | — | Page Up / Page Down | Page Up / Page Down | Rotate Caesar dial / Combo input |
| **Chord `L + R` (>1s)** | — | — | **PANIC LOCK (Instant RAM Wipe)** | — |
| **Cross (`X`)** | Open `.md`/`.txt` or Unlock `.pdoc` | — | — | Confirm Caesar/Vigenère / Combo input |
| **Circle (`O`)** | — | Return to Explorer | Close Vault & **Zeroize RAM** | Delete last combo button / Cancel |
| **Triangle (`/\`)** | Encrypt selected file | — | — | Combo input / Keep plaintext in Modal |
| **Square (`[]`)** | Cycle Theme (Cyberpunk/Sepia/OLED) | Cycle Theme | Cycle Theme | Combo input |
| **`SELECT`** | Toggle `ef0:/DOCS` $\leftrightarrow$ `ms0:/DOCS` | — | **Hold: X-Ray Ciphertext View** | **Save / Confirm Button Combination** |
| **`START`** | — | Open Encryption Wizard | **Save Decrypted & Delete `.pdoc`** | Confirm Button Combination |

---

## 📦 Binary Container Format (`.pdoc`)

Every `.pdoc` file uses an authenticated **Encrypt-then-MAC** binary envelope consisting of a 64-byte little-endian header, variable-length ciphertext payload, and a 32-byte trailing **HMAC-SHA256** authentication tag:

```
+-----------------------------------------------------------------------+
| 0x00..0x3F : 64-Byte Public Header (Authenticated via HMAC-SHA256)    |
|   - magic[4]       : "PDOC" (0x50, 0x44, 0x4F, 0x43)                  |
|   - version        : 0x01                                             |
|   - cipher_level   : 1 (Caesar), 2 (Vigenere), 3 (AES-256), 4 (ChaCha)|
|   - orig_format    : 0 (.txt), 1 (.md)                                |
|   - key_mode       : 0 (Number), 1 (Word), 2 (Physical Button Combo)  |
|   - key_len_hint   : Slot count hint for UI (e.g., 2, 4, or 6)        |
|   - flags_pad[3]   : Reserved (0x000000)                              |
|   - kdf_iters      : PBKDF2-HMAC-SHA256 iteration count               |
|   - plain_len      : Original plaintext length in bytes               |
|   - cipher_len     : Encrypted payload length in bytes                |
|   - salt[16]       : 128-bit CSPRNG salt (or CRC32 in L1/L2)          |
|   - iv_nonce[16]   : 128-bit AES IV or 96-bit ChaCha20 Nonce          |
|   - created_utc    : 64-bit Unix UTC timestamp                        |
+-----------------------------------------------------------------------+
| 0x40..0x40+N : Ciphertext Payload (N = cipher_len bytes)              |
+-----------------------------------------------------------------------+
| Last 32 Bytes: HMAC-SHA256 Tag over (Header[0..63] || Ciphertext)     |
+-----------------------------------------------------------------------+
```

---

## 🔒 Security Architecture & Vulnerability Hardening

This codebase has undergone a comprehensive security audit targeting embedded C99 failure modes:

1. **Strict Verify-then-Decrypt (Constant-Time MAC):**
   * `pdoc_verify_decrypt()` computes the **HMAC-SHA256** over the 64-byte header and ciphertext and compares it against the trailing 32-byte tag using `crypto_constant_time_memcmp()` before any decryption routine is invoked. This neutralizes padding oracle attacks and chosen-ciphertext tampering.
2. **Integer Overflow & Malformed Container Defense:**
   * `pdoc_is_valid_header()` enforces strict upper bounds on both `plain_len` ($\le 128\text{ KB}$) and `cipher_len` ($\le 128\text{ KB} + 16\text{ B}$), verifies exact block alignment (`cipher_len % 16 == 0`) for AES-256-CBC, and requires `cipher_len == plain_len` for stream/classical tiers, preventing 32-bit `size_t` wrap-around buffer overflows.
3. **KDF Downgrade & CPU DoS Protection:**
   * Containers at Levels 3 and 4 enforce $1,000 \le \text{kdf\_iters} \le 100,000$, rejecting both maliciously downgraded headers (`kdf_iters = 0`) and denial-of-service headers (`kdf_iters = 0xFFFFFFFF`) that would freeze the 333 MHz processor.
4. **Constant-Time PKCS#7 Unpadding & Key Zeroization:**
   * `aes256_cbc_decrypt()` validates all padding bytes in constant time without early branch exits and scrubs AES round keys, state matrices, and CBC chaining blocks via `crypto_zeroize()`.
   * Stack-allocated key buffers (`key_data`, `salt`, `iv_nonce`) in both the PSP app and host CLI are zeroized immediately after use.
5. **Hardware Multi-Source Entropy Pool:**
   * `platform/psp_entropy.c` mixes the 64-bit microsecond RTC (`sceRtcGetCurrentTick`), system timer (`sceKernelGetSystemTimeLow`), battery ADC voltage (`scePowerGetBatteryVolt`), and human button/analog jitter into a 1024-bit pool whitened via **SHA-256** with feedback re-seeding.

---

## 🏗️ Project Structure

```
05_CryptoMD_Vault/
├── core/                        # Pure C99 Core (Zero PSP SDK dependencies)
│   ├── crypto_primitives.h/.c   # SHA-256, HMAC-SHA256, PBKDF2, constant-time cmp
│   ├── cipher_classic.h/.c      # Structural Caesar (ROT-N), Vigenere, UTF-8 folding, CRC32
│   ├── cipher_aes256.h/.c       # AES-256-CBC (FIPS 197) + constant-time PKCS#7
│   ├── cipher_chacha20.h/.c     # ChaCha20-256 (RFC 8439) 20-round ARX stream cipher
│   ├── pdoc_container.h/.c      # .pdoc binary packing, header validation & Verify-then-Decrypt
│   ├── md_parser.h/.c           # Streaming Markdown layout engine & UTF-8 sanitizer
│   ├── scroll_physics.h/.c      # Inertial analog scrolling & viewport clamping
│   ├── input_logic.h/.c         # Edge detection, L+R panic chord, PIN/drum state machines
│   └── app_state.h/.c           # Application FSM & automatic RAM vault zeroization
├── platform/                    # Hardware Abstraction Layer (PSP SDK / Host POSIX)
│   ├── psp_entropy.h/.c         # Hardware CSPRNG entropy collector (RTC + ADC + Jitter)
│   ├── psp_fs.h/.c              # ef0:/ & ms0:/ scanner, atomic writes & 2-pass FAT32 shredder
│   ├── psp_input.h/.c           # sceCtrl analog/digital polling & entropy feeding
│   └── psp_power.h/.c           # 333 MHz clock lock, battery telemetry & RTC formatting
├── ui/                          # Presentation Layer (480x272 @ 60 FPS)
│   ├── graphics.h/.c            # Uncached direct-VRAM (0x44000000) double-buffered renderer
│   ├── font_8x8.h/.c            # 8x8 bitmap ASCII + extended Spanish UTF-8 glyph table
│   ├── font_7seg.h/.c           # Procedural 7-segment digital display renderer
│   ├── theme_engine.h/.c        # Dark Cyberpunk, Sepia Paper, and OLED Black palettes
│   └── screens.h/.c             # Explorer, Plain/Vault Readers, Wizard, Key Entry & Modal views
├── tools/
│   └── cryptodoc_cli.c          # Companion macOS/Linux CLI tool (pack/unpack/inspect/verify/shred)
├── tests/
│   └── test_core.c              # 60 automated unit assertions (NIST, RFC, Markdown, Security)
├── docs/
│   ├── ARCHITECTURE.md          # Deep technical architecture & security audit report
│   └── TOOLCHAIN_SETUP.md       # Compilation, CLI usage & USB deployment guide
├── ejemplo_nota.md              # Sample Markdown document
├── prueba_cifrada.pdoc          # Sample Level 4 encrypted container (Combo: L -> R -> L -> R)
├── Dockerfile                   # Reproducible PSPDEV container build
├── activate.sh                  # Portable PSPDEV environment activation script
├── Makefile                     # Dual build system (PSP EBOOT.PBP + Host tests/CLI)
└── main.c                       # PSP entry point,User Mode setup & main event loop
```

---

## 🧰 Building, Testing & Companion CLI

### 1. Run the Automated Unit Test Suite (macOS / Linux Host)
Verify all 60 NIST/RFC cryptographic vectors, Markdown parser rules, and security boundary checks natively with `clang` or `gcc`:

```bash
make clean_host && make test
```

```text
====================================================
   05_CryptoMD_Vault: Comprehensive Test Suite     
====================================================
[*] Testing SHA-256 (FIPS 180-4)...
[*] Testing HMAC-SHA256 (RFC 4231)...
[*] Testing PBKDF2-HMAC-SHA256 (RFC 6070)...
[*] Testing AES-256 (FIPS 197)...
[*] Testing ChaCha20 (RFC 8439 §2.4.2)...
[*] Testing Caesar & Vigenere...
[*] Testing PDOC Container (Levels 1-4 & Tamper Resistance)...
[*] Testing Markdown Layout & Scroll Physics...
[*] Testing Input Logic & Application FSM...

====================================================
Results: 60 PASSED, 0 FAILED
====================================================
```

### 2. Build & Use the Companion CLI (`tools/cryptodoc_cli`)
Encrypt, decrypt, inspect, or shred documents on your computer for seamless interoperability with the PSP Go:

```bash
# Build CLI binary
make cli

# Pack a Markdown file using Level 4 (ChaCha20-256) with button combo L -> R -> L -> R
./tools/cryptodoc_cli pack --level 4 --key LRLR ejemplo_nota.md mi_secreto.pdoc

# Inspect container metadata
./tools/cryptodoc_cli inspect mi_secreto.pdoc

# Verify integrity and key without writing to disk
./tools/cryptodoc_cli verify --key LRLR mi_secreto.pdoc

# Unpack container back to Markdown
./tools/cryptodoc_cli unpack --key LRLR mi_secreto.pdoc nota_recuperada.md
```
*(Combo key tokens supported in CLI: `L`, `R`, `U` = Up, `D` = Down, `<` = Left, `>` = Right, `X` = Cross, `S` = Square, `T` = Triangle).*

### 3. Build the PSP Homebrew Executable (`EBOOT.PBP`)

**Option A — Using Local PSPDEV Toolchain:**
```bash
source ./activate.sh
make clean && make
```

**Option B — Using Docker (Zero local setup required):**
```bash
docker build -t psp-cryptomd-builder .
docker run --rm -v "$PWD":/src psp-cryptomd-builder
```

---

## ⚠️ Hardware Safety & USB Deployment

> [!CAUTION]
> **Strict Hardware Safety on PSP Go (PSP-N1000):**
> * **User Mode Only:** Declared with `PSP_MODULE_INFO("CryptoMDVault", 0, 1, 1)`. Never run kernel-mode flashers or `CIPL_Flasher` on a PSP Go.
> * **Storage Isolation:** File operations are strictly confined to user storage (`ef0:/DOCS` and `ms0:/DOCS`). Internal system partitions (`flash0:`–`flash3:`) are never accessed.

### Safe USB Deployment Protocol (macOS / Linux)
```bash
# 1. Create application and document directories on the PSP Go
mkdir -p "/Volumes/PSP GO/PSP/GAME/CryptoMDVault"
mkdir -p "/Volumes/PSP GO/DOCS"

# 2. Copy the compiled EBOOT.PBP and sample documents
cp EBOOT.PBP "/Volumes/PSP GO/PSP/GAME/CryptoMDVault/EBOOT.PBP"
cp ejemplo_nota.md prueba_cifrada.pdoc "/Volumes/PSP GO/DOCS/"

# 3. Clean macOS AppleDouble (._*) metadata files and flush disk buffers
dot_clean -m "/Volumes/PSP GO"
sync
```
*(Note: The included sample file `prueba_cifrada.pdoc` can be unlocked on the PSP Go with the 4-button sequence **`L` → `R` → `L` → `R`** followed by **`SELECT`**).*

---

## 📊 Technical Specifications

| Parameter | Specification | Notes |
| :--- | :--- | :--- |
| **Target Hardware** | Sony PlayStation Portable Go (`PSP-N1000`) | Compatible with PSP-1000 / 2000 / 3000 / Street |
| **Execution Mode** | PSP User Mode (`0x0000`) | 512 KB Thread Stack / 20 MB Heap reservation |
| **CPU / Bus Clock** | 333 MHz CPU / 166 MHz Bus | Locked via `scePowerSetClockFrequency(333, 333, 166)` |
| **Graphics Pipeline** | Direct Uncached VRAM (`0x44000000`) | 32-bit RGBA 8888, 480×272 @ 60.0 FPS VBLANK |
| **Cryptographic Core** | ChaCha20, AES-256-CBC, Vigenère, Caesar | Authenticated with HMAC-SHA256 & PBKDF2-SHA256 |
| **Max Document Size** | 128 KB (`PDOC_MAX_DOC_SIZE`) | Pre-allocated in `.bss` (Zero heap fragmentation) |
| **Auto-Lock Timeout** | 120 seconds (`7,200` frames) | Or instant Panic Lock via `L + R` (>1.0s) |

---

## 📄 License

This project is released under the **MIT License**. See the [LICENSE](LICENSE) file for full details.
