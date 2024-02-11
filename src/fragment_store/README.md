# Fragment Store

Fragments are are pieces of information split into Metadata and Data.
They can be stored seperated or together.
They can be used as a Transport protocol/logic too.

# Store types

### Object Store

Fragment files are stored with the first 2 hex chars as sub folders:
eg:
objects/ (object store root)
  - 5f/ (first 2hex subfolder)
    - 4fffffff (the fragment file without the first 2 hexchars)

### Split Object Store

Same as Object Store, but medadata and data stored in seperate files.
Metadata files have the `.meta` suffix. They also have a filetype specific suffix, like `.json`, `.msgpack` etc.

### Memory Store

Just keeps the Fragments in memory.


# File formats

Files can be compressed and encrypted. Since compression needs the data structure to funcion, it is applied before it is encrypted.

### Text Json

Text json only makes sense for metadata if it's neither compressed nor encrypted. (otherwise its binary on disk anyway, so why waste bytes).
Since the content of data is not looked at, nothing stops you from using text json and ecrypt it, but atleast basic compression is advised.

A Metadata json object has the following keys:
- `enc` Encryption type of the data, if any
- `comp` Compression type of the data, if any

## Binary file headers

### Split Metadata

file magic bytes `SOLMET` (6 bytes)

1 byte encryption type (0x00 is none)

1 byte compression type (0x00 is none)

...metadata here...

note that the ecnryption and compression are for the metadata only.
The metadata itself contains encryption and compression about the data.

### Split Data

(none) all the data is in the metadata file.
This is mostly to allow direct storage for files in the Fragment store without excessive duplication.
Keep in mind to not use the actual file name as the data/meta file name.

### Single fragment

file magic bytes `SOLFIL` (6 bytes)

1 byte encryption type (0x00 is none)

1 byte compression type (0x00 is none)

...metadata here...

...data here...

