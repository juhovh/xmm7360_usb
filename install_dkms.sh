sudo mkdir /usr/src/xmm7360_usb-0.1
sudo cp -r . /usr/src/xmm7360_usb-0.1
sudo dkms add -m xmm7360_usb/0.1
sudo dkms build -m xmm7360_usb -v 0.1
sudo dkms install -m xmm7360_usb -v 0.1