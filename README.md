Elf loader
=====

> Runs 64-bit Elf files in-process on Linux, FreeBSD and PS4

##Description

The loader requires the following things:
- basic build-tools (clang due to ps4 support)
- ps4dev/libps4
- node.js to run the server and client

Node is needed to run the server, debug (diagnostic) output client and the file uploader. You will also need a current version of clang or alter the Makefile (remove -target specifically additionally ld/objectcopy may produce undesired (large) binaries).

You can of course use other tools for the server, upload and debug output (netcat or a C implementation for example).

The elf loader does not support dynamically linked executables. All libraries need to be statically linked into the executable. You can try musl for an easy to build and use statically linkable libc on your x86-64 linux system (e.g. to test elfs
locally). Libps4 provides a statically linkable libc for the PS4.

##Usage and Hints
###Make
```
make clean
make
Options:
	debug=true			// set loader default to debug mode
 	target=x86-64		// target of loader, build for x86-64
		memory=emulate 	// set loader default to emulate ps4 memory conditions on x86-64
		server=true		// set loader default to server mode under x86-64
	target=ps4-bin		// (default) build as bin for the PS4
		keepelf=true	// keeps elf before converting it to bin (for debug purposes)
	threading=true		// sets loader default to run elfs threaded (ignored in file mode)
	(ldr=bin			// build loader to accept binaries instead of elfs - best not used)
```
###Commandline (x86-64) Arguments
```
--memory-plain				// use plain memory despite the build-in defaults
--memory-emulate			// use emulate memory
--file						// use file input
--server					// use server (5053) input, and debug (5052) if enabled
--debug						// enable debug mode
--no-debug					// disable debug mode
--threading					// enable threading mode (multiple elfs can be executed async - to close the server, send a non-elf - this mode does not print return codes)
--no-threading				// disable threading mode (only one elf runs blocking)
```
###Examples
####Local
```
// build for the ps4
make clean && make debug=true

// build for the local system
make clean && make debug=true target=x86-64 memory=emulate
//Now you can try: bin/ldr ../../libps4-examples/libless/stress/bin/stress

// use args to dynamically set modes
make clean && make target=x86-64
Now you can try: bin/ldr --debug --server --memory-emulate --threading
Then connect to 5052 and send the file over 5053
node client.js 127.0.0.1
node client.js 127.0.0.1 5053 ../../libps4-examples/libless/stress/bin/stress
node client.js 127.0.0.1 5053 ../../libps4-examples/libless/stress/bin/stress
```
####PS4
```
// Start server
node server.js

// Browse to server
// Wait until it hangs in step 5
// Connect debug channel (if build with DEBUG=1 make)
node client.js 192.168.1.45

// Send file
node client.js 192.168.1.45 5053 ../../libps4-examples/libless/stress/bin/stress
// Return code is not returned to the browser (0 is)
// Use your own IO channel (socket) or debug
```

# Internals

You may want to look into the Makefile, main.c, protmem.c, index.html and other files as they will (hopefully somewhat understandably) show you switches (flags) you can use and what they do.

##Switches
###local/index.html
```
var loopOnExit = true; // wait again after normal termination of code (otherwise segfault)
var debug = 3; // show more or less infos in load steps
var debugWait = 4000; // wait per step (to read info) applied on debug >= 3
```

###Defines used
```
__PS4__ // Set in the Makefile for all PS4 builds
Debug || DEBUG // Set one of the env. variables to build in debug mode (see below)
Libps4 || LIBPS4 // Set one of the env. variables to the Libps4 path
ElfLoaderServer // Run loader as server by default (Also Auto-set on __PS4__)
ElfLoaderEmulatePS4Memory // emulate PS4 memory conditions by default - good for impl. new relocs
BinaryLoader // run as binary loader
ElfLoaderThreading // run as threading server
```

##Complete PS4 Debug Example

###Start the server
```
node server.js
Serving directory elfldr/local on port 5350
```

###(In debug mode) connect to debug channel. For a non-debug build no such connection can be made.
```
$ node client.js 192.168.1.45
[local]: Trying to connect to 192.168.1.45:5052
[local]: Connected 192.168.1.45:5052
[local]: No file provided -> debug mode (out shell)
debugOpen(5052) -> 880679da0
ElfLdr
utilServerCreate(5053, 1, 20, 1) -> 166
accept(166, NULL, NULL) -> 167
elfCreateFromSocket(167) -> 880661500
protectedMemoryCreate(880661500) -> 88061abe0
elfLdrLoad(880661500, 202c04000, 202800000) -> 1
struct memory [88061abe0]
{
	writable -> 202a88000
	executable -> 202684000
	writableAligned -> 202c04000
	executableAligned -> 202800000
	writableHandle -> 167
	executableHandle -> 166
	size -> 2098880 = 0x2006c0
	alignment -> 2097152 = 0x200000
	pageSize -> 16384 = 0x4000
	alignedSize -> 4210688 = 0x404000
}
run() [202800000 + 0x4f8 = 2028004f8] -> 66 = 66 = 0x42
protectedMemoryDestroy(88061abe0) -> 1
elfDestroyAndFree(880661500)
debugClose(880679da0)
[local]: Connection closed
```

###Send Elf file
```
$ node client.js 192.168.1.45 5053 ../ps4/stress
[local]: Trying to connect to 192.168.1.45:5053
[local]: Connected 192.168.1.45:5053
[local]: File (../../libps4-examples/libless/stress/bin/stress) provided -> sending mode
[local]: Sending file
[local]: Send file
[local]: Connection closed
```

Binary mode works analogous but is not advertised (since more may go wrong).

##TODO
- Wait for Kernel -__-?
	- Obsolete immediately thereafter ^__^
- Error handling improvements
- Stub debugger in?
- Neat loader screen -__-? Probably on a lazy day.

##Credit
- Rop/Exec stuff goes to CTurt, flatz, SKFU, droogie, Xerpi, Hunger, Takezo, nas, Proxima
	- No idea who did what - contact me to clarify (open an issue)
	- Yifan (?)
