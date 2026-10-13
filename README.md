# tare

a safer library for c++

## preface

compilers that have been tested is the latest tagged release of llvm's clang,
if something is not supported by X compiler but only Y compiler, and needs a preprocessor to work,
do not contribute.

## features

better documentation can be found in the headers per each implementation

1. [`tare::Optional<T>`](./src/tare/optional.hpp) - safe & efficient implementation of returning a value that does/doesnt exist
2. [`tare::Result<T, E>`](./src/tare/result.hpp) - safe & efficient implementation of returning an error or a value
3. traits - helper concepts without depending on most stdlib's huge compile time increasing `type_traits`