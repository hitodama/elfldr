Elf loader
=====

> Runs 64-bit Elf files in-process on Linux, FreeBSD and PS4

##Description

The loader requires the following things:
- basic build-tools (clang due to ps4 support)
- ps4dev/libps4
- node.js to run the server
- socat, netcat etc

You will also need a current version of clang or alter the Makefile (remove -target specifically additionally ld/objectcopy may produce undesired (large) binaries).

You can of course use other tools for the server, upload or debug output (socat, netcat or a C implementation for example).

The elf loader does not support dynamically linked executables. All libraries need to be statically linked into the executable. You can try musl for an easy to build and use statically linkable libc on your x86-64 linux system (e.g. to test elfs
locally). Libps4 provides a statically linkable libc for the PS4.

##Usage and Hints
You may want to look into the Makefile, index.html, config.c and other files as they will (hopefully somewhat understandably) show you switches (defines / flags) you can use and what they do.

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
	stdio=wait|lazy|none // sets default std io redirect mode (lazy is a "may" option), can be combined with debug
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
--threading					// enable threading mode (multiple elfs can be executed async - to close the server, send a non-elf - this prints return codes out of order)
--no-threading				// disable threading mode (only one elf runs blocking)
--stdio-none				// disables stdio redirect, overwritten by debug
--stdio-wait				// enables stdio redirect, connection must be made prior to running code
--stdio-lazy				// enables stdio redirect, connection can be made later, output will be lost
```

##Switches in local/index.html
```
var loopOnExit = true; // wait again after normal termination of code (otherwise segfault)
var debug = 3; // show more or less infos in load steps
var debugWait = 4000; // wait per step (to read info) applied on debug >= 3
```


###Examples
####Local
```
// build for the local system / set defaults via defines
make clean && make debug=true target=x86-64 memory=emulate

//Now you can try:
bin/ldr --file ../../libps4-examples/libless/stress/bin/stress

// use args to dynamically set modes
make clean && make target=x86-64
Now you can try:
bin/ldr --debug --server --memory-emulate --threading

Then connect to 5052 (if debug set) and send the file over 5053
socat - TCP:127.0.0.1:5052
socat -u FILE:../../libps4-examples/libless/stress/bin/stress TCP:127.0.0.1:5053
```
####PS4
```
// build for the ps4 / we need to set all prefered defaults at build time (no args to main)
make clean && make debug=true stdio=wait

// Start server
node server.js

// Browse to server
// Wait until it hangs in step 5

// Connect debug/stdio channel (if build with debug=true or stdio=wait)
socat - TCP:192.168.1.45:5052

// Send file
socat -u FILE:../../libps4-examples/libless/stress/bin/stress TCP:192.168.1.45:5053

// Return code is not returned to the browser (0 is)
// Use stdio=wait, debug=true or your own IO (socket) to get output / return codes
```

##Complete PS4 Debug Example

###Start the server
```
node server.js
Serving directory elfldr/local on port 5350
```

###(In debug mode) connect to debug channel. For a non-debug build no such connection can be made.
```
$ socat - TCP:192.168.178.45:5052
[main|545]: debugOpen(5052)
[main|548]: Mode -> ElfLoader [input: 1, memory: 0, thread: 0, debug: 1, stdio: 1]
[main|550]: utilServerCreate(5053, 20, 1) -> [main|554]: 140
[elfLoaderServerAcceptElf|256]: accept(140, NULL, NULL) -> [elfLoaderServerAcceptElf|263]: 143
[elfLoaderServerAcceptElf|265]: elfCreateFromSocket(143) -> [elfLoaderServerAcceptElf|271]: 880ba8100
[elfLoaderServerAcceptElf|273]: close(143) -> [elfLoaderServerAcceptElf|274]: 0
[main|561]: close(140) -> [main|562]: 0
[elfLoaderMemoryCreate|311]: protectedMemoryCreate(2098784) -> [elfLoaderMemoryCreate|321]: 880bb8160
[elfLoaderRunSetup|373]: elfLoaderLoad(880ba8100, 2013ec000, 2011e8000) -> [elfLoaderRunSetup|379]: 0
[elfLoaderRunSetup|383]: elfDestroyAndFree(880ba8100)
[elfLoaderRunSync|410]: run(1, {"elf", NULL}) [2011e8000 + elfEntry = 2011e8380] -> [elfLoaderRunSync|412]: 66
[elfLoaderMemoryDestroy|351]: protectedMemoryDestroy(880bb8160) -> [elfLoaderMemoryDestroy|356]: 0
[main|580]: debugClose()
$
```

###Send Elf file
```
$ socat -u FILE:../../libps4-examples/libless/stress/bin/stress TCP:192.168.178.45:5053
```

Binary mode works analogous but is not advertised (since more may go wrong).

##Credit
- Rop/Exec stuff goes to CTurt, flatz, SKFU, droogie, Xerpi, Hunger, Takezo, nas, Proxima
	- No idea who did what - contact me to clarify (open an issue)
	- Yifan (?)
