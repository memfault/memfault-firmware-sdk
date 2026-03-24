#!/usr/bin/env bash

# Fastcov wants a single executable when issuing gcov commands, but when using
# clang/llvm, we need to call llvm-cov with the 'gcov' command, so hence this
# little wrapper

${LLVM_COV:-llvm-cov} gcov "$@"
