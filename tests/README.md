# Embedded unit tests!

## Setup

You'll need a c/c++ compiler and Make, as well as
[CPPUTest](https://cpputest.github.io) installed.

1. Install the CppUtest library and the lcov tool:
   - Linux - `sudo apt-get install cpputest lcov`
   - OSX - `brew install cpputest && brew install lcov`
2. Optionally, install the python packages with `pip`:
   - `pip install -r requirements.txt`

## Running tests

If the python packages were installed, you can use invoke to run the tests:

`inv test`

You can also call `make` directly from within this directory:

`make`

## Directory structure

```plaintext
├── Makefile // Invokes all the unit tests
├── MakefileWorker.mk // Comes from CppUTest itself
├── MakefileWorkerOverrides.mk // memfault injected overrides
├── build
│   [...] // Where all the tests wind up
├── fakes
│   // fakes for unit tests
├── mocks
│   // mocks for unit tests
├── makefiles // Each c file you unit test has a makefile here
│   └── Makefile_<module_name>.mk
|   [...]
└── src // test source files
└── test_*
```

## Adding a test

- Add a new test makefile under test/makefiles/. These just list the sources you
  will compile
- Add a new test file under tests/src for the module you want to test
- `inv test`
