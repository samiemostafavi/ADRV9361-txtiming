# ADRV9361-txtiming



## PL Module

Use ip_repo folder to add the module to the Vivado project



## PS Driver

Copy the drivers folder to the drivers folder of the Linux before compiling the kernel.

- Add this line to "arch/arm/configs/zynq_xcomm_adv7511_defconfig"

```
CONFIG_AXITIMER=y
```

- Add this line to "drivers/Kconfig"

```
source "drivers/txtiming/Kconfig"
```

- Add this line to "drivers/Makefile"

```
obj-$(CONFIG_TXTIMING)          += txtiming/
```

- Add this segment to the device tree file "/arch/arm/boot/dts/zynq-adrv9361-z7035-fmc.dts"

```
/ {
        txtiming@0x43c00000 {
                compatible = "txtiming";
                reg = <0x43c00000 0x10000>;
        };
};
```

