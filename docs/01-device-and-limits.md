# 01. 设备画像、挂载与权限边界

采集:2026-10-28 容器内只读 `adb`;原始输出见 `evidence/2026-10-28/`。

## 1. 构建与产品属性

| 属性 | 值 |
|---|---|
| `ro.product.model` / `device` / `name` | `OPPOCNM632` |
| `ro.product.brand` / `manufacturer` | `OPPO` |
| `ro.board.platform` / `ro.hardware` | `m7332` |
| `ro.build.version.release` / `sdk` | `9` / `28` |
| `ro.build.fingerprint` | `OPPO/OPPOCNM632/OPPOCNM632:9/PPR2.180905.006.A1/2301131057:user/release-keys` |
| `ro.build.type` / `tags` | `user` / `release-keys` |
| `ro.debuggable` / `ro.secure` | `0` / `1` |
| `ro.build.version.security_patch` | `2021-07-05` |
| `ro.product.cpu.abi` / `abilist` | `armeabi-v7a` / `armeabi-v7a,armeabi` |
| `ro.boot.verifiedbootstate` | `orange` |
| `persist.adb.tcp.port` / `service.adb.tcp.port` | `5555` / `5555` |

```text
Linux localhost 4.9.118+ #1 SMP PREEMPT Fri Jan 13 11:22:03 CST 2023 armv8l
SELinux: Enforcing(默认)
```

- `armv8l` = AArch64 内核 + 32 位 userspace;公开的 arm64 GKI 内核利用不可直接用。
- `ro.debuggable=0`,但原厂 `com.cvte.factory.service` 自身带 `DEBUGGABLE` 标志(uid 1000,与 `android.uid.system` 共享 UID)—— 这是 10-28 那条受限 root 通道的入口。

## 2. 分区与挂载

| by-name | 设备 | 挂载点 | 状态 |
|---|---|---|---|
| `MBOOT` | `mmcblk0p1` | — | bootloader |
| `tvcertificate` | `mmcblk0p3` | `/vendor/tvcertificate` | ext4 rw |
| `tvconfig` | `mmcblk0p5` | `/vendor/tvconfig` | ext4 rw |
| `recovery` / `boot` | `mmcblk0p8` / `p9` | — | 未挂载 |
| `vbmeta_a` | `mmcblk0p19` | — | AVB 元数据 |
| `system` | `mmcblk0p27` | — | system-as-root |
| `vendor` | `mmcblk0p29` | `/vendor` | ext4 ro |
| `tvservice` | `mmcblk0p30` | `/mnt/vendor/tvservice` | ext4 ro |
| `factory_a` | `mmcblk0p31` | `/factory` | ext4 **ro** |
| `extconfig` | `mmcblk0p33` | `/vendor/extconfig` | ext4 rw |
| `userdata` | `mmcblk0p34` | `/data` | ext4 rw(nosuid,nodev) |
| `3rd_rw` | `mmcblk0p18` | `/data/vendor/3rd_rw` | ext4 rw |

要点:

- `/system` **不是**独立挂载点,根文件系统是只读 `/dev/root`(ext4 ro)。写 `/system` = 写 rootfs(以 root 身份 `mount -o remount,rw /` 可行,2026-10-30 已验证并在事后恢复为 ro)。
- 空间:根分区 2.4G 已用 98%(剩 ~56M),`/data` 8.7G 用 75%。
- `/data` 挂载带 `nosuid` → setuid 二进制在此无效。

## 3. 权限边界(以 10-28 factory 域 `uid=0` 为例)

通道运行时的能力与上下文:

```text
u:r:factory_service_fusion:s0   Uid: 0 0 0 0   CapEff: 0000003fffffffff   Seccomp: 0
```

可以做:执行 `/vendor/bin/*`、读写 `/vendor/tvcertificate`(create/write/unlink)、写 `/vendor/tvconfig` 既有文件(无 unlink/setattr)、读 `mstar_*` 块设备、经 hwbinder 访问 HIDL HAL。单条命令字符串上限约 255 字节。

做不到(策略无 allow,实测失败):

| 目标 | 结果 |
|---|---|
| 写 `/system`/rootfs | ✗ 需 root 现改挂载;10-28 通道无权 |
| 读/写 `boot`、`vbmeta_a`、`system`、`userdata` 块设备 | ✗ `blk_file` 无 allow |
| 关 SELinux / 写 `enforce` | ✗ `selinuxfs:file` 仅 `getattr` |
| `mknod` 造设备节点 | ✗ 无二进制且无 `capability mknod` |
| 打开 `/dev/binder` | ✗ `binder_device:chr_file` 无 allow(`at_sudo` 因此 SIGABRT) |
| 加载内核模块 | ✗ 无 `system module_load` |
| 读 `/data`、`/sdcard`、`/factory` 内容 | ✗ 域受限 / sdcardfs `add_name` 拒绝 |
| remount `/factory` 为 rw | ✗ ext4 ro + 无 `filesystem mount` |

`mstar_boot_block_device`(eMMC boot0/1)**策略允许写**,但属 bootloader 区域,本次从未写入。

## 4. CVE 与 bootloader 结论

| 候选 | 判断 |
|---|---|
| CVE-2023-20938(binder UAF) | 公开 PoC 面向 GKI 5.4/5.10 arm64,与本机 4.9/32 位厂商内核不兼容,移植成本与风险高 |
| CVE-2021-0920(AF_UNIX UAF) | 无开箱可用 PoC;需要自行编写 |
| CVE-2020-0069(mtk-su) | 针对旧 MTK 手机 SoC,本机 MT9632 + 2021-07 补丁基本不适用 |
| 厂商 TV 专用 CVE | 未发现与本机版本匹配且不破坏系统的公开链 |

**结论:没有找到满足「完整 root + 不搞坏电视」的公开 CVE 方案;本归档未执行任何内核漏洞利用。**
bootloader/fastboot 路线未验证(未执行 `adb reboot bootloader`);在缺少 UART/BROM 恢复与分区级备份前不建议触碰 boot/bootloader。

## 5. 当前清理状态

- `/system`:无任何非原厂文件(10-28/29/30 写入物全部移除),adbd 标签已 `restorecon` 修复,rootfs 已回 ro。
- `/vendor/tvconfig`:测试文件(`ocfree.txt`、`ocfree_id.txt`、`mik.ko`)已通过 HAL `removeFile()` 删除。
- `/vendor/tvcertificate/factory.raw`(3.2MB 测试 dump,非原厂文件)仍留在设备上;主机已有 SHA-256 `a95a83c…` 备份,不影响启动。
- `/data/local/tmp`:仅保留现行 root 工具链三件(见 `docs/03`);研究数据已备份至 `backup-20261030-datalocaltmp/`。
- `/sdcard`:`main2.jar`、`r1.sh`(取 root 用);实验日志已清。
