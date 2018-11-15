#!/bin/sh

cat << EOS > /tmp/design.bif 
all:
{
    [destination_device = pl] ultra96_design/ultra96_design.runs/impl_1/design_1_wrapper.bit
}
EOS
bootgen -image /tmp/design.bif -arch zynqmp -w -o fpga.bin
