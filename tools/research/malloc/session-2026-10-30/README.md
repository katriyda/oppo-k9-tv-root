# 稳定重取 root（rev3：最小 APK + 无 fork 终端直取）

## 结论

| 项目 | 状态 |
|---|---|
| 10-29 失联根因 | `cp` 破坏 `/system/bin/adbd` 标签（`system_file`≠`adbd_exec`）→ 开机 enforcing 下 entrypoint 拒绝 → adbd 崩溃循环；`restorecon` 修复，真重启实测自恢复 ✅ |
| **重启后 enforcing 下重取完整 root** | ✅ 实测端到端（含 `setenforce 1` 复测） |
| 终端直取 root | ✅ `sh /data/local/tmp/su.sh [cmd]` → 当前 adb shell 终端即 `uid=0` |
| 任何监听端口 | ✅ **无**（两版 so 均已去除 fork；`netstat` 4445 = 0） |
| 常驻进程 / 开机自启 | ✅ 无 |
| 载体 APK | ✅ 由 10.4MB 瘦身为 **12.7KB**（删除原厂 libutopia 18MB 等无关内容） |

## 流程（重启后照抄）

```bash
adb connect <TV_IP>:5555

# Step 1 — system_app 域：cred patch(8×1000) + 全局 Permissive（~40s，无 fork）
adb shell 'dumpsys tvapisvc -cmd "sh /sdcard/r1.sh"'
#   r1.sh 内容: export CLASSPATH=/sdcard/main2.jar ; app_process /system/bin Main
# 验证: adb shell cat /sdcard/rooterm.log  → START uid=1000 / ROOTCRED / uid=0 / DONE;getenforce → Permissive

# Step 2 — shell 域：终端直取 root
adb shell
$ sh /data/local/tmp/su.sh          # 交互 root shell；或 sh /data/local/tmp/su.sh id
```

## 原理

1. `dumpsys tvapisvc -cmd "sh <脚本>"` → `com.heytap.tvapiservice`（uid 1000，system_app 域）执行任意 shell（doShell 按空格裸分词，复杂命令必须走脚本文件）。
2. `app_process`(zygote_exec) 允许 appdomain `execute_no_trans` → `CLASSPATH=<jar> app_process /system/bin Main` 在调用者域跑任意 dex。
3. system_app 只能 exec `apk_data_file` → Step 1 必须借已安装的载体 APK 的 lib 目录（**本仓库 `libhost2-signed.apk`，12.7KB，包名 `com.zz.libhost`**）。shell 域可直接 exec `shell_data_file`（`/data/local/tmp`）。
4. `librooterm1000/2000.so`：扫 8×1000 / 8×2000 的 cred → patch uid0+满 caps → 写 `enforce=0`，**不起任何子进程**；Step 2 的 Java 侧用 `ProcessBuilder.inheritIO()` 把终端交给 `/system/bin/sh`。
5. shell 域在 enforcing 下不能开 `/dev/malloc`（策略无 allow），故 Step 1 先拿全局 Permissive。

## 设备上保留物（全部）

```text
已装 APK com.zz.libhost 12.7KB → /data/app/com.zz.libhost-c2b1H8Gglnk61WucCqcfFQ==/lib/arm/librooterm1000.so
/sdcard/main2.jar         # Step 1 dex（指向上面的路径）
/sdcard/r1.sh             # Step 1 脚本（可重建）
/data/local/tmp/su.sh     # Step 2 入口
/data/local/tmp/main5.jar # Step 2 dex
/data/local/tmp/librooterm2000.so
```

## 构建方法（wslc 容器 `adb-tv`）

```bash
# 1) 无 fork cred patch so（源码见 rooterm.c；改 "!=1000" / "!=2000" 出两版）
arm-linux-gnueabihf-gcc -marm -shared -fPIC -nostdlib \
  -Wl,-soname,librooterm1000.so -o librooterm1000.so rooterm.c

# 2) dex（Main.java 见本目录；MainA.java 为 Step 1 版）
javac -d cls Main.java && java -cp /tmp/android-14/lib/d8.jar com.android.tools.r8.D8 \
  --release --min-api 28 --output cls cls/Main.class
(cd cls && jar cf main.jar classes.dex)

# 3) 最小 APK（AndroidManifest.xml 见本目录；必须有 <application>，否则 PMS 报
#    INSTALL_FAILED_INTERNAL_ERROR: Error deriving application ABI）
aapt package -f -M AndroidManifest.xml -I android.jar -F libhost2.apk
aapt add libhost2.apk lib/armeabi-v7a/librooterm1000.so classes.dex
zipalign -f 4 libhost2.apk aligned.apk && apksigner sign --ks ks ... --out libhost2-signed.apk aligned.apk
```

## 产物校验（本目录）

旧载体 APK（10-29 版，重装前备份，仅归档不再使用）：
libhost-old-10453351.apk  fbd15fd55dc41e7e6f468d7cb20b941a4d9c951b13193355bc18b8f9f510b462
  - 10,453,351 字节 / 114 条目 / 签名 CN=zz（与 10-29 当时的 key 一致）
  - 内含原厂 libutopia.so(18.3MB，md5 dd52ed4baa2d5c5c905ec3cd122d6b73)、libmi3/libteec/libiniparser、
    某现成 app 的 classes.dex(647KB) 与约 100 个探测 so；仅 librootnow.so(9.7KB) 曾被动用
  - 现状：设备上已被 12.7KB 版本取代（旧包卸载）

新产物校验：
```text
libhost2-signed.apk  a1b97ba48c6594ae9831219e1151a24ca5694ead38d74e6dc65ce35fac656ecc
main2.jar            35a97a9476b613a70764cd9c0a0d5a08680d7702661d8a41a6b46284fca42b70
main5.jar            12b4fb9bfb3dc1ae2b34d6a30fbb28b2bec6dd83812b08de14a9ed22c5d4508d
librooterm1000.so    b22edf5386c51ddf5d3e31fa87de67e6e9066115aa3f9ae6a3bab1324134d122
librooterm2000.so    2e29b3650d657f74da63126782323810cadb15f034bd1ba40da630e84d3f08a0
```

## 清理与安全

- `/system`：无任何非原厂文件；adbd 标签已修复；rootfs ro；`/init.cvteat.rc` 未被改动。
- 旧 APK（10.4MB，含原厂 `libutopia.so` 等）在重装前已备份到容器 `/tmp/bak-libhost-1793328632.apk`；卸载旧包同时释放了 `/data` 上 ~19MB。
- 设备对外仅剩出厂 `5555`（adb，shell 权限）；无 root 端口、无 root 进程常驻。
- `/dev/malloc` 为 `crw-rw-rw-`，配合 Permissive 任意 app 可物理读写内核内存 —— 设备级缺陷。
- cred patch 采用 patch→getuid→失败回滚，brick 风险低（本质仍是内核物理内存写）。
