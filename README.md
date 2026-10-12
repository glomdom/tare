# tare

a safer library for c++

## preface

compilers that have been tested is the latest tagged release of llvm's clang,
if something is not supported by X compiler but only Y compiler, and needs a preprocessor to work,
do not contribute.

## features

1. [`Optional<T>`](./src/tare/optional.hpp) - ever wanted rust's `Option<T>` but its cpp-ified or whatever? use `tare::Optional`
2. traits - helper concepts without depending on most stdlib's huge compile time increasing `type_traits`