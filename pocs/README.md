# PoC inputs

Keep minimized crashing inputs here only after validating them locally against the
matching fuzzer binary. Do not place crashing files in `fuzz/corpus/`; corpus
files should help coverage without immediately failing normal fuzz startup.

Example local command after building fuzzers:

```bash
./stream_fuzzer pocs/<input>.wvl
```
