# PSP Go CryptoMD Vault (`05_CryptoMD_Vault`)

[![Platform](https://img.shields.io/badge/Plataforma-Sony%20PSP%20Go%20%7C%20PSP--N1000-003791?style=for-the-badge&logo=playstation&logoColor=white)](https://en.wikipedia.org/wiki/PlayStation_Portable_Go)
[![Architecture](https://img.shields.io/badge/Arquitectura-MIPS%20Allegrex%20R4000%20%40%20333MHz-blue?style=for-the-badge)](https://en.wikipedia.org/wiki/PlayStation_Portable_hardware)
[![Language](https://img.shields.io/badge/Lenguaje-C99%20Puro-orange?style=for-the-badge&logo=c)](https://en.wikipedia.org/wiki/C99)
[![Crypto](https://img.shields.io/badge/Cifrado-ChaCha20%20%7C%20AES--256%20%7C%20HMAC--SHA256-critical?style=for-the-badge)](docs/ARCHITECTURE.md)
[![Graphics](https://img.shields.io/badge/Gráficos-Direct%20VRAM%20%40%2060%20FPS-green?style=for-the-badge)](https://github.com/pspdev/pspsdk)
[![License](https://img.shields.io/badge/Licencia-MIT-brightgreen?style=for-the-badge)](LICENSE)

> **Estación criptográfica documental *air-gapped* y lector nativo de Markdown a 60 FPS desarrollado íntegramente en C99 puro para Sony PlayStation Portable Go (PSP-N1000).**  
> Cifra, autentica y maqueta documentos `.md` y `.txt` directamente en memoria RAM volátil mediante una arquitectura de 4 niveles criptográficos (**ChaCha20-256**, **AES-256-CBC**, **Vigenère Autónomo** y **César Estructural**), derivación de claves por secuencias físicas de botones (**PBKDF2-HMAC-SHA256**) y un destructor forense **FAT32 de doble pasada**.

---

### Idioma / Language
* **[English Documentation (README.md)](README.md)**
* **Español** (Documento Actual)
* **[Memoria Técnica de Arquitectura y Auditoría de Seguridad (docs/ARCHITECTURE.md)](docs/ARCHITECTURE.md)**
* **[Guía de Toolchain, CLI y Despliegue USB (docs/TOOLCHAIN_SETUP.md)](docs/TOOLCHAIN_SETUP.md)**

---

## Tabla de Contenidos
1. [Visión General del Proyecto](#visión-general-del-proyecto)
2. [Los 4 Niveles Criptográficos](#los-4-niveles-criptográficos)
3. [Características Principales](#características-principales)
4. [Interfaz de Usuario y Ergonomía de Claves](#interfaz-de-usuario-y-ergonomía-de-claves)
5. [Matriz de Controles e Interacción](#matriz-de-controles-e-interacción)
6. [Especificación del Contenedor Binario (`.pdoc`)](#especificación-del-contenedor-binario-pdoc)
7. [Auditoría de Seguridad y Mitigación de Vulnerabilidades](#auditoría-de-seguridad-y-mitigación-de-vulnerabilidades)
8. [Estructura del Proyecto](#estructura-del-proyecto)
9. [Compilación, Pruebas y Herramienta CLI](#compilación-pruebas-y-herramienta-cli)
10. [Seguridad de Hardware y Despliegue por USB](#seguridad-de-hardware-y-despliegue-por-usb)
11. [Especificaciones Técnicas](#especificaciones-técnicas)
12. [Licencia](#licencia)

---

## Visión General del Proyecto

**CryptoMD Vault** transforma la consola Sony PSP Go en una bóveda criptográfica de bolsillo físicamente aislada de red (*air-gapped*) para proteger notas confidenciales y documentación técnica en formato Markdown (`.md`) o texto plano (`.txt`). Construido desde cero en **C99 estricto** sin ninguna dependencia externa, implementa estándares oficiales del NIST y del IETF optimizados para el procesador MIPS Allegrex R4000 de 32 bits.

Al abrir un contenedor cifrado `.pdoc`, la aplicación verifica la firma **HMAC-SHA256** en tiempo constante (*Verify-then-Decrypt*) antes de desencriptar un solo byte. El documento en claro se reconstruye exclusivamente en memoria RAM volátil (`g_app.plain_buf`), se maqueta al vuelo y se renderiza a **60 fotogramas por segundo** sin escribir archivos temporales en la memoria flash.

---

## Los 4 Niveles Criptográficos

| Nivel | Algoritmo | Método de Clave | Derivación e Integridad | Propósito y Perfil Técnico |
| :--- | :--- | :--- | :--- | :--- |
| **Nivel 1** | **César (`ROT-N`)** | Dial de 7 Segmentos (`1..25`) | MAC SHA-256 + CRC32 | **Ofuscación Estructural:** Rota caracteres alfabéticos preservando el formato Markdown (`#`, `-`, `` ` ``) y aplicando plegado fonético español. |
| **Nivel 2** | **Vigenère Autónomo** | Tambor Alfabético (`A..Z`) | MAC SHA-256 + CRC32 | **Polialfabético Clásico:** El índice de clave avanza solo sobre letras para evitar fugas estadísticas de longitud por espacios o signos. |
| **Nivel 3** | **AES-256-CBC** | Combo de Botones (`2`, `4` o `6` teclas) | **PBKDF2-HMAC-SHA256** (`10.000` rondas) + **HMAC-SHA256** | **Estándar Corporativo (FIPS 197):** Cifrado por bloques de 128 bits con clave de 256 bits y validación PKCS#7 en tiempo constante. |
| **Nivel 4** | **ChaCha20-256** | Combo de Botones (`2`, `4` o `6` teclas) | **PBKDF2-HMAC-SHA256** (`15.000` rondas) + **HMAC-SHA256** | **Grado Militar ARX (RFC 8439):** Cifrado de flujo de 20 rondas Add-Rotate-XOR nativo para la ALU MIPS de 32 bits, inmune a ataques de caché. |

---

## Características Principales

* **Bóveda Efímera en RAM (*Zero-Disk-Footprint*):**
  * El texto descifrado reside únicamente en SDRAM volátil. Al salir con `O`, activar el **Panic Lock** (`L + R` > 1 segundo) o cumplirse los **120 segundos de inactividad**, toda la memoria sensible se sobrescribe con ceros mediante punteros `volatile uint8_t *` (`crypto_zeroize`).
* **Modo Rayos X en Tiempo Real (*Hold-to-See-Ciphertext*):**
  * Mantén pulsado `SELECT` dentro del visor de bóveda para inspeccionar en vivo el criptograma subyacente (texto Markdown rotado en Niveles 1 y 2, o volcado hexadecimal de memoria en Niveles 3 y 4).
* **Entrada de Clave Ergonómica y Selector de Longitud:**
  * **Selector de 2, 4 o 6 teclas:** Antes de introducir una combinación física en los Niveles 3 y 4, el usuario elige si desea una clave de **2, 4 o 6 botones** (`L`, `R`, Cruceta y botones geométricos) y confirma su guardado con **`SELECT`**.
  * **Pantalla dedicada de descifrado César:** Al abrir un archivo cifrado con César, se muestra una pantalla exclusiva que pregunta el número de clave con un **display gigante de 7 segmentos centrado en mitad de pantalla** (reiniciado siempre en `01`).
* **Ciclo Limpio de Archivos y Restauración de Extensión Original:**
  * Al cifrar `nota.md` o `apuntes.txt`, se genera `nota.pdoc` o `apuntes.pdoc` almacenando el formato original (`PDOC_FMT_MD` o `PDOC_FMT_TXT`) en la cabecera autenticada.
  * Al pulsar `START` (*Guardar Descifrado*) en el visor de bóveda, el archivo se restaura automáticamente con su **nombre y extensión original (`.md` o `.txt`)** y **se elimina el archivo `.pdoc` cifrado**, evitando que se acumulen archivos duplicados.
* **Destructor Forense FAT32 (*Shredder* de 2 Pasadas):**
  * Permite destruir el archivo original en claro tras cifrarlo sobrescribiendo sus sectores físicos primero con entropía pseudoaleatoria y luego con ceros antes de desvincularlo del directorio FAT32.
* **Renderizador Markdown Directo a VRAM a 60 FPS:**
  * Escritura directa en VRAM no cacheada (`0x44000000`) con limpieza automática de marcadores inline (`**negrita**`, `*cursiva*`, `` `código` ``), simplificación de enlaces `[texto](url)`, soporte de tildes y eñes (`á, é, í, ó, ú, ñ, ¿, ¡`) y scroll analógico inercial.

---

## Interfaz de Usuario y Ergonomía de Claves

### 1. Pantalla de Descifrado César (Número Grande Centrado)
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

### 2. Introducción de Combinación Física (2, 4 o 6 Teclas + Confirmación con `SELECT`)
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

## Matriz de Controles e Interacción

| Control | Explorador Documental | Lector en Claro | Visor Bóveda RAM | Pantallas de Clave |
| :--- | :--- | :--- | :--- | :--- |
| **Stick Analógico** | — | Scroll inercial suave | Scroll inercial suave | — |
| **Cruceta Arriba/Abajo** | Seleccionar archivo | Scroll paso a paso | Scroll paso a paso | Cambiar letra Vigenère / Tecla combo |
| **Cruceta Izq/Der** | — | — | — | Girar dial César / Mover casilla / Tecla combo |
| **Gatillos `L` / `R`** | — | Página Arriba / Abajo | Página Arriba / Abajo | Girar dial César / Tecla combo |
| **Acorde `L + R` (>1s)** | — | — | **PANIC LOCK (Purga RAM inmediata)** | — |
| **Botón Cruz (`X`)** | Abrir `.md`/`.txt` o Desbloquear `.pdoc` | — | — | Confirmar César/Vigenère / Tecla combo |
| **Botón Círculo (`O`)** | — | Volver al Explorador | Cerrar Bóveda y **Purgar RAM** | Borrar última tecla / Cancelar |
| **Botón Triángulo (`/\`)**| Cifrar archivo seleccionado | — | — | Tecla combo / Conservar copia en Modal |
| **Botón Cuadrado (`[]`)** | Cambiar Tema (Cyberpunk/Sepia/OLED) | Cambiar Tema | Cambiar Tema | Tecla combo |
| **Botón `SELECT`** | Alternar `ef0:/DOCS` $\leftrightarrow$ `ms0:/DOCS` | — | **Mantener: Modo Rayos X** | **Guardar / Confirmar Combinación** |
| **Botón `START`** | — | Abrir Asistente de Cifrado | **Guardar Descifrado y Borrar `.pdoc`** | Confirmar Combinación |

---

## Especificación del Contenedor Binario (`.pdoc`)

Cada archivo `.pdoc` encapsula el documento bajo un esquema autenticado **Encrypt-then-MAC** compuesto por una cabecera pública de 64 bytes, el cuerpo cifrado y una etiqueta **HMAC-SHA256** de 32 bytes al final del archivo:

* **`magic[4]`**: `"PDOC"` (`0x50, 0x44, 0x4F, 0x43`)
* **`version`**: `0x01`
* **`cipher_level`**: `1` (César), `2` (Vigenère), `3` (AES-256-CBC), `4` (ChaCha20-256)
* **`orig_format`**: `0` (`.txt`), `1` (`.md`) — utilizado para restaurar la extensión original al guardar.
* **`key_mode`**: `0` (Número), `1` (Palabra), `2` (Combo de botones físicos)
* **`key_len_hint`**: Longitud de la clave (`2`, `4` o `6` teclas en combo, o longitud de palabra)
* **`kdf_iters`**: Iteraciones PBKDF2 (`10.000` en Nivel 3, `15.000` en Nivel 4)
* **`plain_len` / `cipher_len`**: Tamaño original y tamaño cifrado validados estrictamente contra desbordamientos.
* **`salt[16]` / `iv_nonce[16]`**: Sal aleatoria de 128 bits e IV/Nonce único por operación.
* **`hmac_tag[32]`**: Firma HMAC-SHA256 sobre `Cabecera || TextoCifrado`.

---

## Auditoría de Seguridad y Mitigación de Vulnerabilidades

El código ha sido auditado y blindado frente a vulnerabilidades clásicas en C embebido:

1. **Validación Estricta de Cabeceras contra Desbordamientos de Entero y Búfer:**
   * `pdoc_is_valid_header()` verifica que `plain_len <= 128 KB` y `cipher_len <= 128 KB + 16 B`, exige múltiplo exacto de 16 bytes en AES-256-CBC e igualdad `cipher_len == plain_len` en los demás niveles, impidiendo ataques de desbordamiento aritmético en arquitecturas de 32 bits.
2. **Protección contra Ataques de Degradación KDF y Denegación de Servicio (DoS):**
   * En los Niveles 3 y 4 se exige que `1.000 <= kdf_iters <= 100.000`, rechazando tanto archivos manipulados con `kdf_iters = 0` (degradación de seguridad) como valores extremos que bloquearían la CPU a 333 MHz.
3. **Verificación PKCS#7 y HMAC en Tiempo Constante:**
   * Tanto la comparación de la etiqueta HMAC-SHA256 (`crypto_constant_time_memcmp`) como la validación del relleno PKCS#7 en `aes256_cbc_decrypt()` se ejecutan sin ramificaciones dependientes de datos secretos, previniendo ataques de canal lateral (*Padding Oracle* y *Timing Attacks*).
4. **Purga Sistemática de Secretos en Pila y RAM (`crypto_zeroize`):**
   * Las matrices de estado internas de AES, bloques de encadenamiento CBC, contextos SHA-256/HMAC y búferes de clave en pila (`key_data`, `salt`, `iv_nonce`) se limpian con `crypto_zeroize()` inmediatamente después de cada operación.

---

## Compilación, Pruebas y Herramienta CLI

```bash
# 1. Ejecutar las 60 pruebas unitarias (NIST, RFC, Markdown y Seguridad)
make clean_host && make test

# 2. Compilar la utilidad CLI para macOS / Linux
make cli

# 3. Compilar EBOOT.PBP para PSP Go con el toolchain local
source ./activate.sh
make clean && make
```

---

## Licencia

Distribuido bajo la **Licencia MIT**. Consulta el archivo [LICENSE](LICENSE) para más información.
