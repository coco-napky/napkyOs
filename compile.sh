rm ./bootload
rm ./kernel.o
rm ./kernel
rm ./os_api.o

nasm ./bootload.asm
dd if=/dev/zero of=floppya.img bs=512 count=2880
dd if=bootload of=floppya.img bs=512 count=1 conv=notrunc

bcc -ansi -c  -o kernel.o kernel.c
as86 kernel.asm -o kernel_asm.o

ld86 -o kernel -d kernel.o kernel_asm.o
dd if=kernel of=floppya.img bs=512 conv=notrunc seek=3

#Add Map and File Directory to floppy
dd if=map.img of=floppya.img bs=512 count=1 seek=1 conv=notrunc
dd if=dir.img of=floppya.img bs=512 count=1 seek=2 conv=notrunc

#shell
bcc -ansi -c  -o shell.o shell.c
as86 os_api.asm -o os_api_asm.o
as86 lib.asm -o lib.o
ld86 -o shell -d shell.o lib.o os_api_asm.o

./loadFile shell
./loadFile message.txt
./loadFile tstpr2
./loadFile phello

#napky1
bcc -ansi -c  -o napky1.o napky1.c
ld86 -o napky1 -d napky1.o lib.o os_api_asm.o

./loadFile napky1

cd ~/Desktop/siso1/osproject/ProjectC/fuse-smallfs/bin & ./sfs ~/Desktop/siso1/osproject/ProjectC/floppya.img  sfs_root/ -s & cd ~/Desktop/siso1/osproject/ProjectC

bochs -f opsys.bxrc