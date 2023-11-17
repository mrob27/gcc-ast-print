# gcc-ast-print

This code produces path-based descriptions of C functions.

## Requirements

On Ubuntu systems, use **apt** to install gcc-9-plugin-dev. On Red Hat
and derived systems (Fedora, CentOS), use **yum** to install
devtoolset-9-gcc-plugin-devel and devtoolset-9-gcc-c++

You might also wish to install "colordiff", but the script will use
normal **diff** for output if **colordiff** is not available.

## Default test

Run the command

```
  ./mk
```

## How it works

The code in **astprint.cc** is a "GCC plugin". It is compiled into a
library (**.so** file) and then this library is used when compiling
another C program. The option **-fplugin=astprintso** tells **gcc** or
**g++** to load the plugin as a shared library and call its functions
at certain points during the compilation process. Each of the plugin's
callback functions is called at a specific time in the compilation
process (some get called multiple times).

The only callback we use is PLUGIN_PRE_GENERICIZE, which "allows [the
plugin] to see low level AST in C and C++ frontends". The callback calls
the gcc API function dump_node()

