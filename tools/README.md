# 工具与复现

## 1. `activate.sh`

用途：从 PC 重新建立 stock factory root 通道，执行一条命令，取回输出并清理。

```bash
cd /c/<path-to-repo>
bash tools/activate.sh 'id'
bash tools/activate.sh 'ls -l /vendor/tvcertificate'
OCFREE_WAIT=5 bash tools/activate.sh 'cat /proc/partitions'
```

环境变量：

| 变量 | 默认 | 说明 |
|---|---|---|
| `OCFREE_CONTAINER` | `adb-tv` | wslc 容器名 |
| `OCFREE_SERIAL` | `<TV_IP>:5555` | 目标设备 |
| `OCFREE_JDWP_PORT` | `8700` | adb forward 本地端口 |
| `OCFREE_WAIT` | `2` | 调用后等待输出文件的秒数 |
| `WSL_C` | `C:/Program Files/WSL/wslc.exe` | wslc.exe 路径 |

脚本行为：

1. `adb connect` + 等待 `device`；
2. 启动 `com.cvte.factory.service/.MainService`，取 pid；
3. 编译 `JdiCall.java`（如果容器 `/tmp/JdiCall.class` 不存在）；
4. `adb forward tcp:8700 jdwp:<pid>`；
5. 把命令包装成输出重定向，`JdiCall` 调用 `sendCmdToFactorySvc(1, ...)`；
6. `adb shell cat` 取回输出；
7. 通过第二次 root 调用删除输出文件，移除 port forward。

限制：包装后的 payload 必须 ≤ 约 255 字节；超长命令请先写到 `/vendor/tvcertificate` 再执行脚本。

注意：`jdi/JdiCall.java` 内部把触发用的 adb 目标硬编码为 `<TV_IP>:5555`；如果改 `OCFREE_SERIAL`，需要同步改源码。

## 2. 容器 `adb-tv`

本次研究使用的容器包含 `adb`、JDK、`sesearch`。容器丢失时可重建：

```bash
wslc run -d --name adb-tv debian:bookworm-slim sleep infinity
wslc exec adb-tv apt-get update
wslc exec adb-tv apt-get install -y android-tools-adb default-jdk-headless setools
```

重建后：

```bash
wslc exec adb-tv adb connect <TV_IP>:5555
```

首次连接需要在电视上确认“允许 USB 调试”。容器不是持久化存储；重建后 `JdiCall.class` 会重新编译，`/tmp/policy` 也会丢失；策略查询需要从设备重新提取（采集命令见 `../docs/05-evidence-index.md`）。

## 3. JDI 源码

`jdi/` 下是本次使用的 JDI 客户端与探针：

| 文件 | 用途 | 状态 |
|---|---|---|
| `JdiCall.java` | 断点 `MainService.onStartCommand`，反射调用 `sendCmdToFactorySvc` | 本次使用，已实测 |
| `JdiInfo.java` | 读取属性/包信息 | 研究探针 |
| `JdiSysInfo.java` | 系统/存储信息 | 研究探针 |
| `JdiFs.java` | 文件系统/挂载探测 | 研究探针，含失败路径 |
| `JdiSudo.java` | `at_sudo`/binder 尝试 | 研究探针，实测失败 |
| `JdiProbe.java` | 类/方法探测 | 研究探针 |
| `JdiBreak.java` | 断点/注入实验 | 研究探针 |

这些文件含设备特定类名、路径或目标地址，不是通用工具。`JdiCall` 是唯一被 `activate.sh` 使用的客户端。

## 4. `research/`

- `fsclient.c` — 连接抽象 UNIX socket `FACTORY_SERVER` 的 C 客户端；
- `fsprobe.py` — 向 `127.0.0.1:19001` 发送 factory 命令的探针；
- `jdwp_test.py` — JDWP 握手测试；
- `jdb_cmds.txt` — jdb 评估命令记录。

这些是研究辅助，未编译/未纳入自动流程。

## 5. 安全

- 该通道依赖原厂可调试系统应用；任何能连接 ADB 的人都能复现。
- 不使用时关闭网络 ADB/开发者选项。
- 本目录不包含固件、APK 或分区镜像；不会自动刷机。
- 不要用它写 bootloader/factory/system；相关风险见 `../docs/01-device-and-limits.md`。

## 6. 现状(2026-10-30)

`activate.sh` 对应的 factory 通道属于 2026-10-28 的历史方案，重启后未必存在。
当前的取 root 方案见 `../docs/03-root-access.md`：`dumpsys tvapisvc` + `app_process/CLASSPATH`
+ 载体 APK + `/dev/malloc` cred patch，终端直取 root、无监听端口。
产物与构建配方在 `research/malloc/session-2026-10-30/`。
