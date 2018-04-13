cd build
mv fApp.self eboot.bin
curl -T eboot.bin ftp://192.168.1.102:1337/ux0:/app/FAPPENING/

echo " UPLOADED AT : $(date)"

