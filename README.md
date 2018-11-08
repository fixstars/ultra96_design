# Ultra96 Design

## Requirement

These are *MANDATORY* items to be installed.

1. Vivado 2018.2
2. License of Xilinx MIPI CSI-2 Controller SubSystem 

You can use Evaluation license for Xilinx MIPI IP. Please go to the [product page](https://www.xilinx.com/products/intellectual-property/ef-di-mipi-csi-rx.html).

## Getting started

You need to create project by tcl script when right after cloning the repos.
Launch vivado and create project by tcl file.

```.tcl
source script/create_project.tcl
```

Then you can do synthesize, implementation and create bitstream whatever.


## Work with git

If you updated block design, please export design to script/create\_bd.tcl and commit it.

```.tcl
write_bd_tcl script/create_bd.tcl
```

If you want to add new files or directories, handle them in script/create\_project.tcl and commit them.
