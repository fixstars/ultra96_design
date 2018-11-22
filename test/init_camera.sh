#!/bin/sh

mkdir /config/device-tree/overlays/fpga
dtc -I dts -O dtb -o fpga-load.dtb fpga-load.dts
cp fpga-load.dtb /config/device-tree/overlays/fpga/dtbo

mkdir /config/device-tree/overlays/fclk0
dtc -I dts -O dtb -o fclk0-zynqmp.dtb fclk0-zynqmp.dts
cp fclk0-zynqmp.dtb /config/device-tree/overlays/fclk0/dtbo

mkdir /config/device-tree/overlays/udmabuf0
dtc -I dts -O dtb -o udmabuf0.dtb udmabuf0.dts
cp udmabuf0.dtb /config/device-tree/overlays/udmabuf0/dtbo


