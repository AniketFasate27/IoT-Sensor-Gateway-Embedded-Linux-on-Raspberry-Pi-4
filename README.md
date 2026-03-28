<div align="center">

```
██╗ ██████╗ ████████╗    ███████╗███████╗███╗   ██╗███████╗ ██████╗ ██████╗      ██████╗ ██╗    ██╗
██║██╔═══██╗╚══██╔══╝    ██╔════╝██╔════╝████╗  ██║██╔════╝██╔═══██╗██╔══██╗    ██╔════╝ ██║    ██║
██║██║   ██║   ██║       ███████╗█████╗  ██╔██╗ ██║███████╗██║   ██║██████╔╝    ██║  ███╗██║ █╗ ██║
██║██║   ██║   ██║       ╚════██║██╔══╝  ██║╚██╗██║╚════██║██║   ██║██╔══██╗    ██║   ██║██║███╗██║
██║╚██████╔╝   ██║       ███████║███████╗██║ ╚████║███████║╚██████╔╝██║  ██║    ╚██████╔╝╚███╔███╔╝
╚═╝ ╚═════╝    ╚═╝       ╚══════╝╚══════╝╚═╝  ╚═══╝╚══════╝ ╚═════╝ ╚═╝  ╚═╝     ╚═════╝  ╚══╝╚══╝
```

### 🌡️ *Your Pi sniffs the air. The cloud listens. The watchdog never sleeps.* 🐕

<br/>

