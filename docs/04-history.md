# 04. 历史路线与失败记录(2026-10-28 → 10-30)

压缩自原 `docs/02/04/06/07/09/10`。原始证据见 `evidence/` 与 `tools/research/`。

## 时间线

### 2026-10-28:factory 命令通道(受限 uid=0)

- 入口:原厂 `com.cvte.factory.service`(uid 1000,system_app 域,**带 DEBUGGABLE**)。
- 手法:`adb forward tcp:8700 jdwp:<pid>` + JDI attach → 调用 `sendCmdToFactorySvc(1, cmd)` → 由 root HAL `factory_svc` 执行。
- 结果:`uid=0`、`CapEff=0000003fffffffff`、上下文 `u:r:factory_service_fusion:s0`,但**域受限**:不能写 rootfs/boot/vbmeta、不能关 SELinux、不能 mknod/binder/module_load;单条命令 ≤255 字节。详见 `docs/01` 第 3 节。
- 一次正常重启后 `/factory` 未挂载、`factory_svc` 未启动,通道失效 → 结论:该通道**不保证重启后存在**。

### 2026-10-28:非分区写入的其它尝试(全部阻断)

| 路线 | 阻断点 |
|---|---|
| `/vendor/tvconfig` 落 `mik.ko` 并加载 | 无 `system module_load` |
| `/dev/miomap`、`/dev/mik!sys`、`/dev/system` 直开 | 节点存在但策略/驱动门禁拒绝(见 `evidence/2026-10-28/miutil-static.txt`) |
| `/sys/kernel/mik/MI_UTIL`(无线可达的物理内存调试接口) | 门禁校验,未拿到可用写原语 |
| HAL `readReg`/`readEmmc`/`writeFile` | 只读或限定路径 |
| USB `at_script` / `at_sudo` | `at_sudo` 需 `/dev/binder`(无 allow)直接 SIGABRT |
| 内核 CVE(binder UAF / AF_UNIX UAF / mtk-su) | 与本机 4.9/32 位厂商内核不匹配,无可用 PoC(见 `docs/01` 第 4 节) |

结论仍是「fork 受限 uid=0 通道」,未取得可写内核/分区的能力。

### 2026-10-29:`/dev/malloc` 物理读写 → 临时 root → 持久化失败

- 关键突破:`system_app` 域可打开 `/dev/malloc`(MStar mpool),获得**内核物理内存读写**(见 `docs/02`)。
- 提权:JDWP/JDI 注入 `librootnow.so` → 扫 8×1000 cred → patch 自身 uid=0 + 满 caps → 写 `enforcing=0` → `Permissive`;再由该进程 fork `rootserver` 监听 `0.0.0.0:4445` 得到 root shell。
- 已知怪象:app 上下文 fork 出的 rootserver 会在 accept 阶段被杀(shell/init 上下文存活)。
- 持久化尝试(错误):改 `/system/bin/adbd` 为包装脚本、新增 `/system/etc/init/rootshell.rc`、写 `/system/bin/{rootserver,su}`。
- 重启后:5555/4445 全部拒绝连接,**设备远程失联**。

### 2026-10-30:恢复 + 重建链路 + 修复根因

- 恢复途径:Termux sshd(8022)仍在 → 用 `dumpsys tvapisvc -cmd` 通道(system_app 域)重新取得代码执行。
- **失联根因**(AVC 实锤):

```text
avc: denied { entrypoint } for path="/system/bin/adbd"
  scontext=u:r:adbd tcontext=u:object_r:system_file  permissive=0
```

  即 10-29 的 `cp` 破坏 `/system/bin/adbd` 的 SELinux 标签(`adbd_exec` → `system_file`),Enforcing 下 init 无法以其为 entrypoint 启动 → adbd 崩溃循环。修复:`restorecon -vF /system/bin/adbd`;2026-10-30 真重启实测 adbd 自恢复。
- 重新提权链路(Enforcing 下全过程合法):
  1. `dumpsys tvapisvc -cmd` 执行任意 shell(system_app 域);
  2. `app_process + CLASSPATH` 跑任意 dex(appdomain 可 exec `zygote_exec`);
  3. `System.load` 需 `apk_data_file` → 载体 APK `com.zz.libhost`;
  4. `/dev/malloc` cred patch + 关 enforcing;
  5. shell 域重跑 8×2000 版,rootserver 在 shell 上下文存活 → 4445 root。
- 收尾:4445 方案被判为不必要的后门 → 改为**终端直取**(`su.sh` + `inheritIO`,无 fork);载体 APK 由 10.4MB 瘦身到 12.7KB;`/system` 与 `/data/local/tmp` 全部清理。

## 教训

1. 改 rootfs 前必须确认**每个被替换文件的 SELinux 标签**(`restorecon`),否则会像 adbd 一样在下次开机崩溃循环。
2. 不要把「重启后重新获取 root」与「开机自启持久化」混为一谈:前者可靠且低风险,后者在无 AVB 校验的设备上仍极易把自己锁死。
3. 无鉴权的 root 监听端口(4445)是后门,任何时候都不应保留。
4. 应用域(system_app/untrusted_app)能 exec 的只有 `apk_data_file`;凡是想加载自建 native 代码的方案,都必须准备一个载体 APK。
5. 设备遗留的 `/dev/malloc`(`crw-rw-rw-` + 可物理读写)是本次全部成果的根基,也是该机型固件的严重设计缺陷。
