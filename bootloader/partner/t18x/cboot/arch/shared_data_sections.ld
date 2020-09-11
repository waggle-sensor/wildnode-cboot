/* should be included in all lk linker scripts in the .data segment */
. = ALIGN(8);
__commands_start = .;
KEEP (*(.commands))
__commands_end = .;
. = ALIGN(8);
__apps_start = .;
KEEP (*(.apps))
__apps_end = .;
. = ALIGN(8);
__devices = .;
KEEP(*(.devices))
__devices_end = .;
__fs_impl_start = .;
KEEP (*(.fs_impl))
__fs_impl_end = .;
