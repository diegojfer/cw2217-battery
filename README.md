# CW2217B Battery Fuel Gauge Linux Driver

Linux kernel driver for the **Cellwise CW2217B** battery fuel gauge IC. Communicates over I2C and exposes battery and mains data through the Linux `power_supply` subsystem.

## Reported Properties

| Property | Unit | Description |
|---|---|---|
| `POWER_SUPPLY_PROP_PRESENT` | boolean | Always reports battery as present |
| `POWER_SUPPLY_PROP_STATUS` | enum | Charging or discharging based on current direction |
| `POWER_SUPPLY_PROP_CAPACITY` | % | State of charge (0-100) |
| `POWER_SUPPLY_PROP_VOLTAGE_NOW` | uV | Battery voltage |
| `POWER_SUPPLY_PROP_CURRENT_NOW` | uA | Battery current (positive = charging) |
| `POWER_SUPPLY_PROP_TEMP` | 0.1 C | Battery temperature |

The driver polls the CW2217B registers every 1 second via a delayed work queue.

## Configuration

The sense resistor value is defined at the top of `cw2217b.c`:

```c
#define CW2217B_RSENSE_MOHM (10)
```

Adjust this to match the sense resistor on your board (in milliohms).

## Building

Requires kernel headers for the running kernel.

```sh
make
```

The compiled module will be at `cw2217b.ko`.

## Loading / Unloading

```sh
sudo insmod cw2217b.ko
sudo rmmod cw2217b
```

The CW2217B must be registered as an I2C device on the appropriate bus. For example, via a device tree overlay or manual instantiation:

```dts
/dts-v1/;
/plugin/;

/ {
    compatible = "brcm,bcm2711", "brcm,bcm2712", "brcm,bcm2835";

    fragment@0 {
        target = <&i2c1>;
        __overlay__ {
            #address-cells = <1>;
            #size-cells = <0>;
            status = "okay";

            cw2217b: cw2217b@64 {
                reg = <0x64>;
                compatible = "cellwise,cw2217b";
            };
        };
    };
};
```

```sh
echo cw2217b 0x64 | sudo tee /sys/bus/i2c/devices/i2c-1/new_device
echo 0x64 | sudo tee /sys/bus/i2c/devices/i2c-1/delete_device
```

Once loaded, battery data is available under `/sys/class/power_supply/cw2217b/`.

## Cleaning

```sh
make clean
```

## License

GPL License
