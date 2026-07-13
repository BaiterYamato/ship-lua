> English translation of [docs/api/cpp-codegen.md](cpp-codegen.md). The Portuguese document remains the canonical project record.

# Generation of the C++ contract

`tools/generate_cpp_api.py` transforms the three canonical schemas into
`generated/include/shiplua/generated/ApiBindings.h`.

The header contains:

- public types materializable in C++;
- API and schema version;
- error codes;
- IDs and descriptors of functions, arguments and returns;
- event and payload descriptors;
- capabilities and support per host.

It is binding metadata, not an automatic exposure of game headers.
No pointers, arbitrary ABI C or internal layout can be generated.

## Update

After changing a validated schema:

```powershell
rtk python tools/generate_cpp_api.py
```

The generated file is versioned to allow review. The reproducible gate is:

```powershell
rtk python tools/generate_cpp_api.py --check
```

CTest performs this gate and generator tests. Manually change the header
without changing the schemas, or forgetting to regenerate it after a change, it fails
validation.
