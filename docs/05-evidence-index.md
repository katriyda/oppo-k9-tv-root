# 05. 证据索引与复现命令

> **证据文件未随本仓库公开**：原始输出含设备序列号、MAC 等可识别信息，仅本地保留（`_local/evidence/`）。
> 下面记录"采集了什么、用什么命令采集、如何判读"，便于按需自行复现。

原始输出按日期组织：2026-10-28(只读采集)、2026-10-29(临时 root 阶段)。
以下命令在容器 `adb-tv` 内执行;Git Bash 里包一层:

```bash
wslc() { MSYS_NO_PATHCONV=1 "C:/Program Files/WSL/wslc.exe" "$@"; }
D=<TV_IP>:5555
```

## 1. 2026-10-28 采集文件

| 文件 | 内容 |
|---|---|
| `device-props.txt` / `getprop-full.txt` | 关键属性 / 完整 getprop |
| `uname.txt` / `getenforce.txt` | 内核版本 / SELinux 状态 |
| `dumpsys-factory-service.txt` / `app-flags.txt` | factory 应用包信息、flags/uid |
| `by-name-shell.txt` / `mount.txt` / `df.txt` | 分区、挂载、空间 |
| `root-id.txt` / `root-status.txt` | factory 通道 root 进程的 uid/context/caps |
| `jdi-root-run.txt` | JDI attach 与调用日志 |
| `policy-queries.txt` | `sesearch` 对设备策略的查询结果 |
| `factory-image-analysis.txt` | 3.1MB factory dump 的大小/SHA-256/非零字节 |
| `fullroot-probe.txt` | 完整 root 可行性只读探测汇总 |
| `miutil-static.txt` | `MI_UTIL` sysfs、HAL 路径、`/dev/{system,miomap,mik!sys}` 权限与候选排序 |

采集命令(只读):

```bash
wslc exec adb-tv adb -s "$D" shell getprop                 > getprop-full.txt
wslc exec adb-tv adb -s "$D" shell uname -a                > uname.txt
wslc exec adb-tv adb -s "$D" shell getenforce              > getenforce.txt
wslc exec adb-tv adb -s "$D" shell mount                   > mount.txt
wslc exec adb-tv adb -s "$D" shell df -h                   > df.txt
wslc exec adb-tv adb -s "$D" shell ls -l /dev/block/by-name > by-name-shell.txt
wslc exec adb-tv adb -s "$D" shell dumpsys package com.cvte.factory.service \
                                                           > dumpsys-factory-service.txt
```

## 2. 2026-10-29 采集文件

| 文件 | 内容 |
|---|---|
| `addresses.txt` | `/dev/malloc` ioctl、内核/`init_cred`/`enforcing` 地址、cred 扫描 |
| `root-command.txt` | root 进程 `id` 输出 |
| `root-shell.txt` | 4445 root shell、getenforce、rootserver 进程 |
| `post-reboot.txt` | 持久化文件清单与重启后失联记录 |

## 3. factory 通道复现(2026-10-28,历史)

```bash
PID=$(wslc exec adb-tv adb -s "$D" shell pidof com.cvte.factory.service | tr -d '\r' | awk '{print $1}')
wslc exec adb-tv adb -s "$D" forward tcp:8700 "jdwp:$PID"
printf '%s' '<payload≤255字节,如 /vendor/bin/id > /vendor/tvcertificate/eid.txt>' > /tmp/p.txt
wslc exec -i adb-tv tee /tmp/jdi_payload.txt < /tmp/p.txt >/dev/null
wslc exec adb-tv java --add-modules jdk.jdi -cp /tmp JdiCall 1
wslc exec adb-tv adb -s "$D" shell cat /vendor/tvcertificate/eid.txt
wslc exec adb-tv adb -s "$D" forward --remove-all
```

## 4. SELinux 策略查询

设备策略已提取到容器 `/tmp/policy`:

```bash
wslc exec adb-tv sesearch -A -s system_app -t zygote_exec -c file /tmp/policy
wslc exec adb-tv sesearch -A -s system_app -t apk_data_file -c file /tmp/policy
wslc exec adb-tv sesearch -A -t mstar_malloc_device -c chr_file /tmp/policy
wslc exec adb-tv sesearch -T -s system_app /tmp/policy            # type_transition
```

本轮结论举例:`allow appdomain zygote_exec:file { execute execute_no_trans … }`、
`allow appdomain apk_data_file:file { execute map open read … }`、
`allow domain mstar_malloc_device:chr_file { ioctl map open read write }`(shell 域**不在**其中)。

## 5. 现行方案复现

见 `docs/03-root-access.md`;产物与构建配方见 `tools/research/malloc/session-2026-10-30/`。

## 6. 清理与回归检查(2026-10-30 现状)

- `adb forward --list` 为空;
- `/system` 无我们写入的文件:`ls -laZ /system/bin/adbd` 应为 `u:object_r:adbd_exec:s0`;
- `/data/local/tmp` 仅 `su.sh`、`main5.jar`、`librooterm2000.so`;
- `netstat -ltn` 无 4445(设备对外仅出厂 5555);
- `/sdcard`:`main2.jar`、`r1.sh`;
- `adb shell getenforce` 默认 `Enforcing`。
