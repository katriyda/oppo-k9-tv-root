import com.sun.jdi.*;
import com.sun.jdi.connect.*;
import com.sun.jdi.event.*;
import com.sun.jdi.request.*;
import java.util.*;

public class JdiSys {
    static Method find(ReferenceType t, String n, String s) {
        for (Method m : t.methodsByName(n)) if (m.signature().equals(s)) return m;
        return null;
    }
    static void ex(InvocationException ie) {
        ObjectReference e = ie.exception();
        System.out.println("EX " + e.referenceType().name());
        try {
            Field f = e.referenceType().fieldByName("detailMessage");
            if (f != null) System.out.println("  msg=" + e.getValue(f));
        } catch (Throwable t) {}
    }
    public static void main(String[] a) throws Exception {
        String action = a[0], path = a[1];
        AttachingConnector conn = null;
        for (AttachingConnector c : Bootstrap.virtualMachineManager().attachingConnectors())
            if (c.name().equals("com.sun.jdi.SocketAttach")) conn = c;
        Map<String,Connector.Argument> ca = conn.defaultArguments();
        ca.get("hostname").setValue("localhost");
        ca.get("port").setValue("8700");
        ca.get("timeout").setValue("5000");
        VirtualMachine vm = conn.attach(ca);
        try {
            ReferenceType svc = vm.classesByName("com.cvte.factory.service.MainService").get(0);
            Method on = find(svc, "onStartCommand", "(Landroid/content/Intent;II)I");
            BreakpointRequest bp = vm.eventRequestManager().createBreakpointRequest(on.location());
            bp.setSuspendPolicy(EventRequest.SUSPEND_EVENT_THREAD);
            bp.enable();
            Thread t = new Thread(() -> {
                try {
                    Thread.sleep(1200);
                    new ProcessBuilder("adb", "-s", "<TV_IP>:5555", "shell",
                        "am", "startservice", "-n", "com.cvte.factory.service/.MainService",
                        "-a", "cvte.factory.intent.action.APIInit").inheritIO().start().waitFor();
                } catch (Exception e) { e.printStackTrace(); }
            });
            t.start();
            vm.resume();
            EventSet es = vm.eventQueue().remove(15000);
            ThreadReference thr = null;
            if (es != null) for (Event ev : es) if (ev instanceof BreakpointEvent) thr = ((BreakpointEvent) ev).thread();
            if (thr == null) { System.out.println("no breakpoint"); vm.resume(); return; }
            ReferenceType base = vm.classesByName("com.mediatek.twoworlds.factory.MtkTvFApiSystemBase").get(0);
            Value imp = base.getValue(base.fieldByName("mtkFApiSystemImp"));
            if (!(imp instanceof ObjectReference)) { System.out.println("no wrapper"); vm.resume(); return; }
            ObjectReference w = (ObjectReference) imp;
            ReferenceType wt = w.referenceType();
            Method m;
            List<Value> args;
            switch (action) {
                case "create":
                    m = find(wt, "createFile", "(Ljava/lang/String;)I");
                    args = Collections.singletonList((Value) vm.mirrorOf(path));
                    break;
                case "write":
                    m = find(wt, "writeFile", "(Ljava/lang/String;Ljava/lang/String;ZZ)I");
                    String content = a[2].startsWith("@")
                        ? new String(java.nio.file.Files.readAllBytes(java.nio.file.Paths.get(a[2].substring(1))),
                            java.nio.charset.StandardCharsets.UTF_8)
                        : a[2];
                    args = Arrays.asList(vm.mirrorOf(path), vm.mirrorOf(content),
                        vm.mirrorOf(Boolean.parseBoolean(a[3])), vm.mirrorOf(Boolean.parseBoolean(a[4])));
                    break;
                case "chmod":
                    m = find(wt, "changeFileMode", "(Ljava/lang/String;I)I");
                    args = Arrays.asList(vm.mirrorOf(path), vm.mirrorOf(Integer.decode(a[2])));
                    break;
                case "copy":
                    m = find(wt, "copyFile", "(Ljava/lang/String;Ljava/lang/String;Z)I");
                    args = Arrays.asList(vm.mirrorOf(path), vm.mirrorOf(a[2]),
                        vm.mirrorOf(Boolean.parseBoolean(a[3])));
                    break;
                case "read":
                    m = find(wt, "readFile", "(Ljava/lang/String;IJ)Ljava/lang/String;");
                    args = Arrays.asList(vm.mirrorOf(path), vm.mirrorOf(0),
                        vm.mirrorOf(a.length > 2 ? Long.decode(a[2]) : 4096L));
                    break;
                case "readbyte":
                    m = find(wt, "readFileByte", "(Ljava/lang/String;IJ)Ljava/util/ArrayList;");
                    args = Arrays.asList(vm.mirrorOf(path), vm.mirrorOf(0),
                        vm.mirrorOf(a.length > 2 ? Long.decode(a[2]) : 256L));
                    break;
                case "remove":
                    m = find(wt, "removeFile", "(Ljava/lang/String;)I");
                    args = Collections.singletonList((Value) vm.mirrorOf(path));
                    break;
                default:
                    System.out.println("unknown action");
                    vm.resume();
                    return;
            }
            if (m == null) { System.out.println("method missing " + action); vm.resume(); return; }
            Value ret = w.invokeMethod(thr, m, args, 0);
            System.out.println(action + " " + path + " -> " + ret);
            if ("readbyte".equals(action) && ret instanceof ObjectReference) {
                ObjectReference list = (ObjectReference) ret;
                ReferenceType lt = list.referenceType();
                Method size = find(lt, "size", "()I");
                Method get = find(lt, "get", "(I)Ljava/lang/Object;");
                if (size == null || get == null) {
                    System.out.println("no list methods on " + lt.name());
                } else {
                    int n = ((IntegerValue) list.invokeMethod(thr, size, Collections.emptyList(), 0)).value();
                    System.out.println("size=" + n);
                    StringBuilder sb = new StringBuilder();
                    for (int i = 0; i < n; i++) {
                        Value v = list.invokeMethod(thr, get, Collections.singletonList(vm.mirrorOf(i)), 0);
                        int b = v instanceof ByteValue ? ((ByteValue) v).value() & 0xff
                              : ((IntegerValue) v).value() & 0xff;
                        sb.append(String.format("%02x", b));
                    }
                    System.out.println("HEX=" + sb);
                }
            }
            bp.disable();
            vm.resume();
        } catch (InvocationException ie) {
            ex(ie);
            vm.resume();
        } finally {
            vm.dispose();
        }
    }
}
