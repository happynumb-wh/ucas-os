# UCAS-OS

仿照 `linux` 编写的能够运行简单标准 `posix` 标准和 `fat32` 文件系统的小操作系统。

## 编译

需要首先设置 `RISCV_ROOTFS_HOME` 环境变量，目前包含了如下几个测试：

```makefile
$(RISCV_ROOTFS_HOME)/rootfsimg/build/dasics-test-free \
$(RISCV_ROOTFS_HOME)/rootfsimg/build/dasics-test-jump \
$(RISCV_ROOTFS_HOME)/rootfsimg/build/dasics-test-rwx \
$(RISCV_ROOTFS_HOME)/rootfsimg/build/dasics-test-syscall \
$(RISCV_ROOTFS_HOME)/rootfsimg/build/dasics-test-ofb \
$(RISCV_ROOTFS_HOME)/rootfsimg/build/attack-case
```

直接进入到 `riscv-pk` 目录下执行： 

```shell
$: make #内嵌设备树
$: make qemu # QEMU 平台
```

编译得到的 `bbl` 镜像位于 `riscv-pk/build/` 中 

## Difftest

为了 `difftest` 方便现目前这些测试都是内嵌到内核当中。

`QEMU` 标准运行结果：

```shell
bbl loader
> [INIT] First core enter kernel
> [INIT] 1022 Mem initialization succeeded.
> [INIT] shell initialization succeeded, pid: 1.
> [INIT] Interrupt processing initialization succeeded.
> [INIT] System call initialized successfully.
> [INIT] SCREEN initialization succeeded.
> [INIT] System_futex initialization succeeded.
[Shell]: DASICS_TEST:
[Shell]: Do test : dasics_test_rwx
[kernel]: start_code: 0x10000, end_code: 0x22e00
[kernel]: start_data: 0x23000, end_data: 0x242d8
[kernel]: elf_bss: 0x242d8, elf_brk: 0x24380
[kernel]: ulibtext start: 0x217e4, end: 0x218ec
[kernel]: text start: 0x100e8, end: 0x217e2
[MAIN]-  Test 3: bound register allocation and authority 
************* ULIB START ***************** 
try to print the read only buffer: [ULIB1]: It's readonly buffer!
try to print the rw buffer: [ULIB1]: It's public rw buffer!
try to modify the rw buffer: [ULIB1]: It's public rw buffer!
new rw buffer: [ULIB1]: It's publis Bw buffer!
try to modify read only buffer
[DASICS EXCEPTION]Info: dasics store fault occurs, scause = 0x1a, sepc = 0x21888, stval = 0x241a7
try to load from the secret
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x218a0, stval = 0x24133
try to store to the secret
[DASICS EXCEPTION]Info: dasics store fault occurs, scause = 0x1a, sepc = 0x218c0, stval = 0x24133
************* ULIB   END ***************** 
[MAIN]test dasics finished
[Shell]: Do test : dasics_test_jump
[kernel]: start_code: 0x10000, end_code: 0x22f60
[kernel]: start_data: 0x23000, end_data: 0x242d8
[kernel]: elf_bss: 0x242d8, elf_brk: 0x24380
[kernel]: ulibtext start: 0x217d8, end: 0x218d8
[kernel]: text start: 0x100e8, end: 0x217d6
[MAIN]-  Test 2: lib function jump or call 
************* ULIB START ***************** 
This is a normal lib call
************* ULIB   END ***************** 
************* ULIB START ***************** 
try to printf directly without maincall
[DASICS EXCEPTION]Info: dasics fetch fault occurs, scause = 0x18, sepc = 0x21948, stval = 0x15178
try to jump to main function
[DASICS EXCEPTION]Info: dasics fetch fault occurs, scause = 0x18, sepc = 0x2195c, stval = 0x101a8
try to jump from freezone area to lib area
[DASICS EXCEPTION]Info: dasics fetch fault occurs, scause = 0x18, sepc = 0x21970, stval = 0x2180c
try to jump between freezone area
This is a normal free call
************* ULIB   END ***************** 
[DASICS EXCEPTION]Info: dasics fetch fault occurs, scause = 0x18, sepc = 0x217f0, stval = 0x15226
Try to call free in lib area
[DASICS EXCEPTION]Info: dasics fetch fault occurs, scause = 0x18, sepc = 0x21864, stval = 0x218d8
[MAIN]test dasics finished
[Shell]: Do test : dasics_test_ofb
[kernel]: start_code: 0x10000, end_code: 0x22c80
[kernel]: start_data: 0x23c80, end_data: 0x24e88
[kernel]: elf_bss: 0x24e88, elf_brk: 0x24f30
[kernel]: ulibtext start: 0x21780, end: 0x21808
[kernel]: text start: 0x100e8, end: 0x2177e
[MAIN]-  Test 1: out of bound test 
************* ULIB START ***************** 
try to load from the unbounded address: 0x24db0
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x217b8, stval = 0x24db0
try to store to the unbounded address:  0x24db0
[DASICS EXCEPTION]Info: dasics store fault occurs, scause = 0x1a, sepc = 0x217dc, stval = 0x24db1
************* ULIB   END ***************** 
[MAIN]test dasics finished
[Shell]: Do test : dasics_test_free
[kernel]: start_code: 0x10000, end_code: 0x22be0
[kernel]: start_data: 0x23be0, end_data: 0x24de8
[kernel]: elf_bss: 0x24de8, elf_brk: 0x24e90
[kernel]: ulibtext start: 0x0, end: 0x0
[kernel]: text start: 0x100e8, end: 0x217f2
[MAIN]-  Test 4: test dadisc bound register free 
DASICS uLib CFG Registers: handle:0  config: b 
DASICS uLib CFG Registers: handle:0  config: ffffffff 
[MAIN]test dasics finished
[Shell]: Do test : case-study
[kernel]: start_code: 0x10000, end_code: 0x22e30
[kernel]: start_data: 0x23000, end_data: 0x241b8
[kernel]: elf_bss: 0x241b8, elf_brk: 0x24270
[kernel]: ulibtext start: 0x0, end: 0x0
[kernel]: text start: 0x100e8, end: 0x2181a
> [Begin] Call Malicious
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000fad0
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000fad1
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000fad2
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000fad3
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000fad4
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000fad5
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000fad6
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000fad7
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000fad8
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000fad9
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000fada
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000fadb
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000fadc
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000fadd
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000fade
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000fadf
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000fae0
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000fae1
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000fae2
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000fae3
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000fae4
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000fae5
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000fae6
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000fae7
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000fae8
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000fae9
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000faea
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000faeb
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000faec
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000faed
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000faee
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000faef
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000faf0
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000faf1
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000faf2
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000faf3
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000faf4
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000faf5
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000faf6
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000faf7
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000faf8
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000faf9
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000fafa
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000fafb
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000fafc
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000fafd
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000fafe
[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x19, sepc = 0x21964, stval = 0xf0000faff
 - - - - - - Malicious Call - - - - - - - -  
 Try to get Secret data: \D0\D1\D2\D3\D4\D5\D6\D7\D8\D9\DA\DB\DC\DD\DE\DF\E0\E1\E2\E3\E4\E5\E6\E7\E8\E9\EA\EB\EC\ED\EE\EF\F0\F1\F2\F3\F4\F5\F6\F7\F8\F9\FA\FB\FC\FD\FE\FF\90\F6, ret point: 0xf0000faf8, ret_addr: 0xfffefdfcfbfaf9f8 
 Try to change return address of main to Rop: 0x102b0 
[DASICS EXCEPTION]Info: dasics store fault occurs, scause = 0x1a, sepc = 0x218f8, stval = 0xf0000faf8
 - - - - - - Malicious Call End- - - - - - - -  
> [End] Back to Main
[Shell]: Over, go to sleep
```

