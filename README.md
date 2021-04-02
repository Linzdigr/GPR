# GPR Project

This project is intended to provided a way to see under the ground thanks to the FMCW radar technology.

Two antennas are used (Rx & Tx).

Tx will emmit a frequency changing over time repeatedly (sweep) in order to mix the Tx and received back Rx signal to create an output LF beat which will later be analysed (FFT) to determine the density, size and distance of various subjects under the ground.

## Installing
```sh
git clone https://github.com/Linzdigr/GPR.git
```

## Compiling (Linux)
```sh
cd GPR
make && make clean
sudo ./nallàma
```

## Troobleshoot / Optimisation

You might expect some frequency instabilities of the generated DAC signal and it's probably caused from the Raspberry Pi 4 clock speed changing over time (ARM slowing down when it's not busy) and impacting the related clock of the main clock of SPI. The output DAC frequency (driven over SPI) is then not corresponding to the requested values anymore.

To read the actual speed of your CPU:

```
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq   # you'll probably see this changing over time
```

To force a fixed (here higher) CPU frequency, edit the `/boot/config.txt` and update/add these values to:

```
over_voltage=2
arm_freq=1750

force_turbo=1 # Fix the CPU freq to max 
```

Note: you should upgrade before the distro:

```sh
sudo apt update
sudo apt dist-upgrade
```

Overclock will take effect on the next reboot:

```sh
reboot # or sudo init 6
```

**WARNING: Overclocking your CPU will generate much more heat from the CPU itself. You must have provided a heat dissipation system to your Raspberry Pi in order to prevent permanent damage!**
