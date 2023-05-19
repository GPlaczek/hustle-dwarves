#!/bin/bash

set -e

usage() {
    printf "%s [-h] [-i n] [-t]\n\n" "$(basename "$0")"
    printf "where:\n"
    printf "  -h\tprint help\n"
    printf "  -i\tnumber of nodes to start\n"
    printf "  -t\ttear down instead of setting up\n"
    printf "  -s\topen network namespace shell\n"
}

while getopts "i:ths" arg; do
    case $arg in
        i)
            N_NODES=$OPTARG ;;
        t)
            TEARDOWN=1 ;;
        s)
            SHELL_=1 ;;
        h)
            usage
            exit 0
            ;;
        *)
            usage
            exit 1
            ;;
    esac
done

if [ "$SHELL_" ] && [ "$TEARDOWN" ]; then
    usage
    exit 1
fi

TAP_PREFIX="husdw"
BRIDGE_NAME="husdwbr0"

WORK_DIR="$(pwd)/work"
CONF_DIR="$(pwd)/conf"

IMG_DIR="$WORK_DIR/img"

PIDFILES_DIR="$WORK_DIR/pidfiles"
DNSMASQ_PIDFILE_DIR="$PIDFILES_DIR/dnsmasq"
QEMU_PIDFILE_DIR="$PIDFILES_DIR/qemu"

OS_IMG_URL="https://cloud.debian.org/images/cloud/bullseye/20230515-1381/debian-11-nocloud-amd64-20230515-1381.qcow2"

IMAGE_NAME="$IMG_DIR/img.qcow2"
NETWORK_NSPACE="husdwns0"
VETH_NS="husdwveth0"
VETH_BR="husdwveth1"

NSPACE_IP="192.168.1.1"

dl_image() {
    tmp_mount="$WORK_DIR/nbdm"

    mkdir -p "$IMG_DIR"
    mkdir -p "$tmp_mount"
    wget "$OS_IMG_URL" -O "$IMAGE_NAME"

    trap 'umount $tmp_mount; qemu-nbd --disconnect /dev/nbd0; rmmod nbd' RETURN
    modprobe nbd max_part=1
    qemu-nbd --connect /dev/nbd0 "$IMAGE_NAME"
    sleep 1
    mount /dev/nbd0p1 "$tmp_mount"

    cp "$CONF_DIR/ssh.service" "$tmp_mount/lib/systemd/system/ssh.service"
    cp "$CONF_DIR/root.conf" "$tmp_mount/etc/ssh/sshd_config.d"
    cp "$CONF_DIR/interfaces" "$tmp_mount/etc/network/interfaces"
}

create_network() {
    mkdir -p "$WORK_DIR"
    mkdir -p "$DNSMASQ_PIDFILE_DIR"

    ip link add name "$BRIDGE_NAME" type bridge nf_call_iptables 0 || true
    ip link set "$BRIDGE_NAME" up

    for i in $(seq 1 "$N_NODES"); do
        tap_name="${TAP_PREFIX}$i"
        ip tuntap add "$tap_name" mode tap
        brctl addif "$BRIDGE_NAME" "$tap_name"
        ip link set "$tap_name" up
    done

    ip netns add "$NETWORK_NSPACE"
    ip link add "$VETH_NS" type veth peer "$VETH_BR"

    ip link set "$VETH_NS" netns "$NETWORK_NSPACE"
    ip link set "$VETH_BR" master "$BRIDGE_NAME"

    ip netns exec "$NETWORK_NSPACE" ip link set lo up
    ip netns exec "$NETWORK_NSPACE" ip addr add "$NSPACE_IP"/24 dev "$VETH_NS"

    ip link set "$VETH_BR" up
    ip netns exec "$NETWORK_NSPACE" ip link set "$VETH_NS" up

    ip netns exec "$NETWORK_NSPACE" dnsmasq -C "$CONF_DIR"/dnsmasq.conf --pid-file="$DNSMASQ_PIDFILE_DIR/dnsmasq.pid"

    iptables -A FORWARD -m physdev --physdev-is-bridged -j ACCEPT
}

teardown_network() {
    ip link set "${BRIDGE_NAME}" down
    ip link set "${VETH_BR}" down

    ip link del "${BRIDGE_NAME}"
    ip link del "${VETH_BR}"

    ip netns del "${NETWORK_NSPACE}"
    for iface in $(ip addr | cut -d' ' -f2 | grep -oe 'husdw[0-9]'); do
        ip link del "$iface"
    done

    kill -9 "$(cat "$DNSMASQ_PIDFILE_DIR/dnsmasq.pid")"
    rm -f "$DNSMASQ_PIDFILE_DIR/dnsmasq.pid"
}

stop_qemu() {
    for pidfile in "$QEMU_PIDFILE_DIR"/*; do
        kill -9 "$(cat "$pidfile")"
        rm -f "$pidfile"
    done
    rm -rf "$IMG_DIR"/husdw*
}

run_qemu() {
    mkdir -p "$QEMU_PIDFILE_DIR"

    for i in $(seq 1 "$N_NODES"); do
        pidfile="$QEMU_PIDFILE_DIR/vm$i.pid"
        tap_name="${TAP_PREFIX}$i"
        img_overlay="$IMG_DIR/husdw$i.qcow2"
        mac_addr=de:ad:ca:fe:00:0$i

        qemu-img create -F qcow2 -f qcow2 -b "$IMAGE_NAME" "$img_overlay"
        echo "$tap_name"

        qemu-system-x86_64 \
            -smp 2 \
            -m 2G \
            -drive format=qcow2,file="$img_overlay" \
            -enable-kvm \
            -pidfile "$pidfile" \
            -netdev tap,id=lan,ifname="$tap_name",script=no,downscript=no \
            -device virtio-rng \
            -device virtio-net-pci,netdev=lan,mac="$mac_addr" \
            -vga none \
            -display none \
            -daemonize
    done
}

if [ "$TEARDOWN" ]; then
    teardown_network || true
    stop_qemu || true
elif [ "$SHELL_" ]; then
    ip netns exec "$NETWORK_NSPACE" "$SHELL"
else
    [ ! -f "$IMAGE_NAME" ] && dl_image
    create_network || true
    run_qemu
fi
