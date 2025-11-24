# jan/02/1970 00:03:37 by RouterOS 7.16.2
# Default configuration for a 26 port switch - rcprior
# 
# Remove all interfaces from the DHCP client
/ip dhcp-client
remove [ find ]
# Construct MAC address for the bridge interface
:local ether1Mac [/interface ethernet get [find default-name="ether1"] mac-address]
:local macPart1 [:pick $ether1Mac 0 15]
:local macPart2 "FF"
:local newMac ($macPart1 . $macPart2)
# Do the initializations
/interface bridge
add admin-mac=$newMac auto-mac=no comment=defconf name=bridge
/interface ethernet
set [ find ] disable-running-check=no
set [ find default-name=ether25 ] name=sfp-sfpplus1
set [ find default-name=ether26 ] name=sfp-sfpplus2
/interface bridge port
add bridge=bridge comment=defconf interface=ether1
add bridge=bridge comment=defconf interface=ether2
add bridge=bridge comment=defconf interface=ether3
add bridge=bridge comment=defconf interface=ether4
add bridge=bridge comment=defconf interface=ether5
add bridge=bridge comment=defconf interface=ether6
add bridge=bridge comment=defconf interface=ether7
add bridge=bridge comment=defconf interface=ether8
add bridge=bridge comment=defconf interface=ether9
add bridge=bridge comment=defconf interface=ether10
add bridge=bridge comment=defconf interface=ether11
add bridge=bridge comment=defconf interface=ether12
add bridge=bridge comment=defconf interface=ether13
add bridge=bridge comment=defconf interface=ether14
add bridge=bridge comment=defconf interface=ether15
add bridge=bridge comment=defconf interface=ether16
add bridge=bridge comment=defconf interface=ether17
add bridge=bridge comment=defconf interface=ether18
add bridge=bridge comment=defconf interface=ether19
add bridge=bridge comment=defconf interface=ether20
add bridge=bridge comment=defconf interface=ether21
add bridge=bridge comment=defconf interface=ether22
add bridge=bridge comment=defconf interface=ether23
add bridge=bridge comment=defconf interface=ether24
add bridge=bridge comment=defconf interface=sfp-sfpplus1
add bridge=bridge comment=defconf interface=sfp-sfpplus2
/ip address
add address=192.168.88.1/24 comment=defconf interface=bridge network=\
    192.168.88.0
