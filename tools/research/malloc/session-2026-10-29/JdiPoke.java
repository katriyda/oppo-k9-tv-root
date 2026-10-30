import com.sun.jdi.*;
import com.sun.jdi.connect.*;
import com.sun.jdi.event.*;
import com.sun.jdi.request.*;
import java.util.*;

public class JdiPoke {
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
        long addr = Long.decode(a[0]);
        byte[] data = a.length > 1 ? hex(a[1]) : null;
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
            ObjectReference mem = (ObjectReference)os.invokeMethod(thr, find(os,"open","(Ljava/lang/String;II)Ljava/io/FileDescriptor;"),
                Arrays.asList(vm.mirrorOf("/proc/self/mem"), vm.mirrorOf(2), vm.mirrorOf(0)), 0);
            Method pread = find(os,"pread","(Ljava/io/FileDescriptor;[BIIJ)I");
            Method pwrite = find(os,"pwrite","(Ljava/io/FileDescriptor;[BIIJ)I");
            ArrayType at = (ArrayType)pread.argumentTypes().get(1);
            ArrayReference buf = at.newInstance(64);
            try {
                Value n = os.invokeMethod(thr, pread, Arrays.asList((Value)mem,(Value)buf,vm.mirrorOf(0),vm.mirrorOf(64),vm.mirrorOf(addr)),0);
                System.out.println("pread n=" + n);
                for (Value v : buf.getValues()) System.out.printf("%02x", ((ByteValue)v).value()&0xff);
                System.out.println();
            } catch (InvocationException ie) { System.out.println("pread"); ex(ie); }
            if (data != null) {
                ArrayReference wb = at.newInstance(data.length);
                List<Value> vals = new ArrayList<>();
                for (byte b : data) vals.add(vm.mirrorOf(b));
                wb.setValues(vals);
                try {
                    Value n = os.invokeMethod(thr, pwrite, Arrays.asList((Value)mem,(Value)wb,vm.mirrorOf(0),vm.mirrorOf(data.length),vm.mirrorOf(addr)),0);
                    System.out.println("pwrite n=" + n);
                } catch (InvocationException ie) { System.out.println("pwrite"); ex(ie); }
            }
            bp.disable(); vm.resume();
        } finally { try { vm.dispose(); } catch (Throwable t) {} }
    }
    static byte[] hex(String s) {
        if ((s.length() & 1) != 0) s = "0" + s;
        byte[] b = new byte[s.length()/2];
        for (int i=0;i<b.length;i++) b[i]=(byte)Integer.parseInt(s.substring(2*i,2*i+2),16);
        return b;
    }
}
