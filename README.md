TeaSafe: An experimental fuse-based filesystem
----------------------------------------------

TeaSafe is an experimental filesystem designed with encryption in mind. 
The whole filesystem is block-based, exising as a single 'image'
that can be mounted to a user-specified mount-point. Rather than encrypting each file entry
individually, the whole image is transformed using a variant of the XTEA algorithm.
TeaSafe also incorporates an experimental
'coffee mode' in which a 'hidden partition' can be specified at the time
of image creation. At the time of mounting, the user can choose to mount this
rather than the default root folder.

### Security

The filesystem is encrypted using a varient of the XTEA algorithm. 
I don't consider myself an expert in encryption so I would suggest you
review my code before you consider it secure or not.
The more keen developer is encouraged to implement their own transformational cipher. All she 
needs to do is implement the function `doTransform` in `IByteTransformer` as defined in `IByteTransformer.hpp`.
See file `BasicByteTransformer.hpp` as an example of how this is done. The pointer type of `m_byteTransformer`
as initialized in the constructor argument list of `TeaSafeImageStream.cpp` then needs to be updated to
the developer's new implementation e.g.:

`m_byteTransformer(boost::make_shared<cipher::BasicByteTransformer>(io.password))` --->
`m_byteTransformer(boost::make_shared<cipher::SomeOtherImplementation>(io.password))`

A hidden partition scheme is also supported in a similar vein to truecrypt. More details soon.

### Development requirements

All development was undertaken on a machine running osx10.9.
The actual development requirements are thus:

- Because of the use of strongly-typed enums, a c++11 compiler 
- a couple of boost libraries (system and filesystem) and the boost headers. Note, the makefile will need 
updating according to where boost is installed
- OpenSSL crypto library and sha.h header which is only used for generating SHA256 hashes
- the latest version of osxfuse (I'm using 2.6.2). As with boost, you might need to update the makefile
(afaik, on linux, an implementation of FUSE is part of the kernel already).

I envisage no problems running and compiling on linux. Windows unfortunately is a completely different beast
not least of which is due to a lack of a FUSE implementation.

### Compiling

`make` or `make all` will compile everything and all binaries. Please see above notes
on modifying the Makefile to point to correct library and header paths.

### Running

`./test` will run the test suite. This unit tests various parts of TeaSafe. As I uncover
new bugs and attempt to fix them, I will probably (but not always) add new units to verify the fixes.

`./maketeasafe image.tea 128000` will create a 500MB TeaSafe image when the block
size is 4096 (note the block size is hardcoded into DetailTeaSafe.hpp and represents
a good compromise between file speed and space efficiency). Example output:

<pre>
image path: test.tea
file system size in blocks: 128000
password:
</pre>

The password string will seed a sha256 hash used to generate the
cipher stream.

`./maketeasafe image.tea 128000 --coffee true` will create a 500MB TeaSafe image with
both a default root folder offset at block 0, and an extra partition offset by a user-specified
'pin value' that must be less than the number of blocks (128000 in this example)
but greater than 0. Example output:

<pre>
image path: test.tea
file system size in blocks: 128000
password:
magic number:
</pre>

The 'magic number' input will specify the pin value.

`./teasafe image.tea testMount` will launch and mount image.teasf under 
the directory testMount in fuse debug mode; note to disable debug
mode you need to specify `--debug 0` as an extra parameter. Disabling
debug mode will mount the image in single-threaded mode. Multi-threaded mode
is not currently supported.

If the image was initialized with a coffee partition, then the image can be mounted
with the coffee parameter, i.e.:

`./teasafe image.tea testMount --coffee true`

This will ask the user to enter both the decryption password and the magic number.

Licensing
---------

TeaSafe follows the BSD 2-Clause licence. 