[![Yocto](https://img.shields.io/badge/Yocto-Kirkstone-blueviolet?style=for-the-badge&logo=linux&logoColor=white)](https://www.yoctoproject.org/)
[![Platform](https://img.shields.io/badge/Target-Raspberry%20Pi%204B-cc0000?style=for-the-badge&logo=raspberry-pi&logoColor=white)](https://www.raspberrypi.com/)
[![Cloud](https://img.shields.io/badge/Cloud-AWS%20IoT%20Core-FF9900?style=for-the-badge&logo=amazon-aws&logoColor=white)](https://aws.amazon.com/iot-core/)
[![Language](https://img.shields.io/badge/Language-C11-00599C?style=for-the-badge&logo=c&logoColor=white)](https://en.wikipedia.org/wiki/C11_(C_standard_revision))
[![License](https://img.shields.io/badge/License-MIT-22c55e?style=for-the-badge)](LICENSE)
[![Build](https://img.shields.io/badge/Build-BitBake-f97316?style=for-the-badge)](https://docs.yoctoproject.org/bitbake/)

</div>

---

## 🤔 What even is this?

A **fully custom Embedded Linux image** baked with Yocto that turns a Raspberry Pi 4 into a production-grade IoT sensor gateway. It reads **temperature, humidity, pressure** (BME280 over I²C) and **8 analog channels** (MCP3008 over SPI), wraps it all in JSON, and fires it at **AWS IoT Core** over MQTT/TLS — every 10 seconds, forever, without complaining.

Oh, and a **hardware watchdog** reboots the whole thing if the daemon ever freezes. Cold. Ruthless. Reliable. 

> **"Why not just use MicroPython on an Arduino?"**
> Because we're adults who enjoy pain, and also because you can't run `systemd`, kernel drivers, or TLS on an Arduino. 🙂

---

## ✨ Features at a glance

| Feature | Details |
|---|---|
| 🏗️ **Custom Yocto Image** | Kirkstone, minimal rootfs, systemd init |
| 🌡️ **BME280 Sensor** | Temp / Humidity / Pressure over I²C with full Bosch compensation |
| 📊 **MCP3008 ADC** | 8-channel 10-bit analog reads over SPI |
| ☁️ **AWS IoT Core** | MQTT/TLS on port 8883, per-device cert auth |
| 🔒 **Secure by default** | Dedicated service user, `NoNewPrivileges`, `chmod 600` keys |
| 🐕 **Hardware Watchdog** | Armed at boot, kicks every cycle, reboots on hang |
| 📦 **BitBake recipe** | CMake build, auto device-ID injection, syslog integration |
| 🔌 **Device Tree Overlay** | Proper `.dts` for I²C + SPI nodes — no `/boot/config.txt` hacks |

---

## 🏛️ Architecture

```
                         ┌─────────────────────────────────────────┐
                         │         Raspberry Pi 4B                 │
                         │         (Yocto Kirkstone)               │
                         │                                         │
  BME280                 │  ┌───────────────────────────────────┐  │
  Temp/Hum/Press ──I²C──►│  │                                   │  │
                         │  │       sensor-gateway daemon        │  │
  MCP3008                │  │            (C11 app)               │  │
  8-ch ADC     ───SPI───►│  │                                   │  │
                         │  │  read → compensate → JSON → TLS   │  │
                         │  └───────────────┬───────────────────┘  │
                         │                  │ MQTT/TLS :8883        │
                         └──────────────────┼──────────────────────┘
                                            │
                               ┌────────────▼────────────┐
                               │      AWS IoT Core        │
                               │  gw/{device-id}/telm     │
                               └────────────┬────────────┘
                                            │ IoT Rules Engine
                              ┌─────────────┼─────────────┐
                              ▼             ▼             ▼
                          DynamoDB         S3           SNS Alert
```

---

## 🧰 Hardware you need

```
  ┌─────────────────┐      ┌─────────────────┐      ┌─────────────────┐
  │  Raspberry Pi   │      │    BME280        │      │    MCP3008      │
  │      4B         │      │  (I²C, 0x76)    │      │  (SPI, CE0)     │
  │                 │      │                 │      │                 │
  │  GPIO2  ────────┼──SDA─┤                 │      │                 │
  │  GPIO3  ────────┼──SCL─┤                 │      │                 │
  │  GPIO8  ────────┼──────┼─────────────────┼─CS───┤                 │
  │  GPIO9  ────────┼──────┼─────────────────┼─MISO─┤                 │
  │  GPIO10 ────────┼──────┼─────────────────┼─MOSI─┤                 │
  │  GPIO11 ────────┼──────┼─────────────────┼─CLK──┤                 │
  │  3.3V   ────────┼──VCC─┤                 │ VCC──┤                 │
  │  GND    ────────┼──GND─┤                 │ GND──┤                 │
  └─────────────────┘      └─────────────────┘      └─────────────────┘
```

---

## 📂 Repo layout

```
meta-iot-gateway/
│
├── 📁 conf/
│   └── layer.conf                        ← Yocto layer config
│
├── 📁 recipes-app/sensor-gateway/
│   ├── sensor-gateway_1.0.bb             ← BitBake recipe (the main event)
│   └── files/
│       ├── sensor_gateway.c              ← Main daemon loop
│       ├── bme280.c / .h                 ← I²C driver (Bosch algorithm)
│       ├── mcp3008.c / .h                ← SPI ADC driver
│       ├── mqtt_client.c / .h            ← Paho MQTT/TLS wrapper
│       ├── config.h                      ← Build-time config knobs
│       └── CMakeLists.txt
│
├── 📁 recipes-kernel/
│   └── linux-raspberrypi_%.bbappend      ← I²C + SPI + WDT kernel config
│
├── 📁 recipes-bsp/device-tree/
│   └── sensor-gateway-overlay.dts        ← Device Tree: I²C + SPI nodes
│
├── 📁 recipes-core/images/
│   └── iot-gateway-image.bb              ← Custom minimal image recipe
│
└── 📁 recipes-system/sensor-gateway-init/
    ├── sensor-gateway.service            ← Systemd unit (hardened)
    └── sensor-gateway.conf               ← Runtime env overrides
```

---

## 🚀 Build it

> ⚠️ Grab a coffee. Yocto's first build takes a while. Like, *a long* while.

```bash
# 1. Pull Poky (Kirkstone) + BSP layers
git clone -b kirkstone git://git.yoctoproject.org/poky.git
cd poky
git clone -b kirkstone git://git.yoctoproject.org/meta-raspberrypi.git
git clone https://github.com/YOUR_USERNAME/meta-iot-gateway.git

# 2. Set up the build environment
source oe-init-build-env build

# 3. Drop this into conf/bblayers.conf
#    (add meta-raspberrypi and meta-iot-gateway to BBLAYERS)

# 4. Add to conf/local.conf
echo 'MACHINE = "raspberrypi4-64"'          >> conf/local.conf
echo 'ENABLE_I2C = "1"'                     >> conf/local.conf
echo 'ENABLE_SPI_BUS = "1"'                 >> conf/local.conf
echo 'RPI_EXTRA_CONFIG = "dtoverlay=sensor-gateway"' >> conf/local.conf

# 5. Bake 🍞
bitbake iot-gateway-image
```

**Output lands here:**
```
tmp/deploy/images/raspberrypi4-64/iot-gateway-image-raspberrypi4-64.wic.bz2
```

---

## ☁️ AWS setup (quick version)

```bash
# Create a Thing
aws iot create-thing --thing-name rpi4-gw-001

# Generate certs
aws iot create-keys-and-certificate --set-as-active \
    --certificate-pem-outfile device.crt \
    --private-key-outfile device.key

# Grab root CA
curl -o AmazonRootCA1.pem \
    https://www.amazontrust.com/repository/AmazonRootCA1.pem
```

MQTT topic your data lands on: **`gw/rpi4-gw-001/telemetry`**

Payload looks like this every 10 seconds:
```json
{
  "device_id": "rpi4-gw-001",
  "timestamp": "2025-11-14T08:32:01Z",
  "fw_version": "1.0.0",
  "environment": {
    "temperature_c": 24.37,
    "humidity_pct": 58.12,
    "pressure_hpa": 1013.25
  },
  "adc_channels": [
    { "channel": 0, "raw": 512, "voltage": 1.65 },
    { "channel": 1, "raw": 204, "voltage": 0.66 }
  ]
}
```

---

## 🔐 Security posture

```
🔑  device.key          chmod 600, owned by sensor-gw user only
👤  sensor-gw user      no shell, no sudo, no home dir
🛡️  systemd hardening   NoNewPrivileges + ProtectSystem=strict
📜  per-device policy   AWS IoT policy scoped to one Thing only
🐕  hardware watchdog   reboots device if daemon freezes for >30s
🔒  TLS everywhere      port 8883, mutual cert auth, no plain MQTT
```

---

## 🔧 After flashing

```bash
# Flash the image
bunzip2 iot-gateway-image-raspberrypi4-64.wic.bz2
sudo dd if=iot-gateway-image-raspberrypi4-64.wic of=/dev/sdX bs=4M status=progress

# Copy certs before first boot
sudo cp AmazonRootCA1.pem device.crt device.key \
    /mnt/rootfs/etc/sensor-gateway/certs/

# SSH in and check the service
ssh root@<device-ip>
systemctl status sensor-gateway
journalctl -u sensor-gateway -f
```

---

## 🗺️ Roadmap

- [ ] 🔄 **SWUpdate OTA** — A/B partition delta updates over the air  
- [ ] 🤖 **TFLite inference** — on-device anomaly detection on sensor streams  
- [ ] 📴 **Offline buffer** — SQLite ring buffer when network drops  
- [ ] 💨 **SGP30 sensor** — CO₂ / TVOC on the same I²C bus  
- [ ] 📈 **Grafana dashboard** — AWS IoT → Timestream → live graphs  
- [ ] 🏭 **Fleet provisioning** — zero-touch AWS onboarding at scale  

---

## 🤝 Contributing

PRs welcome. Open an issue first for big changes. Please don't push directly to `main` — the watchdog is watching. 🐕

---

## 📄 License

MIT © 2025. Do whatever you want with it, just don't blame me if your Pi catches fire. (It won't. But still.)

---

<div align="center">

**Built with 🧠 C, ☕ caffeine, and an unreasonable love for BitBake.**

*If this saved you hours of Yocto suffering, consider starring the repo ⭐*

</div>
