# hlmaster

Masterserver for Half-Life 1 (GoldSrc) servers in C and UDP sockets.

Probably the smallest and fastest implementation on planet Earth.  
Supports server list filtering by gamedir.

Win32 binary size 4KB, MSVC 14.29.30133, /O2, no standard library  
Linux binary size 12KB, GCC 6.3.0, -O2, libc

Static RAM usage 13MB at the default settings:  
MAX_SERVERS 8192 * MAX_GAMES 128 = 1048576 maximum servers

Testing indicates this implementation can serve ~8 million requests/sec with a single thread on i5-6600k @ 4.30GHz, OS Windows 7 x64.  
This means the only real limitation is your network speed.
