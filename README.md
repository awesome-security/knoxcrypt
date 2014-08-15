TeaSafe: An encrypted container format
--------------------------------------

##### What is it?

- an encrypted container format
- containers can be created as sparse for better space manageability 
- employs a very simple and custom developed filesystem (see wiki!)
- TeaSafe containers can be browsed using either of the provided shell or gui interfaces
- alternatively, you can implement your own. 
- can also use the provided FUSE-layer for more realistic filesystem interoperability
- utilizes XTEA, a publicly documented and easily understood cipher
- further utilizes the scrypt algorithm for key derivation

### Compiling

Note, only tested on Linux and Mac. With a bit of work, will probably build (sans fuse-bits) on windows
too.

Note:
 
- requires some of the boost headers and libraries to build (see makefile).
- requires fuse for the main fuse layer binary (the binary 'teasafe')

If you don't have fuse installed, you'll probably want to only build the main 
teasafe library (libteasafe.a), the shell (teashell) and maketeasafe, the binary
used to make teasafe containers. To build these, respectively:
<pre>
make lib
make shell
make maketeasafe
</pre>
Note that building either of the binaries `teashell` or `maketeasafe` will automatically build 
libteasafe.a first.

`make` or `make all` will compile everything except the GUI, i.e., the following binaries:

<pre>
test         : unit tests various parts of the main api
maketeasafe  : builds teasafe containers
teasafe      : fuse layer used for mounting teasafe containers
teashell     : shell utility used for accessing and modifying teasafe containers
</pre>

To build a teasafe container, use the `maketeasafe` binary:

<pre>
./maketeasafe ./test.bfs 128000
</pre>

Sparse containers can be created too. These are far smaller than
non-sparse versions; such a container will dynamically grow as more data is written to it.
Just use the `--sparse` flag during creation, i.e.:

<pre>
./maketeasafe ./test.bfs 128000 --sparse 1
</pre>

Now to mount it to `/testMount` via fuse, use the `teasafe` binary:

<pre>
./teasafe ./test.bfs /testMount
</pre>

Runs the interactive shell on it using the `teashell` binary:

<pre>
./teashell ./test.bfs
</pre>

For more info, please post up on `https://groups.google.com/forum/#!forum/teasafe`.

### Building the GUI

To build the GUI, first make sure that `libteasafe.a` has been built by issuing the
command `make lib` in the top-level build-folder. 

The GUI uses Qt. Please download and install the latest version (Qt 5.3 at the time
of writing) and open gui.pro in QtCreator. Build and run by clicking on the build icon.

The GUI provides a simple interface to browsing and manipulating TeaSafe containers.

![TeaSafe GUI](screenshots/gui.png?raw=true)



Licensing
---------

TeaSafe follows the BSD 3-Clause licence. 


