#!/usr/bin/env bash
# ============================================================================
#  CortexOS - Dependency Installer
#  Multi-platform: Linux (Arch, Debian, Fedora, SUSE, Void, Alpine, Gentoo,
#                   NixOS, FreeBSD), macOS, Windows (MSYS2/Git Bash/WSL)
# ============================================================================
set -euo pipefail

# ── Colors ──────────────────────────────────────────────────────────────────
if [[ -t 1 ]] && command -v tput &>/dev/null && [[ "$(tput colors 2>/dev/null || echo 0)" -ge 8 ]]; then
    R='\033[0;31m'   # Red
    G='\033[0;32m'   # Green
    Y='\033[0;33m'   # Yellow
    B='\033[0;34m'   # Blue
    M='\033[0;35m'   # Magenta
    C='\033[0;36m'   # Cyan
    W='\033[1;37m'   # White (bold)
    DIM='\033[2m'    # Dim
    BOLD='\033[1m'   # Bold
    RESET='\033[0m'  # Reset
else
    R='' G='' Y='' B='' M='' C='' W='' DIM='' BOLD='' RESET=''
fi

# ── Symbols ─────────────────────────────────────────────────────────────────
OK="${G}[+]${RESET}"
FAIL="${R}[!]${RESET}"
INFO="${C}[*]${RESET}"
WARN="${Y}[~]${RESET}"
ASK="${M}[?]${RESET}"
STAR="${Y}[*]${RESET}"
ARROW="${C}->${RESET}"
CHECK="${G}${BOLD}✓${RESET}"
CROSS="${R}${BOLD}✗${RESET}"

