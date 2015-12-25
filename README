Elf loader
=====

> Runs 64-bit Elf files in-process on Linux, FreeBSD and PS4

##Description

The loader is (almost) self-contained. The two (non C) dependencies
needed are ```libps4``` and Node.js. Node is needed to run the server,
debug (diagnostic) output client and the file uploader. You will also need
a current version of clang or alter the Makefile (remove -target for example).

You can of course use other tools for the job (netcat or a C implementation).

Obviously, all libraries need to be statically linked into the executable. You
can try musl for an easy to build and use statically linkable lib C, to test elfs
locally as well as the PS4. Libps4 provides a statically linkable lib C for the PS4.
Be aware that the libps4-examples currently build in binary mode. This will change tomorrow.

##Usage and Hints
###Make
```
DEBUG=1 Make all
```
###Local
```
#You may want to run through 'local' versions first
# (Local) Hello World
bin/local/elfldr bin/example/null
# (Local) Hello World with memory restrictions similar to the PS4s Jit
bin/local/elfldr-protmem bin/example/stress
# (Local) Hello World in server mode (see example below)
# In debug mode connect to 5052, then send the file over 5053
bin/local/elfldr-srv
#Lastly
bin/local/elfldr-protmem-srv
```
###PS4
```
#Start server
node server.js
#Browse to server
# Wait until it hangs in step 5
# Connect debug channel (if build with DEBUG=1 make)
node client.js 192.168.1.45
# Send file
node client.js 192.168.1.45 5053 ../ps4/stress
# Return code is not returned to the browser (0 is)
# Use debug or your own IO channel (socket) for that
```

# Internals

It is advised to look into the Makefile, index.html and other files as they will
(hopefully somewhat understandably) show you switches (flags) you can use and
what they do.

##Switches
###local/index.html
```
var isBinaryLoader = false; // accept elfs / bins on 5053
var loopOnExit = true; // wait again after normal termination of code (otherwise segfault)

var debug = 3; // show more or less infos in load steps
var debugWait = 4000; // wait per step (to read info) applied on debug >= 3
```

###In Makefile (Defines)
```
__PS4__ // Set in the Makefile for all PS4 builds
Debug || DEBUG // Set one of the env. variables to build in debug mode (see below)
Libps4 || LIBPS4 // Set one of the env. variables to the Libps4 path
ElfLdrServer // Run loader as server (Also Auto-set on __PS4__)
ElfLdrEmulatePS4Memory // emulate PS4 memory conditions - good for impl. new relocs
BinLdr // run as bin loader
```
##Files
```
/ps4/bin/example/stress // shows vars and relocs working
/ps4/bin/example/forawhile // hangs
/ps4/bin/example/null // minimal test, does nothing but return
/ps4/bin/ldrgen // generates js loaders

/ps4/bin/ps4/binldr.bin // converted to js and automatically put in local/ldr
/ps4/bin/ps4/binldr.elf // for readelf and object copy mostly
/ps4/bin/ps4/elfldr.bin // converted to js and automatically put in local/ldr
/ps4/bin/ps4/elfldr.elf // for readelf and object copy mostly

// if you got a bug, try them bottom to top to find out what the trouble is
/ps4/bin/local/elfldr-protmem-srv // local server mode loader
/ps4/bin/local/elfldr-protmem // local file based protected memory version
/ps4/bin/local/elfldr-srv // server mode, with simple memory version
/ps4/bin/local/elfldr // file based, simple memory version
```

##Example

###Start the server
```
node server.js
Serving directory elfldr/local on port 5350
```

###(In debug mode) connect to debug channel. For a non-debug build no such
connection can be made.
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
[local]: Connected 192.168.178.1:5053
[local]: File (../ps4/stress) provided -> sending mode
[local]: Sending file
[local]: Send file
[local]: Connection closed
```

Binary mode works analogous but is not advertised (since more may go wrong).

##TODO
- Wait for Kernel -__-?
	- Obsolete immediately thereafter ^__^
- Unmap and unjit etc. appropriately
- Error handling improvements
- Clean up further / spilt mains out
- Stub debugger in?
- Neat loader screen -__-? Probably on a lazy day.

##Credit
- Rop/Exec stuff goes to CTurt, flatz, SKFU, droogie, Xerpi, Hunger, Takezo, nas, Proxima
	- No idea who did what - contact me to clarify (open an issue)
	- Yifan (?)
