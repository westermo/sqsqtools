Squash-Squash Image Tools
=========================

This repository contains some tools used to manage SQSQ images.

SQSQ Image Format
=================
The SQSQ format is a custom format designed for a specific use-case, namely
embedded device images and addition to these.

The format consists of an arbitrary amount of chained SquashFS pairs. Each pair
has a data filesystem and a meta filesystem describing the data filesystem.

Basic layout
------------
```
+-------------------+------------------+-------------------+-------------------+
|                   |                  |                   |                   |
| SquashFS Data     | Sqashfs Meta     | SquashFS Data     | SquashFS Meta     |
|                   |                  |                   |                   |
+-------------------+--------------------------------------+-------------------+
^                                      ^                                       ^
|          SQSQ Filesystem 1           |          SQSQ Filesystem 2            |
+--------------------------------------+---------------------------------------+
```
In a nutshell, the SQSQ image format is a chain of Squash filesystems. Arraigned
in pairs of data and meta. Where meta contains information about the data.

Rationale
---------
The SQSQ image format is designed to address two main problem.

* How to bundle the signature and checksum of a Squash filesystem in the same
image file.
* How to add data to a signed SquashFS without modifying the signature.

### Use case example

An embedded device is manufactured by Encom. It boots "encom-data.sqfs" which
contains an operating system and a basic userland. Encom wants to sign the data
in "encom-data.sqfs" with there own key and bundle the signature in the same
image file. SQSQ solves this by providing a trailing filesystem "encom-meta.sqfs"
for storing the signature in. Still within the same image file "encom.sqsq". The
files in "encom.sqsq" are still easily accessible using the sqsqtools.

A second entity "Flynn" might want to add signed data to the manufacturers
"encom.sqsq" image. Perhaps a branding logotype, a user space application or a
clip of a dog. Flynn doesn't have access to the Encom signing key, so he can't
add data to the "encom-data.sqfs" filesystem without breaking the signature in
"encom-meta.sqfs". The SQSQ format allows Flynn to add an additional signed SQSQ
pair "flynn.sqsq" to "encom.sqsq" which might create "tron.sqsq".

The file "tron.sqsq" contains both "encom.sqsq" and "flynn.sqsq". Both are signed
by different entities and it's up to the signed code in "encom-data.sqfs" to
validate the legitimacy of Flynns signature in "flynn-meta.sqfs".

Here's how "tron.sqsq" looks:
```
+-------------------+------------------+-------------------+-------------------+
|                   |                  |                   |                   |
|  encom-data.sqfs  |  encom-meta.sqfs |  flynn-data.sqfs  |  flynn-meta.sqfs  |
|                   |                  |                   |                   |
+-------------------+--------------------------------------+-------------------+
^                                      ^                                       ^
|              encom.sqsq              |               flynn.sqsq              |
+--------------------------------------+---------------------------------------+
^                                                                              ^
|                                 image.sqsq                                   |
+------------------------------------------------------------------------------+
```

The "chain of trust" in the above example might be implemented as follows:
1) The bootloader calculates the sha256 digest of the first data partition
   "encom-data.sqfs".
2) The bootloader checks the sha256 digest signature in first meta partition
   "encom-meta.sqfs" against a trusted Encom key from hardware.
3) The bootloader boots the "encom-data.sqfs" filesystem.
4) Code within the "encom-data.sqfs" filesystem calculates sha256 digest of the
   second data filesystem "flynn-data.sqfs".
5) Code within the "encom-data.sqfs" filesystem checks the sha256 digest
   signature in second meta partition "flynn-meta.sqfs" against a trusted Flynn
   key from the Encom data partition "encom-data.sqfs".
6) The second meta partition "flynn.sqfs" is mounted and used by trusted Encom
   code.

Benefits
--------

### Ease of use

Standard tools such as mksquashfs, unsquashfs and mount can be used to create,
view and modify the individual SQSQ filesystems.

### SquashFS compatibility

As the first header of any SQSQ chain is a standard SquashFS header, a tool
which do not understand SQSQ handles the first data filesystem as a single normal
Squash filesystem. This makes the format backwards compatible for some specific
use-cases. Such as booting the first data filesystem from a bootloader which do
not understand the SQSQ format.

### Scalability

An arbitrary amount of SQSQ filesystems can be chained together. This allows
additions to a monolithic image file. For example, the first data filesystem
might be a signed root filesystem and the second data filesystem might contain
signed distribution additions.

### Integrity

The meta filesystem can have arbitrary content (see Limitations) but some
filenames are reserved for integrity check of the data filesystem
(see Meta Format). This makes software (data filesystem) signing and integrity
check easy.

Limitations
-----------

### Filesystem addition

It's not known to the SQSQ format how many filesystem pairs it contains. This
means that nothing in the SQSQ format stops a rogue user from adding additional
filesystem pairs. The implementation or user need to take this into
consideration and not use unknown filesystems.


### Meta data addition

The SQSQ format supports data filesystem signing and integrity validation,
this controls what's stored in the data filesystem. However, nothing prevents
additions to the meta filesystem. This means that the implementation or user
needs to take this into consideration.

* Avoid placing anything expect SQSQ data on the meta filesystem.
* **Don't execute anything stored in the meta filesystem.**


Meta Format
-----------

The meta filesystem is a normal Squash filesystem. Some files are used by the
SQSQ format for integrity check and signature validation.

### Meta Files

**sha1sum**

A file containing the sha256sum of the entire data filesystem (including
padding). Represented by 64 hex ascii characters.

Tools
=====

sqsq-image
----------
`sqsq-image` is the low level tool which understands the internals of the SQSQ
format. It can be used separately to get various information from a SQSQ mage
file. See `sqsq-image -h` for more info.

### sqsq-image example

```
$ sqsq-image foo.sqsq
1: Filesystem01      (17302428 bytes @ 0)
2: Filesystem01-meta (324 bytes @ 17305600)
3: Filesystem02      (277 bytes @ 17309696)
4: Filesystem02-meta (324 bytes @ 17313792)

Found 4 squash filesystems in image
```

sqsq-verify
-----------
`sqsq-verify` is a helper script which utilizes `sqsq-image` to validate the
integrity of one or more of the data filesystems in the passed SQSQ image file.
For more info see `sqsq-verify -h`.

### sqsq-verify example
```
$ sqsq-verify foo.sqsq
Filesystem 1
------------
Blocks     : 4225
Sha256sum  : OK (a7bcbea8fa69...)
Signature  : Unchecked

Filesystem 2
------------
Blocks     : 1
Sha256sum  : OK (dcf3dbeed46f...)
Signature  : Unchecked
```

sqsq-create
-----------
`sqsq-create` is a helper script which allows a user to create SQSQ image files
from a standard Squash file. Multiple SQSQ image files can be joined by simply
concatenating them like `cat rootfs.sqsq additions.sqsq > image.sqsq`.
See `sqsq-create -h` for more info.

### sqsq-create example
```
$ sqsq-create rootfs.sqfs

Attempting to create rootfs.sqfs.bin from rootfs.sqfs
Successfully created rootfs.sqfs.bin
```
