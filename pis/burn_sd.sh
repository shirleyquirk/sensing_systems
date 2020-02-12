#!/usr/bin/bash
echo "copying mbr"
dd if=mbr.bin of=/dev/mmcblk0 bs=446 count=1
use parted to make 1st partition fat32 starting at 8192 256MiB
2nd partition ext4
sudo mount /dev/mmcblk0p1 boot
sudo mount /dev/mmcblk0p2 root
cd boot
sudo tar -xzpf ../raspi_boot.tar.gz . --numeric-owner
cd ../root
sudo tar -xzpf ../raspi_root.tar.gz . --numeric-owner
edit root/etc/hostname
edit root/etc/hosts
rm -r root/home/pi/videos/*
cp -r rpi_videos/Bank {N} root/home/pi/videos

cd ..
sudo umount boot
sudo umount root

