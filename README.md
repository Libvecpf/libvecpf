#Libvecpf: The Vector Printf Library
For the GNU/Linux Operating System and GNU C Library (glibc) Version 2.10+  
_Contributed by IBM Corporation_

This library extends ISO C printf so that it may print out vector data types.  The description of the extensions are in the AltiVec Technology Programming Interface Manual.  Below is a paraphrasing of the extensions:

New separator chars (used like flags):

```
,
;
:
_
```

_Note: The default separator is a space unless the 'c' conversion is  
being used.  If 'c' is being used the default separator character  
is a null.  Only one separator character may be specified._  

New size modifiers:

```
vl, vh, lv, hv, v
```

Valid modifiers and conversions (all else are undefined):

```
vl or lv: integer conversions; vectors are composed of four byte vals
vh or hv: integer conversions; vectors are composed of two byte vals
v: integer conversions; vectors are composed of 1 byte vals
v: float conversions; vectors are composed of 4 byte vals
```

## Deviations from the PIM:

 * We only support the default separator; as things are today we can't add flags to printf to support the other separators.

 * We add a new modifier 'vv' to support vector double for VSX, which is new in Power Instruction Set Architecture (ISA) Version 2.06.

## Quick intro to vectors:

Vectors are 16 bytes long and can hold a variety of data types.  These
are the vector data types that Libvecpf knows about at the moment.

### Altivec and VSX:
 
```
Type                            Elements  VMX  VSX
--------------------------------------------------
unsigned and signed char          16       Y    Y
bool char                         16       Y    Y
unsigned and signed short          8       Y    Y
bool short                         8       Y    Y
unsigned and signed int            4       Y    Y
bool int                           4       Y    Y
float                              4       Y    Y
pixel                              8       Y    Y
```

### VSX only:

```
Type                            Elements  VMX  VSX
--------------------------------------------------
double                             2       N    Y
```

The Altivec Programming Interface Manual defines how to print all of these data types except for pixel, which is undefined and double, which is newer than the Altivec PIM.  We add support for double but leave pixel alone.

_Please see [README](https://raw.github.com/Libvecpf/libvecpf/master/README "README") for more information._
