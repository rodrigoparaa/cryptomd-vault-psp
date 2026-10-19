#!/usr/bin/env bash

# Auto-detect PSPDev toolchain directory
if [ -n "$PSPDEV" ] && [ -d "$PSPDEV" ]; then
    export PSPDEV="$PSPDEV"
elif [ -d "/usr/local/pspdev" ]; then
    export PSPDEV="/usr/local/pspdev"
elif [ -d "/opt/pspdev" ]; then
    export PSPDEV="/opt/pspdev"
else
    FOUND_PSPDEV=$(find "$HOME" -maxdepth 5 -type d -name "pspdev" 2>/dev/null | head -n 1)
    if [ -n "$FOUND_PSPDEV" ] && [ -d "$FOUND_PSPDEV/bin" ]; then
        export PSPDEV="$FOUND_PSPDEV"
    else
        echo "[!] Warning: PSPDev toolchain directory not found. Set \$PSPDEV or use Docker."
        return 1 2>/dev/null || exit 1
    fi
fi

export PATH="$PSPDEV/bin:$PATH"

echo "[+] PSPDev environment activated: $PSPDEV"
echo "    psp-gcc version: $(psp-gcc -dumpversion 2>/dev/null || echo 'unknown')"
echo "    Run 'make clean && make' to build EBOOT.PBP."
echo "    Run 'deactivate_psp' to restore your original shell PATH."

deactivate_psp() {
    export PATH=$(echo "$PATH" | sed -e "s|$PSPDEV/bin:||g" -e "s|:$PSPDEV/bin||g")
    unset PSPDEV
    unset -f deactivate_psp
    echo "[-] PSPDev environment deactivated."
}
