import com.sun.jdi.*;
import com.sun.jdi.connect.*;
import com.sun.jdi.event.*;
import com.sun.jdi.request.*;
import java.util.*;
import java.nio.file.*;

public class JdiPut {
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
        String src = a[0], dst = a[1];
        byte[] data = Files.readAllBytes(Paths.get(src));
        System.out.println("src=" + src + " bytes=" + data.length + " dst=" + dst);
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
                Collections.singletonList(vm.mirrorOf(dst)), 0);
            // Ensure the parent directory exists.
            try {
                Value parent = file.invokeMethod(thr, find(fc, "getParentFile", "()Ljava/io/File;"), Collections.emptyList(), 0);
                if (parent instanceof ObjectReference) {
                    ((ObjectReference)parent).invokeMethod(thr, find(fc, "mkdirs", "()Z"), Collections.emptyList(), 0);
                }
            } catch (Throwable e2) { System.out.println("mkdirs failed: " + e2); }
            ReferenceType fosC = vm.classesByName("java.io.FileOutputStream").get(0);
            ObjectReference fos;
            try {
                fos = ((ClassType)fosC).newInstance(thr,
                    find(fosC, "<init>", "(Ljava/io/File;)V"),
                    Collections.singletonList((Value)file), 0);
            } catch (InvocationException ie) {
                ObjectReference e = ie.exception();
                System.out.println("fos exception=" + e.referenceType().name());
                try {
                    Field f = e.referenceType().fieldByName("detailMessage");
                    if (f != null) System.out.println("msg=" + e.getValue(f));
                } catch (Throwable x) {}
                bp.disable(); vm.resume(); return;
            }
            // Build byte[] in debuggee from the host data.
            Method write = find(fosC, "write", "([B)V");
            ArrayType at = (ArrayType) write.argumentTypes().get(0);
            ArrayReference arr = at.newInstance(data.length);
            List<Value> vals = new ArrayList<Value>(data.length);
            for (byte b : data) vals.add(vm.mirrorOf(b));
            arr.setValues(vals);
            fos.invokeMethod(thr, write, Collections.singletonList((Value)arr), 0);
            fos.invokeMethod(thr, find(fosC, "close", "()V"), Collections.emptyList(), 0);
            try {
                file.invokeMethod(thr, find(fc, "setExecutable", "(Z)Z"), Collections.singletonList(vm.mirrorOf(true)), 0);
                System.out.println("chmod +x attempted");
            } catch (Throwable e3) { System.out.println("chmod failed: " + e3); }
            System.out.println("wrote");
            bp.disable(); vm.resume();
        } finally { try { vm.dispose(); } catch (Throwable t) {} }
    }
}
