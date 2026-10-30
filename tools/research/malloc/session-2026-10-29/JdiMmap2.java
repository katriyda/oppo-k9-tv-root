import com.sun.jdi.*;
import com.sun.jdi.connect.*;
import com.sun.jdi.event.*;
import com.sun.jdi.request.*;
import java.util.*;

public class JdiMmap2 {
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
        long off = a.length > 0 ? Long.decode(a[0]) : 0;
        long len = a.length > 1 ? Long.decode(a[1]) : 4096;
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
            ClassType os = (ClassType)vm.classesByName("android.system.Os").get(0);
            Method open = find(os, "open", "(Ljava/lang/String;II)Ljava/io/FileDescriptor;");
            Method mmap = find(os, "mmap", "(JJIILjava/io/FileDescriptor;J)J");
            ObjectReference fd = (ObjectReference)os.invokeMethod(thr, open,
                Arrays.asList(vm.mirrorOf("/dev/miomap"), vm.mirrorOf(0), vm.mirrorOf(0)), 0);
            System.out.println("fd=" + fd + " off=" + off + " len=" + len);
            Value addr = os.invokeMethod(thr, mmap,
                Arrays.asList(vm.mirrorOf(0L), vm.mirrorOf(len), vm.mirrorOf(1), vm.mirrorOf(2), (Value)fd, vm.mirrorOf(off)), 0);
            System.out.println("addr=" + addr);
            Method close = find(os, "close", "(Ljava/io/FileDescriptor;)V");
            try { os.invokeMethod(thr, close, Collections.singletonList((Value)fd), 0); } catch (Throwable x) {}
            bp.disable(); vm.resume();
        } catch (InvocationException ie) {
            System.out.println("EX " + ie.exception().referenceType().name());
            try { Field f=ie.exception().referenceType().fieldByName("detailMessage"); if(f!=null) System.out.println("msg="+ie.exception().getValue(f)); } catch(Throwable x) {}
            vm.resume();
        } finally { try { vm.dispose(); } catch (Throwable t) {} }
    }
}
