# Trace allocations with a LD_PRELOAD library

Build the library with:

```
gcc --shared -nostartfiles -fPIC -g log-malloc.c -o log-malloc.so -lpthread -ldl
```

To trace a program, run:

```
LD_PRELOAD=./log-malloc.so command args ...
```

Example, showing a 24 bytes malloc along with the backtrace of its call site.

```console
$ LD_PRELOAD=./log-malloc.so jk run test-issue-0071.js
[...]

malloc 24        0x22a48e0 [bt] Execution path:
./log-malloc.so(+0xb4e)[0x7f4e07de4b4e]
./log-malloc.so(malloc+0xa3)[0x7f4e07de4c6b]
/usr/lib/x86_64-linux-gnu/libstdc++.so.6(_Znwm+0x18)[0x7f4e078d2e78]
jk(_ZN2v88internal28DefaultDeserializerAllocator17DecodeReservationESt6vectorINS0_14SerializedData11ReservationESaIS4_EE+0xdf)[0x1061c7f]
jk(_ZN2v88internal12DeserializerINS0_28DefaultDeserializerAllocatorEEC2IKNS0_12SnapshotDataEEEPT_b+0x172)[0xd5e822]
jk(_ZN2v88internal8Snapshot10InitializeEPNS0_7IsolateE+0x11f)[0xd5c90f]
jk(_ZN2v87Isolate10InitializeEPS0_RKNS0_12CreateParamsE+0x11a)[0xa0082a]
jk(_ZN2v87Isolate3NewERKNS0_12CreateParamsE+0x2a)[0xa009da]
jk(worker_new+0xf3)[0x9ce2d3]
jk(_cgo_930f4c6fb697_Cfunc_worker_new+0x16)[0x9ccf46]
jk[0x6dcc00]

[...]
```
