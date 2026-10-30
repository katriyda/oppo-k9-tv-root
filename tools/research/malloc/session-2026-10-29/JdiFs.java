import com.sun.jdi.*;
import com.sun.jdi.connect.*;
import com.sun.jdi.event.*;
import com.sun.jdi.request.*;
import java.util.*;

public class JdiFs {
    static Method findMethod(ReferenceType t, String name, String sig) {
        for (Method m : t.methodsByName(name)) if (m.signature().equals(sig)) return m;
        return null;
    }
    static void describe(InvocationException ie) {
        ObjectReference ex = ie.exception();
        System.out.println("  exception=" + ex.referenceType().name());
        try {
            Field f = ex.referenceType().fieldByName("detailMessage");
            if (f != null) System.out.println("  message=" + ex.getValue(f));
        } catch (Throwable t) {}
    }
    static ObjectReference newFile(VirtualMachine vm, ThreadReference thr, String path) throws Exception {
        ReferenceType fc = vm.classesByName("java.io.File").get(0);
        Method ctor = findMethod(fc, "<init>", "(Ljava/lang/String;)V");
        return ((ClassType) fc).newInstance(thr, ctor, Collections.singletonList(vm.mirrorOf(path)), 0);
    }
    static Value call(ThreadReference thr, ObjectReference obj, String name, String sig, List<Value> args) throws Exception {
        Method m = findMethod(obj.referenceType(), name, sig);
        if (m == null) { System.out.println("  no method " + name + sig); return null; }
        return obj.invokeMethod(thr, m, args, 0);
    }

    public static void main(String[] args) throws Exception {
        AttachingConnector conn = null;
        for (AttachingConnector c : Bootstrap.virtualMachineManager().attachingConnectors()) {
            if (c.name().equals("com.sun.jdi.SocketAttach")) conn = c;
        }
        Map<String, Connector.Argument> cargs = conn.defaultArguments();
        cargs.get("hostname").setValue("localhost");
        cargs.get("port").setValue("8700");
        cargs.get("timeout").setValue("5000");
        VirtualMachine vm = conn.attach(cargs);
        System.out.println("attached " + vm.name());
        try {
            ReferenceType svc = vm.classesByName("com.cvte.factory.service.MainService").get(0);
            Method onStart = findMethod(svc, "onStartCommand", "(Landroid/content/Intent;II)I");
            BreakpointRequest bp = vm.eventRequestManager().createBreakpointRequest(onStart.location());
            bp.setSuspendPolicy(EventRequest.SUSPEND_EVENT_THREAD);
            bp.enable();
            Thread trigger = new Thread(() -> {
                try {
                    Thread.sleep(1200);
                    new ProcessBuilder("adb", "-s", "<TV_IP>:5555", "shell",
                            "am", "startservice", "-n", "com.cvte.factory.service/.MainService",
                            "-a", "cvte.factory.intent.action.APIInit").inheritIO().start().waitFor();
                } catch (Exception e) { e.printStackTrace(); }
            });
            trigger.start();
            vm.resume();
            EventSet es = vm.eventQueue().remove(15000);
            ThreadReference thr = null;
            if (es != null) for (Event ev : es) if (ev instanceof BreakpointEvent) thr = ((BreakpointEvent) ev).thread();
            if (thr == null) { System.out.println("no breakpoint"); vm.resume(); return; }
            System.out.println("thread " + thr.name());

            String[] paths = {"/factory/bin/factory_svc", "/factory", "/factory/bin",
                    "/vendor/bin/hw/vendor.mediatek.tv.mtktvfactory@1.0-service"};
            for (String p : paths) {
                try {
                    ObjectReference f = newFile(vm, thr, p);
                    Value ex = call(thr, f, "exists", "()Z", Collections.emptyList());
                    Value len = call(thr, f, "length", "()J", Collections.emptyList());
                    Value can = call(thr, f, "canRead", "()Z", Collections.emptyList());
                    System.out.println(p + " exists=" + ex + " len=" + len + " canRead=" + can);
                    if (p.equals("/factory") || p.equals("/factory/bin")) {
                        Value list = call(thr, f, "list", "()[Ljava/lang/String;", Collections.emptyList());
                        if (list instanceof ArrayReference) {
                            ArrayReference ar = (ArrayReference) list;
                            StringBuilder sb = new StringBuilder();
                            for (Value v : ar.getValues()) sb.append(((StringReference) v).value()).append(" ");
                            System.out.println("  list: " + sb);
                        } else System.out.println("  list=" + list);
                    }
                } catch (InvocationException ie) { System.out.println(p); describe(ie); }
            }

            // copy /factory/bin/factory_svc to /sdcard/factory_svc
            try {
                ObjectReference src = newFile(vm, thr, "/factory/bin/factory_svc");
                long len = ((LongValue) call(thr, src, "length", "()J", Collections.emptyList())).value();
                if (len > 0 && len < 8 * 1024 * 1024) {
                    ReferenceType fisC = vm.classesByName("java.io.FileInputStream").get(0);
                    Method fisCtor = findMethod(fisC, "<init>", "(Ljava/io/File;)V");
                    ObjectReference fis = ((ClassType) fisC).newInstance(thr, fisCtor, Collections.singletonList((Value) src), 0);
                    Method read = findMethod(fisC, "read", "([B)I");
                    ArrayReference buf = ((ArrayType) read.argumentTypes().get(0)).newInstance((int) len);
                    int n = ((IntegerValue) fis.invokeMethod(thr, read, Collections.singletonList((Value) buf), 0)).value();
                    System.out.println("read " + n + " bytes");
                    ReferenceType fosC = vm.classesByName("java.io.FileOutputStream").get(0);
                    Method fosCtor = findMethod(fosC, "<init>", "(Ljava/lang/String;)V");
                    ObjectReference fos = ((ClassType) fosC).newInstance(thr, fosCtor, Collections.singletonList(vm.mirrorOf("/sdcard/factory_svc")), 0);
                    Method write = findMethod(fosC, "write", "([B)V");
                    fos.invokeMethod(thr, write, Collections.singletonList((Value) buf), 0);
                    call(thr, fos, "close", "()V", Collections.emptyList());
                    call(thr, fis, "close", "()V", Collections.emptyList());
                    System.out.println("copied");
                } else {
                    System.out.println("bad length " + len);
                }
            } catch (InvocationException ie) { describe(ie); }

            bp.disable();
            vm.resume();
            System.out.println("resumed");
        } finally {
            vm.dispose();
        }
    }
}
