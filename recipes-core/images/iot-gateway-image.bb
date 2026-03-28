require recipes-core/images/core-image-minimal.bb

SUMMARY = "IoT Sensor Gateway minimal image for Raspberry Pi 4"

IMAGE_INSTALL:append = " \
    sensor-gateway \
    sensor-gateway-init \
    openssh \
    i2c-tools \
    spi-tools \
    ca-certificates \
    htop \
    tcpdump \
"

# Enable systemd
DISTRO_FEATURES:append = " systemd"
VIRTUAL-RUNTIME_init_manager = "systemd"

IMAGE_FEATURES += "ssh-server-openssh"
IMAGE_ROOTFS_EXTRA_SPACE = "524288"  # 512 MB extra

# Strip debug symbols in production
IMAGE_INSTALL:append = " packagegroup-core-boot"
