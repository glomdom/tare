# tare

a safer library for c++

## preface

compilers that have been tested is the latest tagged release of llvm's clang,
if something is not supported by X compiler but only Y compiler, and needs a preprocessor to work,
do not contribute.

## features

better documentation can be found in the headers per each implementation

1. [`traits.hpp`](./src/tare/traits.hpp) - helper concepts without depending on most stdlib's huge compile time increasing `type_traits`
2. [`tare::Optional<T>`](./src/tare/optional.hpp) - safe & efficient implementation of returning a value that does/doesnt exist
3. [`tare::Result<T, E>`](./src/tare/result.hpp) - safe & efficient implementation of returning an error or a value
4. [`tare::Span<T>`](./src/tare/span.hpp) - safe implementation of a view over an array