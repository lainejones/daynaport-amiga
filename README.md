# Amiga DaynaPORT Driver for BlueSCSI V2 and ZuluSCSI
Created by RobSmithDev

## About this fork

This is a fork of [RobSmithDev/daynaport-amiga](https://github.com/RobSmithDev/daynaport-amiga)
(the driver is Rob Smith's; see the credits below). It carries a few fixes and one feature that are
not in the original yet. Releases of the fork are on this repository's Releases page; the file is
`scsidayna.device`, copied to `DEVS:Networks/`, with the same `scsidayna.prefs` as the original plus
one optional key. The fork has been running on an Amiga 500 (GVP A530, BlueSCSI v2) and an Amiga 1200
(DataFlyer SCSI+, ZuluSCSI) for months; it is built for the 68000 and runs on every CPU.

- **1.8** (2026-10-22): after a TCP/IP stack cycled the unit `S2_OFFLINE` then `S2_ONLINE` to reset it
  (PrismNet does that when a write stalls), the driver refused every read and write "out of service"
  until the device was closed and reopened; the network was dead for good. The packet task now
  republishes its state on every pass and raises `S2EVENT_ONLINE` when the gates reopen that way.
- **1.7** (2026-10-19): `S2_ADDMULTICASTADDRESS` sent the wrong transfer length to the adapter
  (asked for 1536 bytes), which some SCSI drivers reported as an "unexpected status" requester.
  Sent upstream as [pull request 18](https://github.com/RobSmithDev/daynaport-amiga/pull/18).
- **1.6** (2026-10-19): multicast. `S2_ADDMULTICASTADDRESS` / `S2_DELMULTICASTADDRESS` /
  `S2_MULTICAST` are implemented and, with `MULTICAST=1` in `scsidayna.prefs`, the groups are given
  to the adapter, so mDNS names (`amiga.local`) resolve. Without the key the driver behaves as before.
- **2026-08**: crashes and races on shutdown fixed: `S2_OFFLINE` terminates every pending request
  before it returns (a stack could free its buffers while requests were still queued), the flush walked
  a list it was modifying, `AbortIO` searched the wrong lists, `CloseDevice` leaked its port and buffer
  management, the batch send wrote a word to an odd address (an address error on a 68000), the WiFi
  key no longer appears in the debug log, and a few more (see the commit history).

Original README follows.


This is an implementation of a SANA-II driver for the Amiga, which allows you to use either a PicoW enabled BlueSCSI V2, or various WiFi equipped ZuluSCSI models to give internet access to the machine!

This code was originally based on the MNT ZZ9000Net driver by Lukas F. Hartmann (which is based on work by Henryk Richter) and also borrows a little from the PlipBox device driver, which has several contributors.

### NEW Setup Information
This will work with the older firmware, (BlueSCSI v2024.10.26, ZuluSCSI v2024.03.07)
**however** To use the new BATCH transfer speed for increased speed:
- Create or rename your NE4 file to AM4 - "AM" is for "Amiga"
- Optionally play with some of the new settings in the prefs file, eg: DATASIZE
- TIP: Actually performs better with priority set to 0!

### OLD!!!! Setup Guides:
- [YouTube video by Retronaut](https://www.youtube.com/watch?v=FDtqd04bq-k)
- [BlueSCSI Docs](https://github.com/blueSCSI/blueSCSI-v2/wiki/WiFi-Amiga)
- [Discord](https://discord.gg/pQsU3CR9fe)
- [Workbench GUI Config Tool](https://github.com/AidanHolmes/BlueSCSIUI/releases/)

### SCSI Hardware Requirements:
- [BlueSCSI Pico (V2) Wi-fi version](https://bluescsi.com/docs/WiFi-DaynaPORT) (v2024.10.26 or later firmware)
- [ZuluSCSI Pico OSHW board with Pico W](https://zuluscsi.com/oshw/)  (v2024.03.07 or later firmware)
- [ZuluSCSI Pico Slim (DB25)](https://shop.rabbitholecomputing.com/products/zuluscsi-rp2040-pico-slim)
- [ZuluSCSI Blaster (RP2350B)](https://zuluscsi.com) with RM2 WiFi module

Some Tested Configurations:

### Amiga 500/+ (2M Chip)
- A590 HDD (2M Fast RAM, 7.0 ROM) with Kickstart 3.2 (scsi.device)
- Ematrix 530 (ematscsi.device)

### Amiga 1200
- Blizzard 1230IV / SCSI Kit (1230scsi.device)

### Amiga 2000 (2M Chip)
- GVP Impact A2000-HC+8 Series II (8M Fast RAM), Kickstart 3.1 (gvpscsi.device) - MODE=2
- GVP Impact A2000-HC+8 Series II + Guru ROM 6.14 - MODE=0 
- A2091, Kickstart 3.1 (scsi.device) - MODE=1
- phase5 Blizzard 2060 SCSI, Kickstart 3.1 (2060scsi.device)
	
#### Side effects
With BlueSCSI V2 hardware, the HDD light constantly flashes while the driver is in use.
The driver needs to be copied to the `DEVS:Networks` folder and then setup your TCP/IP stack as normal.

#### Notes
If you find your data transfer is *very* slow, like less than 5K/s then check you've turned the debug log off within your BlueSCSI or ZuluSCSI configuration!

### Config File (IMPORTANT)
`scsidayna.prefs` contains an example config file for the device. This needs to be copied to `ENVARC:` on the Amiga and rebooted. 
**If you change this file, they will not be picked up until restart or you copy it to ENV:**
You can also manage this file with the [Workbench GUI config tool by Aidan Holmes](https://github.com/AidanHolmes/BlueSCSIUI/releases/).

The format of that file is:
```
DEVICE=scsi.device
DEVICEID=-1
PRIORITY=0
MODE=1
AUTOCONNECT=0
SSID=
KEY=
DATASIZE=
DEBUG=
AGGRESSIVE=
```

where:
- DEVICE is the name of the SCSI driver, eg: scsi.device or gvpscsi.device
- DEVICEID is the SCSI device index the DaynaPORT is on, or -1 for Auto Detect
- PRIORITY -128 to 127, sets the I/O task priority, see below
- MODE see below
- AUTOCONNECT 0/1 if 1, the driver will attempt to connect to the WIFI device (you can also configure BlueSCSI or ZuluSCSI to do this)
- SSID The SSID/Wifi name to connect to if autoconnect=1
- KEY the wifi key/password
- DATASIZE With the new Scsi firmware, you can bulk-transfer packet data upto this amount for increased speed (defaults to 8192, some devices might not support different sizes)
- DEBUG 0/1 Causes a console window to appear to help debug issues with the driver, disable when sorted or it slows things down
- MULTICAST 0/1 (fork, 1.6+) Gives the multicast groups a TCP/IP stack asks for to the adapter, so mDNS names resolve. Default 0.
- AGGRESSIVE 0/1 Enables agressive mode, driver run without 1ms wait - experimental - may scale with processor/priority

## Mode
This patches around weirdness in the various SCSI drivers. Mode should be:
- 0: This runs in normal mode
- 1: Runs in 24-byte pad mode (required for scsi.device - A590/A2091)
- 2: Runs in 'single transfer' mode (required for gvpscsi.device)

## DEVICE
This needs to match the SCSI interface you're using. You can check this using HDToolbox (see what device it uses in the tool type) or SCSIMounter etc.

## Task Priority
A small note about task priority. 

- With the new driver, leave this at zero as it performs better!
- With the original driver, left at 0 the device will function perfectly fine, however the throughput of data is somewhat all over the place. For stable throughput, then set this to '1', but also expect this will possibly slow down some of the other applications running on your system.
