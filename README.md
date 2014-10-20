ip0x_urjtag-0.10
=================

urjtag-0.10  hack to add support for ip0x board

based on hack of jtagprog => jtagprog-bf532-ip04.tgz available on the net...
I hack this package due to lack of paralel port on my dev. machines and I need to flash my old IP0x u-boot.
I used urjtag-0.10 as the source that has support new jtag probe that use USB.
I test using USB JTAG gnICE v-1.0


here is the comment that I used (copied from David Rowe blog's)

cable gnIce

detect

initbus bf532_bf1

spidetectflash 0

spieraseflash 0 4

spiflashmem 0 u-boot.ldr
