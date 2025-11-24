==========================================
= Create the Router and Switch templates =
==========================================

* Download the CHR raw disk image from https://mikrotik.com/download#chr

* Unzip the file, convert it to a (sparse) qcow2 file and move it to the images directory:
unzip chr-7.16.2.img.zip
qemu-img convert -f raw -O qcow2 chr-7.16.2.img chr-7.16.2.qcow2
mv chr-7.16.2.qcow2 ~/GNS3/images/QEMU/
rm chr-7.16.2.img

* Create thin-provisioned base images for Router and Switch:
cd ~/GNS3/images/QEMU/
qemu-img create -f qcow2 -b chr-7.16.2.qcow2 -F qcow2 chr-7.16.2-router.qcow2
qemu-img create -f qcow2 -b chr-7.16.2.qcow2 -F qcow2 chr-7.16.2-switch.qcow2

* Create the templates:
  640 MB RAM
  HDD virtio
  Router:
     11 virtio-net-pci network interfaces with name format ether{port1}
        Configure custom adapters -> rename adapter 10 to sfp1
  Switch:
     26 virtio-net-pci network interfaces with name format ether{port1}
        Configure custom adapters -> rename adapter 24 to sfp-sfpplus1
                                     rename adapter 25 to sfp-sfpplus2
  For now, disable "Use as linked base VM".

* Create a temporary project with one of each and configure them according
  to the instructions below, under "Setup a switch or router"

* Shutdown both with
/system shutdown

* Delete the temporary project and reconfigure the templates to enable
  "Use as linked base VM".


============================
= Setup a switch or router =
============================

* Create the .rsc file:
/file add name=clean_config.rsc
/file edit clean_config.rsc contents

* Paste the appropriate script and terminate with CTRL-o.

* Now (and whenever you need it), reset the configuration:
/system reset-configuration no-defaults=yes skip-backup=yes run-after-reset=clean_config.rsc

