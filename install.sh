#!/bin/bash

#Bash script to install supporting software for the Robotics Cape

INSTALL_DIR="/home/root"

touch *

echo "Flashing Cape EEPROM"
cat install_files/data.eeprom > /sys/bus/i2c/devices/1-0054/eeprom 


echo "Compiling and Installing Device Tree Overlay"
dtc -O dtb -o /lib/firmware/SD-101B-00A0.dtbo -b 0 -@ install_files/SD-101B-00A0.dts
cp /boot/am335x-boneblack.dtb /boot/am335x-boneblack.dtb.old 
cp install_files/am335x-boneblack.dtb /boot/
cp install_files/tieqep.ko /usr/bin/


echo "Copying uEnv.txt to Disable HDMI"
mount /dev/mmcblk0p1 /media/BEAGLEBONE >/dev/null
cp /media/BEAGLEBONE/uEnv.txt /media/BEAGLEBONE/uEnv.txt.old
cp install_files/uEnv.txt /media/BEAGLEBONE/


echo "Enabling Boot Script"
#cp -r startup /home/root/
cp -r install_files/bootscript.sh /usr/bin/
chmod u+x /usr/bin/bootscript.sh
cp install_files/bootscript.service /lib/systemd/
rm /etc/systemd/system/bootscript.service
ln /lib/systemd/bootscript.service /etc/systemd/system/bootscript.service
systemctl daemon-reload 
systemctl enable bootscript.service 


echo "Installing Examples"
cp -r examples/ $INSTALL_DIR
cp $INSTALL_DIR/examples/balance/balance 			/usr/bin/
cp $INSTALL_DIR/examples/bare_minimum/bare_minimum 	/usr/bin/
cp $INSTALL_DIR/examples/test_buttons/test_buttons 	/usr/bin/
cp $INSTALL_DIR/examples/test_encoders/test_encoders /usr/bin/
cp $INSTALL_DIR/examples/test_motors/test_motors 	/usr/bin/
cp $INSTALL_DIR/examples/test_spektrum/test_spektrum 	/usr/bin/
cp $INSTALL_DIR/examples/calibrate_spektrum/calibrate_spektrum 	/usr/bin/
chmod 755 /usr/bin/*

echo "Installing Supporting Libraries"
cp -r libraries/ $INSTALL_DIR
cd $INSTALL_DIR/libraries
make clean
make install

echo
echo "Robotics Cape Configured and Installed"
echo "Reboot to complete installation."
echo

