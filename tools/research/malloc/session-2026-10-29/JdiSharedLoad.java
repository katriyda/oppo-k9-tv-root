import com.sun.jdi.*;
import com.sun.jdi.connect.*;
import com.sun.jdi.event.*;
import com.sun.jdi.request.*;
import java.util.*;
import java.nio.file.*;

public class JdiSharedLoad {
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
        byte[] data = Files.readAllBytes(Paths.get(a[0]));
        String name = a.length > 1 ? a[1] : "ocfree";
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
            ClassType shC = (ClassType)vm.classesByName("android.os.SharedMemory").get(0);
            Method create = find(shC, "create", "(Ljava/lang/String;I)Landroid/os/SharedMemory;");
            ObjectReference sh = (ObjectReference)shC.invokeMethod(thr, create,
                Arrays.asList(vm.mirrorOf(name), vm.mirrorOf(data.length)), 0);
            System.out.println("SharedMemory=" + sh);
            Method getFdM = find(shC, "getFileDescriptor", "()Ljava/io/FileDescriptor;");
            ObjectReference fd = (ObjectReference)sh.invokeMethod(thr, getFdM, Collections.emptyList(), 0);
            Field fdField = fd.referenceType().fieldByName("descriptor");
            int num = ((IntegerValue)fd.getValue(fdField)).value();
            System.out.println("fd=" + num);
            Method mapRW = find(shC, "mapReadWrite", "()Ljava/nio/ByteBuffer;");
            ObjectReference bb = (ObjectReference)sh.invokeMethod(thr, mapRW, Collections.emptyList(), 0);
            ClassType bbC = (ClassType)vm.classesByName("java.nio.ByteBuffer").get(0);
            Method put = find(bbC, "put", "([B)Ljava/nio/ByteBuffer;");
            ArrayType at = (ArrayType)put.argumentTypes().get(0);
            ArrayReference arr = at.newInstance(data.length);
            List<Value> vals = new ArrayList<Value>(data.length);
            for (byte b : data) vals.add(vm.mirrorOf(b));
            arr.setValues(vals);
            try {
                Value pv = bb.invokeMethod(thr, put, Collections.singletonList((Value)arr), 0);
                System.out.println("put=" + pv);
            } catch (InvocationException ie) {
                ObjectReference e = ie.exception();
                System.out.println("put EX " + e.referenceType().name());
                try { Field f=e.referenceType().fieldByName("detailMessage"); if(f!=null) System.out.println("msg="+e.getValue(f)); } catch(Throwable x){}
            }
            System.out.println("size=" + sh.invokeMethod(thr, find(shC, "getSize", "()I"), Collections.emptyList(), 0));
            String path = "/proc/self/fd/" + num;
            ClassType sys = (ClassType)vm.classesByName("java.lang.System").get(0);
            try {
                sys.invokeMethod(thr, find(sys, "load", "(Ljava/lang/String;)V"),
                    Collections.singletonList(vm.mirrorOf(path)), 0);
                System.out.println("loaded shared");
            } catch (InvocationException ie) {
                ObjectReference e = ie.exception();
                System.out.println("EX " + e.referenceType().name());
                try {
                    Field f = e.referenceType().fieldByName("detailMessage");
                    if (f != null) System.out.println("msg=" + e.getValue(f));
                } catch (Throwable x) {}
            }
            try { sh.invokeMethod(thr, find(shC, "close", "()V"), Collections.emptyList(), 0); } catch (Throwable x) {}
            bp.disable(); vm.resume();
        } finally { try { vm.dispose(); } catch (Throwable t) {} }
    }
}
