# Malloc / mpool root research archive (2026-10-29)

This directory contains the 32-bit ARM native probes and JDI tooling used to
obtain a temporary root shell on the OPPO K9 TV (`<TV_IP>`).

## Layout

```text
session-2026-10-29/          extracted session snapshot
  *.c                        all C probes from the container /tmp
  *.java                     all JDI Java clients from the container /tmp
  bin/                       selected static binaries and shared libraries
```

`bin/` includes:

```text
rootserver2, rootserver3    ARM static root servers
su2, su4, su5, su6, su7     setuid/su experiments
mounter, setnsexec2         setns/mount experiments
setnslog, nnp, euid         diagnostics
librootnow.so               final temporary-root library
libcredroot.so              cred scanner
libultimate2.so             root + security experiment
libultimate4.so             root + security/exec experiment
librootcmd.so               root command executor
libcredview.so              cred layout viewer
```

## Key sources

| File | Purpose |
|---|---|
| `credroot.c` | scan cred structures for 8x1000, patch UID/GID/caps |
| `rootnow.c` | cred root, write SELinux enforcing=0, exec rootserver |
| `rootcmd.c` | root, disable enforcing, run a root shell command |
| `rootserver2.c` / `rootserver3.c` | bind 0.0.0.0:4445, accept, fork `/system/bin/sh -i` |
| `credview.c` | dump live cred and `init_cred` |
| `mounter.c` / `setnsexec2.c` | setns to init mount namespace, tmpfs/suid experiments |
| `su*.c` | setuid/su experiments; not a reliable final root path |

## Build

Shared library (JDI `System.load`):

```bash
arm-linux-gnueabihf-gcc -marm -shared -fPIC -nostdlib \
  -Wl,-soname,librootnow.so -o librootnow.so rootnow.c
```

Static binary:

```bash
arm-linux-gnueabihf-gcc -marm -static -nostdlib -Wl,-e,_start \
  -o rootserver3 rootserver3.c
```

APK packaging used:

```bash
cp librootnow.so /tmp/libhost/lib/armeabi-v7a/
cd /tmp/libhost
aapt add /tmp/libhost.apk lib/armeabi-v7a/librootnow.so
apksigner sign --min-sdk-version 21 --ks /tmp/zz.keystore \
  --ks-pass pass:android --key-pass pass:android \
  --out /tmp/libhost-signed.apk /tmp/libhost.apk
adb install -r /tmp/libhost-signed.apk
```

## JDI load

```bash
adb forward tcp:8700 jdwp:<factory-service-pid>
PKG=$(adb shell "pm path com.zz.libhost" | sed 's/package://;s#/base.apk##')
java -cp /tmp JdiLoad "$PKG/lib/arm/librootnow.so"
```

## Status (2026-10-30)

The 2026-10-29 flow above (JDWP/JDI + 4445 rootserver + `/system` persistence) is
**obsolete and has been removed from the device** — it broke adbd via a wrong
SELinux label on `/system/bin/adbd` and locked the TV out after reboot.

The current, verified flow uses no port and no daemon:

```text
session-2026-10-30/   ← 现行: 最小载体 APK(12.7KB) + app_process/CLASSPATH +
                        /dev/malloc cred patch + su.sh(inheritIO)
```

See `docs/03-root-access.md` (procedure), `docs/02-malloc-physical-rw.md`
(mechanism) and `docs/04-history.md` (why the old flow failed).

## Caveat

These are offensive research artifacts. They write kernel physical memory and
can reboot or brick the device. Never leave a listening root service or an
unverified `/system` change on the device.
