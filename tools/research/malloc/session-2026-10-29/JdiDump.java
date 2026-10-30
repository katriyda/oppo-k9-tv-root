import com.sun.jdi.*;
import com.sun.jdi.connect.*;
import com.sun.jdi.event.*;
import com.sun.jdi.request.*;
import java.util.*;
import java.nio.file.*;

public class JdiDump {
    static Method find(ReferenceType t, String n, String s) {
        for (Method m : t.methodsByName(n)) if (m.signature().equals(s)) return m;
        return null;
    }
    static ThreadReference waitBreak(VirtualMachine vm, int timeout) throws Exception {
        EventSet es = vm.eventQueue().remove(timeout);
        if (es == null) return null;
        for (Event ev : es) if (ev instanceof BreakpointEvent) return ((BreakpointEvent) ev).thread();
        return null;
    }
    public static void main(String[] a) throws Exception {
        String path = a[0];
        int max = a.length > 1 ? Integer.parseInt(a[1]) : 1048576;
        AttachingConnector conn = null;
        for (AttachingConnector c : Bootstrap.virtualMachineManager().attachingConnectors())
            if (c.name().equals("com.sun.jdi.SocketAttach")) conn = c;
        Map<String,Connector.Argument> ca = conn.defaultArguments();
        ca.get("hostname").setValue("localhost"); ca.get("port").setValue("8700"); ca.get("timeout").setValue("5000");
        VirtualMachine vm = conn.attach(ca);
        try {
            ReferenceType svc = vm.classesByName("com.cvte.factory.service.MainService").get(0);
            Method on = find(svc, "onStartCommand", "(Landroid/content/Intent;II)I");
            BreakpointRequest bp = vm.eventRequestManager().createBreakpointRequest(on.location());
            bp.setSuspendPolicy(EventRequest.SUSPEND_EVENT_THREAD); bp.enable();
            Thread t = new Thread(() -> { try { Thread.sleep(1200);
                new ProcessBuilder("adb","-s","<TV_IP>:5555","shell","am","startservice",
                    "-n","com.cvte.factory.service/.MainService","-a","cvte.factory.intent.action.APIInit")
                    .inheritIO().start().waitFor(); } catch (Exception e) { e.printStackTrace(); } });
            t.start(); vm.resume();
            ThreadReference thr = waitBreak(vm, 15000);
            if (thr == null) { System.out.println("no breakpoint"); vm.resume(); return; }
            ReferenceType fc = vm.classesByName("java.io.File").get(0);
            ObjectReference file = ((ClassType)fc).newInstance(thr,
                find(fc, "<init>", "(Ljava/lang/String;)V"),
                Collections.singletonList(vm.mirrorOf(path)), 0);
            ReferenceType fisC = vm.classesByName("java.io.FileInputStream").get(0);
            ObjectReference fis = ((ClassType)fisC).newInstance(thr,
                find(fisC, "<init>", "(Ljava/io/File;)V"),
                Collections.singletonList((Value)file), 0);
            Method read = find(fisC, "read", "([B)I");
            ArrayType at = (ArrayType) read.argumentTypes().get(0);
            ArrayReference buf = at.newInstance(65536);
            ClassType b64 = null;
            Method encToString = null;
            long total = 0;
            while (total < max) {
                Value nv = fis.invokeMethod(thr, read, Collections.singletonList((Value)buf), 0);
                int n = ((IntegerValue)nv).value();
                if (n <= 0) break;
                StringBuilder sb = new StringBuilder(n * 2);
                for (Value v : buf.getValues(0, n)) {
                    int b = ((ByteValue)v).value() & 0xff;
                    sb.append(String.format("%02x", b));
                }
                System.out.println(sb);
                total += n;
            }
            System.err.println("total=" + total);
            fis.invokeMethod(thr, find(fisC, "close", "()V"), Collections.emptyList(), 0);
            bp.disable(); vm.resume();
        } finally { vm.dispose(); }
    }
}
