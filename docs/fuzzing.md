# Fuzzing workflow notes

The ClusterFuzzLite entry point is `.clusterfuzzlite/build.sh`. It expects the
standard OSS-Fuzz/ClusterFuzzLite environment variables:

- `$SRC`: repository root
- `$WORK`: scratch build directory
- `$OUT`: fuzz target output directory
- `$CC`, `$CXX`, `$CFLAGS`, `$CXXFLAGS`
- `$LIB_FUZZING_ENGINE`

The build script compiles common project objects once, links all harnesses, and
copies the dictionary and seed corpus archives into `$OUT`.

Local OSS-Fuzz helper flow:

```bash
cd /path/to/oss-fuzz
export PATH_TO_PROJECT=/absolute/path/to/wayvault
python infra/helper.py build_image --external "$PATH_TO_PROJECT"
python infra/helper.py build_fuzzers --external "$PATH_TO_PROJECT" --sanitizer address
python infra/helper.py check_build --external "$PATH_TO_PROJECT" --sanitizer address
python infra/helper.py run_fuzzer --external --corpus-dir=/tmp/wayvault-stream "$PATH_TO_PROJECT" stream_fuzzer
```

Do not place known crashing inputs in `fuzz/corpus/`. Keep minimized PoCs in
`pocs/` only after validating them locally.