# ── Helpers ─────────────────────────────────────────────────────────────────
banner() {
    echo ""
    echo -e "${C}${BOLD}"
    cat << 'BANNER'
    _   _ _____ _____     _____         _
   | \ | |_   _| ____|   |  ___|_ _ __| | ___  _ __
   |  \| | | | |  _|     | |_ / _` / __| |/ _ \| '_ \
   | |\  | | | | |___    |  _| (_| \__ \ | (_) | | | |
   |_| \_| |_| |_____|   |_|  \__,_|___/_|\___/|_| |_|
BANNER
    echo -e "${RESET}"
    echo -e "${W}   Dependency Installer${RESET}"
    echo -e "${DIM}   Multi-platform • Interactive • Colorful${RESET}"
    echo ""
    echo -e "${DIM}$(printf '─%.0s' {1..56})${RESET}"
    echo ""
}

log_ok()   { echo -e "  ${CHECK}  ${G}$1${RESET}"; }
log_fail() { echo -e "  ${CROSS}  ${R}$1${RESET}"; }
log_info() { echo -e "  ${INFO}  ${B}$1${RESET}"; }
log_warn() { echo -e "  ${WARN}  ${Y}$1${RESET}"; }
log_ask()  { echo -e "  ${ASK}  ${M}$1${RESET}"; }

section() {
    echo ""
    echo -e "  ${C}${BOLD}$1${RESET}"
    echo -e "${DIM}  $(printf '─%.0s' {1..50})${RESET}"
}

ask_confirm() {
    local msg="$1"
    local default="${2:-y}"
    local hint
    if [[ "$default" == "y" ]]; then
        hint="[Y/n]"
    else
        hint="[y/N]"
    fi
    echo ""
    read -rp "$(echo -e "  ${ASK}  ${W}$msg${RESET} $DIM$hint${RESET} ")" answer
    answer="${answer:-$default}"
    [[ "${answer,,}" == "y" || "${answer,,}" == "yes" ]]
}

spinner() {
    local pid=$1
    local msg="${2:-Working...}"
    local chars='⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏'
    local i=0
    while kill -0 "$pid" 2>/dev/null; do
        printf "\r  ${C}%s${RESET}  %s" "${chars:i++%${#chars}:1}" "$msg"
        sleep 0.08
    done
    printf "\r%*s\r" $((${#msg} + 6)) ""
}

detect_os() {
    OS=""
    # Termux is Linux-like but has its own package manager and filesystem.
    # Detect it before /etc/os-release so it is not mistaken for Debian.
    if [[ -n "${TERMUX_VERSION:-}" ]] || [[ "${PREFIX:-}" == /data/data/com.termux/files/usr* ]]; then
        OS="termux"
        DISTRO="termux"
        return
    fi
    if [[ -n "${MACOS_DETECTED:-}" ]] || [[ "$(uname)" == "Darwin" ]]; then
        OS="macos"
        DISTRO="macos"
        return
    fi
    if [[ -f /etc/os-release ]]; then
        . /etc/os-release
        DISTRO="${ID:-unknown}"
    elif [[ -f /etc/lsb-release ]]; then
        . /etc/lsb-release
        DISTRO="${DISTRIB_ID:-unknown}"
    elif [[ "$(uname)" == "FreeBSD" ]]; then
        DISTRO="freebsd"
    else
        DISTRO="unknown"
    fi
    OS="linux"
}

detect_pkg_mgr() {
    PKG_MGR=""
    case "$DISTRO" in
        termux)
            PKG_MGR="termux" ;;
        arch|manjaro|endeavouros|garuda)
            PKG_MGR="pacman" ;;
        ubuntu|debian|linuxmint|pop|elementary|zorin|kali|raspbian)
            PKG_MGR="apt" ;;
        fedora)
            PKG_MGR="dnf" ;;
        opensuse*|suse*|sles)
            PKG_MGR="zypper" ;;
        void)
            PKG_MGR="xbps" ;;
        alpine)
            PKG_MGR="apk" ;;
        gentoo|funtoo)
            PKG_MGR="emerge" ;;
        nixos|nix)
            PKG_MGR="nix" ;;
        freebsd)
            PKG_MGR="pkg" ;;
        macos)
            PKG_MGR="brew" ;;
        *)
            if [[ -n "${TERMUX_VERSION:-}" ]] && command -v pkg &>/dev/null; then
                PKG_MGR="termux"
            elif command -v apt &>/dev/null; then
                PKG_MGR="apt"
            elif command -v pacman &>/dev/null; then
                PKG_MGR="pacman"
            elif command -v dnf &>/dev/null; then
                PKG_MGR="dnf"
            elif command -v brew &>/dev/null; then
                PKG_MGR="brew"
            fi
            ;;
    esac
}

# ── Package names per distro ────────────────────────────────────────────────
_pkg_names_legacy() {
    local pkg="$1"
    case "$PKG_MGR" in
        pacman)
            case "$pkg" in
                gcc)        echo "gcc" ;;
                binutils)   echo "binutils" ;;
                make)       echo "make" ;;
                grub)       echo "grub" ;;
                xorriso)    echo "xorriso" ;;
                mtools)     echo "mtools" ;;
                qemu)       echo "qemu-full" ;;
                python)     echo "python" ;;
                nasm)       echo "nasm" ;;
                installer)  echo "gptfdisk parted e2fsprogs dosfstools" ;;
            esac ;;
        apt)
            case "$pkg" in
                gcc)        echo "gcc" ;;
                binutils)   echo "binutils" ;;
                make)       echo "make" ;;
                grub)       echo "grub-pc-bin grub-common xorriso mtools" ;;
                xorriso)    echo "xorriso" ;;
                mtools)     echo "mtools" ;;
                qemu)       echo "qemu-system-x86" ;;
                python)     echo "python3" ;;
                nasm)       echo "nasm" ;;
                installer)  echo "gdisk parted e2fsprogs dosfstools" ;;
            esac ;;
        dnf)
            case "$pkg" in
                gcc)        echo "gcc" ;;
                binutils)   echo "binutils" ;;
                make)       echo "make" ;;
                grub)       echo "grub2-tools xorriso mtools" ;;
                xorriso)    echo "xorriso" ;;
                mtools)     echo "mtools" ;;
                qemu)       echo "qemu-system-x86" ;;
                python)     echo "python3" ;;
                nasm)       echo "nasm" ;;
                installer)  echo "gdisk parted e2fsprogs dosfstools" ;;
            esac ;;
        zypper)
            case "$pkg" in
                gcc)        echo "gcc" ;;
                binutils)   echo "binutils" ;;
                make)       echo "make" ;;
                grub)       echo "grub2-x86_64 xorriso mtools" ;;
                xorriso)    echo "xorriso" ;;
                mtools)     echo "mtools" ;;
                qemu)       echo "qemu-x86" ;;
                python)     echo "python3" ;;
                nasm)       echo "nasm" ;;
                installer)  echo "gptfdisk parted e2fsprogs dosfstools" ;;
            esac ;;
        xbps)
            case "$pkg" in
                gcc)        echo "gcc" ;;
                binutils)   echo "binutils" ;;
                make)       echo "make" ;;
                grub)       echo "grub" ;;
                xorriso)    echo "xorriso" ;;
                mtools)     echo "mtools" ;;
                qemu)       echo "qemu" ;;
                python)     echo "python3" ;;
                nasm)       echo "nasm" ;;
                installer)  echo "gptfdisk parted e2fsprogs dosfstools" ;;
            esac ;;
        apk)
            case "$pkg" in
                gcc)        echo "gcc" ;;
                binutils)   echo "binutils" ;;
                make)       echo "make" ;;
                grub)       echo "grub grub-efi xorriso mtools" ;;
                xorriso)    echo "xorriso" ;;
                mtools)     echo "mtools" ;;
                qemu)       echo "qemu-system-x86_64" ;;
                python)     echo "python3" ;;
                nasm)       echo "nasm" ;;
                installer)  echo "gptfdisk parted e2fsprogs dosfstools" ;;
            esac ;;
        emerge)
            case "$pkg" in
                gcc)        echo "sys-devel/gcc" ;;
                binutils)   echo "sys-libs/binutils" ;;
                make)       echo "sys-devel/make" ;;
                grub)       echo "sys-boot/grub dev-libs/xorriso dev-util/mtools" ;;
                xorriso)    echo "dev-libs/xorriso" ;;
                mtools)     echo "dev-util/mtools" ;;
                qemu)       echo "app-emulation/qemu" ;;
                python)     echo "dev-lang/python" ;;
                nasm)       echo "dev-lang/nasm" ;;
                installer)  echo "sys-apps/util-linux sys-apps/parted sys-fs/e2fsprogs sys-fs/dosfstools" ;;
            esac ;;
        nix)
            case "$pkg" in
                gcc)        echo "gcc" ;;
                binutils)   echo "binutils" ;;
                make)       echo "gnumake" ;;
                grub)       echo "grub" ;;
                xorriso)    echo "xorriso" ;;
                mtools)     echo "mtools" ;;
                qemu)       echo "qemu_full" ;;
                python)     echo "python3" ;;
                nasm)       echo "nasm" ;;
                installer)  echo "gptfdisk parted e2fsprogs dosfstools" ;;
            esac ;;
        pkg)
            case "$pkg" in
                gcc)        echo "gcc" ;;
                binutils)   echo "binutils" ;;
                make)       echo "gmake" ;;
                grub)       echo "grub2 xorriso mtools" ;;
                xorriso)    echo "xorriso" ;;
                mtools)     echo "mtools" ;;
                qemu)       echo "qemu" ;;
                python)     echo "python3" ;;
                nasm)       echo "nasm" ;;
                installer)  echo "gptfdisk parted e2fsprogs dosfstools" ;;
            esac ;;
        brew)
            case "$pkg" in
                gcc)        echo "gcc" ;;
                binutils)   echo "binutils" ;;
                make)       echo "make" ;;
                grub)       echo "grub" ;;
                xorriso)    echo "xorriso" ;;
                mtools)     echo "mtools" ;;
                qemu)       echo "qemu" ;;
                python)     echo "python3" ;;
                nasm)       echo "nasm" ;;
                installer)  echo "gptfdisk e2fsprogs dosfstools" ;;
            esac ;;
    esac
}

# Names which are not identical across distributions.  These are kept
# separate from the legacy native-package table above so adding a target
# architecture cannot accidentally change the x86_64 package set.
pkg_names() {
    local pkg="$1"
    case "$pkg:$PKG_MGR" in
        gcc:termux) echo "clang" ;;
        binutils:termux) echo "binutils" ;;
        make:termux) echo "make" ;;
        nasm:termux) echo "nasm" ;;
        xorriso:termux) echo "xorriso" ;;
        mtools:termux) echo "mtools" ;;
        qemu:termux) echo "qemu-system-x86-64-headless" ;;
        python:termux) echo "python" ;;
        grub:termux) echo "" ;;
        installer:termux) echo "" ;;
        rust:termux) echo "rust" ;;
        qemu_arm:termux) echo "qemu-system-arm-headless qemu-system-aarch64-headless" ;;
        cross_aarch64:termux|cross_armv7:termux) echo "" ;;

        rust:pacman) echo "rust" ;;
        rust:apt) echo "rustc cargo" ;;
        rust:dnf) echo "rust cargo" ;;
        rust:zypper) echo "rust cargo" ;;
        rust:xbps) echo "rust cargo" ;;
        rust:apk) echo "rust cargo" ;;
        rust:emerge) echo "dev-lang/rust" ;;
        rust:nix) echo "rustc cargo" ;;
        rust:pkg) echo "rust" ;;
        rust:brew) echo "rust" ;;

        cross_aarch64:apt) echo "gcc-aarch64-linux-gnu binutils-aarch64-linux-gnu" ;;
        cross_armv7:apt) echo "gcc-arm-linux-gnueabihf binutils-arm-linux-gnueabihf" ;;
        qemu_arm:apt) echo "qemu-system-arm" ;;

        cross_aarch64:dnf) echo "gcc-aarch64-linux-gnu binutils-aarch64-linux-gnu" ;;
        cross_armv7:dnf) echo "gcc-arm-linux-gnueabihf binutils-arm-linux-gnu" ;;
        qemu_arm:dnf) echo "qemu-system-arm" ;;

        cross_aarch64:zypper) echo "cross-aarch64-gcc cross-aarch64-binutils" ;;
        cross_armv7:zypper) echo "cross-arm-linux-gnueabihf-gcc cross-arm-linux-gnueabihf-binutils" ;;
        qemu_arm:zypper) echo "qemu-arm" ;;

        cross_aarch64:pacman) echo "aarch64-linux-gnu-gcc aarch64-linux-gnu-binutils" ;;
        cross_armv7:pacman) echo "arm-linux-gnueabihf-gcc arm-linux-gnueabihf-binutils" ;;
        qemu_arm:pacman) echo "qemu-full" ;;
        cross_aarch64:xbps|cross_armv7:xbps) echo "aarch64-linux-gnu-gcc arm-linux-gnueabihf-gcc" ;;
        qemu_arm:xbps) echo "qemu" ;;
        cross_aarch64:apk|cross_armv7:apk) echo "gcc-aarch64-linux-gnu gcc-arm-none-eabi" ;;
        qemu_arm:apk) echo "qemu-system-aarch64 qemu-system-arm" ;;
        cross_aarch64:brew|cross_armv7:brew) echo "aarch64-elf-gcc arm-none-eabi-gcc" ;;
        qemu_arm:brew) echo "qemu" ;;
        cross_aarch64:emerge|cross_armv7:emerge) echo "crossdev" ;;
        qemu_arm:emerge) echo "app-emulation/qemu" ;;
        cross_aarch64:nix|cross_armv7:nix) echo "pkgsCross.aarch64-multiplatform.stdenv.cc" ;;
        qemu_arm:nix) echo "qemu_full" ;;
        cross_aarch64:pkg|cross_armv7:pkg) echo "" ;;
        qemu_arm:pkg) echo "qemu" ;;
        *) _pkg_names_legacy "$pkg" ;;
    esac
}

# ── Dependency definitions ──────────────────────────────────────────────────
declare -A DEP_NAMES
DEP_NAMES=(
    [gcc]="GNU C Compiler (gcc)"
    [binutils]="GNU Binutils (ld, as)"
    [make]="GNU Make"
    [grub]="GRUB + xorriso + mtools (for ISO creation)"
    [xorriso]="xorriso (ISO 9660 manipulation)"
    [mtools]="mtools (FAT filesystem tools)"
    [qemu]="QEMU x86_64 System Emulator"
    [python]="Python 3 (rootfs builder)"
    [nasm]="Netwide Assembler (nasm)"
    [rust]="Rust compiler (rustc)"
    [qemu_arm]="QEMU ARM emulators (qemu-system-arm/aarch64)"
    [cross_aarch64]="AArch64 cross compiler and binutils"
    [cross_armv7]="ARMv7 cross compiler and binutils"
    [installer]="Disk installer tools (sgdisk, partprobe, mkfs.ext2, mkfs.fat)"
)

# Dependencies are categorized for clarity
DEP_LIST_CORE=(gcc binutils make nasm rust)
DEP_LIST_ISO=(grub xorriso mtools)
DEP_LIST_QEMU=(qemu qemu_arm)
DEP_LIST_CROSS=(cross_aarch64 cross_armv7)
DEP_LIST_PY=(python)
DEP_LIST_INSTALLER=(installer)

# ── Check functions ─────────────────────────────────────────────────────────
check_cmd() {
    command -v "$1" &>/dev/null
}

check_gcc()          { check_cmd gcc || { [[ "${PKG_MGR:-}" == termux ]] && check_cmd clang; }; }
check_binutils()     { check_cmd ld && check_cmd as; }
check_make()         { check_cmd make || check_cmd gmake; }
check_nasm()         { check_cmd nasm; }
check_xorriso()      { check_cmd xorriso; }
check_mtools()       { check_cmd mformat; }
check_qemu()         { check_cmd qemu-system-x86_64 || check_cmd qemu-system-x86-64; }
check_qemu_arm()     { check_cmd qemu-system-arm && check_cmd qemu-system-aarch64; }
check_python()       { check_cmd python3 || check_cmd python; }
check_rust()         { check_cmd rustc; }
check_grub()         { check_cmd grub-mkrescue && check_cmd grub-file; }
check_cross_aarch64(){ check_cmd aarch64-linux-gnu-gcc && check_cmd aarch64-linux-gnu-ld; }
check_cross_armv7()  { check_cmd arm-linux-gnueabihf-gcc && check_cmd arm-linux-gnueabihf-ld; }
check_installer()    { check_cmd blkid && check_cmd grub-install && check_cmd mkfs.ext2 && check_cmd mkfs.fat && check_cmd partprobe && check_cmd sgdisk && check_cmd wipefs; }

# ── Install functions ───────────────────────────────────────────────────────
run_as_root() {
    if [[ "${EUID:-$(id -u)}" -eq 0 ]]; then
        "$@"
    elif check_cmd sudo; then
        sudo "$@"
    else
        log_fail "This installation needs root privileges, but sudo is unavailable"
        return 1
    fi
}

install_with_termux() {
    local packages=()
    for p in "$@"; do
        local names
        names=$(pkg_names "$p")
        for n in $names; do
            [[ -n "$n" ]] && packages+=("$n")
        done
    done
    if [[ ${#packages[@]} -eq 0 ]]; then return 0; fi
    pkg update -y
    pkg install -y "${packages[@]}"
}

install_with_pacman() {
    local packages=()
    for p in "$@"; do
        local names
        names=$(pkg_names "$p")
        for n in $names; do
            packages+=("$n")
        done
    done
    if [[ ${#packages[@]} -eq 0 ]]; then return 0; fi
    run_as_root pacman -S --noconfirm --needed "${packages[@]}"
}

install_with_apt() {
    local packages=()
    for p in "$@"; do
        local names
        names=$(pkg_names "$p")
        for n in $names; do
            packages+=("$n")
        done
    done
    if [[ ${#packages[@]} -eq 0 ]]; then return 0; fi
    run_as_root apt-get update -qq
    run_as_root apt-get install -y "${packages[@]}"
}

install_with_dnf() {
    local packages=()
    for p in "$@"; do
        local names
        names=$(pkg_names "$p")
        for n in $names; do
            packages+=("$n")
        done
    done
    if [[ ${#packages[@]} -eq 0 ]]; then return 0; fi
    run_as_root dnf install -y "${packages[@]}"
}

install_with_zypper() {
    local packages=()
    for p in "$@"; do
        local names
        names=$(pkg_names "$p")
        for n in $names; do
            packages+=("$n")
        done
    done
    if [[ ${#packages[@]} -eq 0 ]]; then return 0; fi
    run_as_root zypper --non-interactive install "${packages[@]}"
}

install_with_xbps() {
    local packages=()
    for p in "$@"; do
        local names
        names=$(pkg_names "$p")
        for n in $names; do
            packages+=("$n")
        done
    done
    if [[ ${#packages[@]} -eq 0 ]]; then return 0; fi
    run_as_root xbps-install -Sy "${packages[@]}"
}

install_with_apk() {
    local packages=()
    for p in "$@"; do
        local names
        names=$(pkg_names "$p")
        for n in $names; do
            packages+=("$n")
        done
    done
    if [[ ${#packages[@]} -eq 0 ]]; then return 0; fi
    run_as_root apk add "${packages[@]}"
}

install_with_emerge() {
    local packages=()
    for p in "$@"; do
        local names
        names=$(pkg_names "$p")
        for n in $names; do
            packages+=("$n")
        done
    done
    if [[ ${#packages[@]} -eq 0 ]]; then return 0; fi
    run_as_root emerge -av "${packages[@]}"
}

install_with_nix() {
    local packages=()
    for p in "$@"; do
        local names
        names=$(pkg_names "$p")
        for n in $names; do
            packages+=("-p" "$n")
        done
    done
    if [[ ${#packages[@]} -eq 0 ]]; then return 0; fi
    nix-env -iA nixpkgs.gnumake "${packages[@]}" 2>/dev/null || nix-env -i "${packages[@]}"
}

install_with_pkg() {
    local packages=()
    for p in "$@"; do
        local names
        names=$(pkg_names "$p")
        for n in $names; do
            packages+=("$n")
        done
    done
    if [[ ${#packages[@]} -eq 0 ]]; then return 0; fi
    run_as_root pkg install -y "${packages[@]}"
}

install_with_brew() {
    local packages=()
    for p in "$@"; do
        local names
        names=$(pkg_names "$p")
        for n in $names; do
            packages+=("$n")
        done
    done
    if [[ ${#packages[@]} -eq 0 ]]; then return 0; fi
    brew install "${packages[@]}"
}

do_install() {
    local func="install_with_${PKG_MGR}"
    "$func" "$@"
}

# ── Get list of missing deps ────────────────────────────────────────────────
get_missing() {
    local missing=()
    for dep in "$@"; do
        local func="check_${dep}"
        if ! $func 2>/dev/null; then
            missing+=("$dep")
        fi
    done
    echo "${missing[@]}"
}

# ── Pretty status for a single dep ──────────────────────────────────────────
show_dep_status() {
    local dep="$1"
    local func="check_${dep}"
    local name="${DEP_NAMES[$dep]:-$dep}"
    if $func 2>/dev/null; then
        log_ok "$name"
    else
        log_fail "$name  ${DIM}(not found)${RESET}"
    fi
}

# ── Main ────────────────────────────────────────────────────────────────────
main() {
    banner

    detect_os
    detect_pkg_mgr

    echo -e "  ${INFO}  Platform:   ${W}${OS}${RESET} / ${W}${DISTRO}${RESET}"
    echo -e "  ${INFO}  Pkg Manager:${W} ${PKG_MGR:-unknown}${RESET}"
    if [[ "${PKG_MGR}" == "termux" ]]; then
        echo -e "  ${WARN}  Termux profile: clang replaces gcc; GRUB ISO and GNU ARM cross tools are not available here."
        echo -e "  ${INFO}  For the complete build, use Debian/Ubuntu, WSL or CI."
    fi
    echo ""

    # ── Show status of all deps ─────────────────────────────────────────
    section "Checking Dependencies"

    if [[ "${PKG_MGR}" == "termux" ]]; then
        # Termux can build/check the freestanding sources and run QEMU, but
        # it does not provide the GNU ARM Linux cross toolchains or GRUB ISO
        # stack used by the full Linux build.
        ALL_DEPS=("${DEP_LIST_CORE[@]}" "${DEP_LIST_PY[@]}" "${DEP_LIST_QEMU[@]}")
    else
        ALL_DEPS=("${DEP_LIST_CORE[@]}" "${DEP_LIST_ISO[@]}" "${DEP_LIST_PY[@]}" "${DEP_LIST_QEMU[@]}" "${DEP_LIST_CROSS[@]}")
    fi
    if [[ "${WITH_INSTALLER:-0}" == "1" && "${PKG_MGR}" != "termux" ]]; then
        ALL_DEPS+=("${DEP_LIST_INSTALLER[@]}")
    fi
    # Remove duplicates without word-splitting package names.
    declare -A seen=()
    UNIQUE_DEPS=()
    for dep in "${ALL_DEPS[@]}"; do
        [[ -n "${seen[$dep]:-}" ]] && continue
        seen[$dep]=1
        UNIQUE_DEPS+=("$dep")
    done
    ALL_DEPS=("${UNIQUE_DEPS[@]}")

    for dep in "${ALL_DEPS[@]}"; do
        show_dep_status "$dep"
    done

    # ── Find missing ────────────────────────────────────────────────────
    MISSING=()
    for dep in "${ALL_DEPS[@]}"; do
        local func="check_${dep}"
        if ! $func 2>/dev/null; then
            MISSING+=("$dep")
        fi
    done

    echo ""
    if [[ "${CHECK_ONLY:-0}" == "1" ]]; then
        if [[ ${#MISSING[@]} -eq 0 ]]; then
            log_ok "Dependency check passed"
            return 0
        fi
        log_fail "${#MISSING[@]} dependency group(s) missing"
        return 1
    fi
    if [[ ${#MISSING[@]} -eq 0 ]]; then
        echo -e "  ${CHECK}  ${G}${BOLD}All dependencies are installed!${RESET}"
        echo ""
        echo -e "  ${INFO}  You can now build CortexOS:"
        echo -e "      ${C}make clean && make${RESET}"
        echo -e "      ${C}make iso${RESET}"
        echo -e "      ${C}make run${RESET}"
        echo ""
        return 0
    fi

    echo -e "  ${WARN}  ${Y}${BOLD}${#MISSING[@]} package(s) missing:${RESET}"
    for dep in "${MISSING[@]}"; do
        echo -e "      ${R}•${RESET} ${DEP_NAMES[$dep]:-$dep}"
    done

    # ── Ask to install ──────────────────────────────────────────────────
    echo ""
    if [[ -z "$PKG_MGR" ]]; then
        echo -e "  ${FAIL}  ${R}Could not detect a supported package manager.${RESET}"
        echo -e "  ${INFO}  Install manually:"
        echo -e "      ${W}gcc${RESET}  ${W}binutils${RESET}  ${W}make${RESET}  ${W}xorriso${RESET}  ${W}mtools${RESET}  ${W}python3${RESET}  ${W}qemu-system-x86_64${RESET}  ${W}nasm${RESET}  ${W}gdisk${RESET}  ${W}parted${RESET}  ${W}e2fsprogs${RESET}  ${W}dosfstools${RESET}"
        echo ""
        return 1
    fi

    echo -e "  ${ASK}  ${W}Install missing packages via ${C}${PKG_MGR}${W}?${RESET}"
    echo -e "  ${DIM}     This will run with sudo if needed.${RESET}"

    if ! ask_confirm "Proceed with installation?" "y"; then
        echo ""
        echo -e "  ${INFO}  Skipped. Install manually later."
        echo ""
        return 0
    fi

    # ── Install ─────────────────────────────────────────────────────────
    section "Installing Packages"

    echo -e "  ${ARROW}  Installing all missing dependency groups in one transaction..."
    if do_install "${MISSING[@]}" 2>&1 | while IFS= read -r line; do
        echo -e "     ${DIM}${line}${RESET}"
    done; then
        log_ok "Package installation finished"
    else
        log_fail "Package installation failed"
    fi

    # ── Post-install verify ─────────────────────────────────────────────
    section "Verification"

    local all_ok=true
    for dep in "${MISSING[@]}"; do
        local func="check_${dep}"
        local name="${DEP_NAMES[$dep]:-$dep}"
        if $func 2>/dev/null; then
            log_ok "$name"
        else
            log_fail "$name  ${R}(still missing)${RESET}"
            all_ok=false
        fi
    done

    echo ""
    if $all_ok; then
        echo -e "  ${CHECK}  ${G}${BOLD}All dependencies installed successfully!${RESET}"
    else
        echo -e "  ${WARN}  ${Y}Some packages failed. Check errors above.${RESET}"
        echo -e "  ${INFO}  Revisa los nombres de paquete de tu distribución y vuelve a ejecutar --check."
    fi
    echo ""
    echo -e "  ${INFO}  Build CortexOS:"
    echo -e "      ${C}make clean && make${RESET}"
    echo -e "      ${C}make iso${RESET}"
    echo -e "      ${C}make run${RESET}"
    echo ""
}

# ── Options ─────────────────────────────────────────────────────────────────
AUTO_YES=0
CHECK_ONLY=0
WITH_INSTALLER=0
for arg in "$@"; do
    case "$arg" in
        -y|--yes) AUTO_YES=1 ;;
        --check) CHECK_ONLY=1 ;;
        --with-installer) WITH_INSTALLER=1 ;;
        -h|--help) SHOW_HELP=1 ;;
        *) echo "Unknown option: $arg" >&2; exit 2 ;;
    esac
done

if [[ "${SHOW_HELP:-0}" == "1" ]]; then
    echo "Usage: $0 [--yes] [--check] [--with-installer] [--help]"
    echo ""
    echo "Options:"
    echo "  -y, --yes          Skip prompts and install missing build dependencies"
    echo "  --check            Only check dependencies; exit 1 when something is missing"
    echo "  --with-installer   Include disk/partition tools (not needed for a normal build)"
    echo "  -h, --help         Show this help"
    echo ""
    echo "Supported platforms:"
    echo "  Arch, Manjaro, EndeavourOS, Garuda"
    echo "  Ubuntu, Debian, Linux Mint, Pop!_OS, Kali, Zorin"
    echo "  Fedora, openSUSE, SLES"
    echo "  Void Linux, Alpine, Gentoo, NixOS"
    echo "  FreeBSD, macOS (Homebrew)"
    echo "  Windows (MSYS2, Git Bash, WSL)"
    echo "  Termux (build/QEMU profile; complete ISO build requires Linux/WSL)"
    exit 0
fi

if [[ "${AUTO_YES:-}" == "1" ]]; then
    # Override ask_confirm to always return true
    ask_confirm() { return 0; }
fi

main "$@"
