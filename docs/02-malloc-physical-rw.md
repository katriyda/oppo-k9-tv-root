# 02. `/dev/malloc`(MStar mpool)内核物理读写

- 机制首验:2026-10-29;现行 root 方案(`docs/03`)即建立在此机制之上。
- 设备:`OPPOCNM632` / MT9632,内核 4.9.118 aarch64,用户态 32 位(arm)。

## 1. 设备节点与映射公式

`system_app` 域可打开 `/dev/malloc`(节点 `crw-rw-rw-`,**任何 uid 都能开**)。驱动是 MStar `mpool`,可用 ioctl:

```c
struct mpool_info {
    unsigned long long addr;      // bus - 0x20000000
    unsigned long long size;
    unsigned long long interval;
    unsigned char miusel;
    unsigned char pad[7];
};                                // sizeof == 32

ioctl(fd, 0x80204d04, &info);     // MPOOL_IOC_SET
p = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
```

偏移规则:

```text
mmap offset = bus_address - 0x20000000   // 0x20000000 = ARM_MIU0_BUS_BASE
```

`bus_address` 是 MIU0 总线地址(不是内核虚拟地址)。内核镜像 `virt → bus` 换算:

```text
bus = virt - 0xffffff7fe7e00000
```

## 2. 已验证的物理地址(总线地址)

| 对象 | 总线地址 | 验证方式 |
|---|---|---|
| 内核 `_text` | `0x20280000` | 与内核镜像/反汇编交叉 |
| `init_task` | `0x2121a880` | 页内 `comm`/`tasks`/`cred` 字段 |
| `init_cred` | `0x2122d590` | `usage=4`、UID 全 0、`CAP_FULL_SET` |
| `modprobe_path` | `0x2122c0f0` | 指向 `/sbin/modprobe` 字符串 |
| SELinux `enforcing` | `0x2137bdfc` | `sel_write_enforce` 的 `str w0,[x23,#3580]` |

`credview` 实测 `init_cred`:

```text
000000002122d590 04 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
000000002122d5c0 ff ff ff ff 3f 00 00 00 ff ff ff ff 3f 00 00 00
```

## 3. 已验证的写操作

### 3.1 关闭 SELinux enforcing

```text
virtual = 0xffffff800917bdfc  →  bus = 0x2137bdfc
```

写 0 后 `getenforce` = `Permissive`。仅当前启动周期有效(或直到再次 `setenforce 1`)。

### 3.2 cred patch(提权核心)

扫描 MIU0 总线范围,查找 8 个连续的相同 uid/gid 字段(`uid/gid/suid/sgid/euid/egid/fsuid/fsgid`):

- 命中后把 8 个字段置 0、`securebits` 置 0、capability 字段置 `0xffffffff`;
- 立即 `getuid()` 验证;不是 0(说明 patch 的是别的进程)则**原样恢复**;
- 实测日志:`START uid=<n>` → `ROOTCRED <bus>` → `uid=0`。

扫描模式需匹配目标进程自身 uid:
- 8×`1000` → 用于 system_app 域(如 `com.heytap.tvapiservice`);
- 8×`2000` → 用于 adb shell(shell 域)。

### 3.3 `modprobe_path`(未走通)

已能定位并写入自定义路径,但用未知二进制触发 `call_modprobe` 未成功启动服务(`rc=127`),原因未定论;**不是**可用路径。

## 4. 关键代码

```text
tools/research/malloc/session-2026-10-29/  credroot.c rootnow.c ultimate*.c mounter.c su*.c 等
tools/research/malloc/session-2026-10-29/bin/
tools/research/malloc/session-2026-10-30/  rooterm.c(现行,无 fork)与产物
```

构建(容器内):

```bash
arm-linux-gnueabihf-gcc -marm -shared -fPIC -nostdlib \
  -Wl,-soname,librooterm2000.so -o librooterm2000.so rooterm.c
```

## 5. 限制

- 效果限于**当前启动周期**;重启后 SELinux 与 cred 均恢复。
- 写内核数据有 panic/重启风险;`/dev/malloc` 的可见物理范围存在空洞,需逐段(16MB)探测。
- 这是物理内存写原语,**不要**当作安全持久化手段。
