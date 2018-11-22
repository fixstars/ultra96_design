#!/bin/bash

TARGET=demosaic
SUFFIX=
GENERATOR=${TARGET}_generator.cc
TESTCODE=${TARGET}_test.cc

echo "generator =" $GENERATOR
echo "test =" $TESTCODE

## for FPGA
# compile
g++ -D HALIDE_FOR_FPGA \
-fno-rtti -O2 -g -std=c++11 \
    -I$HALIDE_ROOT/include -I$HALIDE_ROOT/tools -I$HALIDE_ELEMENTS/include \
    -I$HALIDE_ELEMENTS/include/Element -I ../include \
-L$HALIDE_ROOT/lib \
$GENERATOR $HALIDE_ROOT/../tools//GenGen.cpp \
-o ${TARGET}_gen_hls \
-ldl -lpthread -lz -lHalide

# generate
HL_DEBUG_CODEGEN=2 LD_LIBRARY_PATH=$HALIDE_ROOT/lib ./${TARGET}_gen_hls -o . -g ${TARGET} -e hls target=fpga-64-vivado_hls 2>halide.log

## for host simulation
# compile
g++ \
-fno-rtti -O2 -g -std=c++11 \
    -I$HALIDE_ROOT/include -I$HALIDE_ROOT/tools -I$HALIDE_ELEMENTS/include \
    -I$HALIDE_ELEMENTS/include/Element -I ../include \
-L$HALIDE_ROOT/lib \
$GENERATOR $HALIDE_ROOT/../tools//GenGen.cpp \
-o ${TARGET}_gen_sim \
-ldl -lpthread -lz -lHalide

# generate
LD_LIBRARY_PATH=$HALIDE_ROOT/lib ./${TARGET}_gen_sim -o . -g ${TARGET} -e h,static_library target=host-no_asserts

# test binary
g++ \
-O2 -g -std=c++11 \
    -I . -I ../include -I$HALIDE_ROOT/include -I$HALIDE_ROOT/tools \
    -I$HALIDE_ELEMENTS/include/Element -I$HALIDE_ELEMENTS/include \
-L$HALIDE_ROOT/lib \
$TESTCODE *.a -o ${TARGET}_test -ldl -lpthread

#cp ${TARGET}${SUFFIX}_tb.cc ${TARGET}${SUFFIX}.hls/
#cp ${TARGET}${SUFFIX}_tb.h ${TARGET}${SUFFIX}.hls/
#cp run_hls.tcl ${TARGET}${SUFFIX}.hls/
