#!/bin/sh

mkdir /config/device-tree/overlays/fpga
dtc -I dts -O dtb -o fpga-load.dtb fpga-load.dts
cp fpga-load.dtb /config/device-tree/overlays/fpga/dtbo

mkdir /config/device-tree/overlays/fclk0
dtc -I dts -O dtb -o fclk0-zynqmp.dtb fclk0-zynqmp.dts
cp fclk0-zynqmp.dtb /config/device-tree/overlays/fclk0/dtbo

mkdir /config/device-tree/overlays/v4l2
dtc -I dts -O dtb -o v4l2.dtb v4l2.dts
cp v4l2.dtb /config/device-tree/overlays/v4l2/dtbo

./caminit
