qemu -serial mon:stdio -hdb fs.img xv6.img -smp 2 -m 512  -S -gdb tcp::1234
