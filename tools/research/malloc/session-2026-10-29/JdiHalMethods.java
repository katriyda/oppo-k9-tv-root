import com.sun.jdi.*;
import com.sun.jdi.connect.*;
import com.sun.jdi.event.*;
import com.sun.jdi.request.*;
import java.util.*;

public class JdiHalMethods {
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
        String filter = a.length > 0 ? a[0] : "";
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
            ReferenceType base = vm.classesByName("com.mediatek.twoworlds.factory.MtkTvFApiSystemBase").get(0);
            ObjectReference w = (ObjectReference)base.getValue(base.fieldByName("mtkFApiSystemImp"));
            ReferenceType wt = w.referenceType();
            System.out.println("TYPE " + wt.name());
            for (Field f : wt.fields()) System.out.println("FIELD " + f.name() + " " + f.typeName());
            for (String n : new String[]{"writeFile","writeFileByte","readFile","readFileByte","readEmmc","writeEmmc","readReg","writeReg","createFile","copyFile","removeFile","changeFileMode"}) {
                for (Method m : wt.methodsByName(n)) System.out.println("N " + m.name() + m.signature());
            }
            System.out.println("ALLMETHODS");
            for (Method m : wt.allMethods()) {
                if (filter.isEmpty() || m.name().toLowerCase().contains(filter.toLowerCase()))
                    System.out.println("A " + m.name() + m.signature());
            }
            Field pf = null;
            for (Field f : wt.allFields()) if (f.name().equals("sysHidlProxy")) pf=f;
            if (pf != null) {
                ObjectReference proxy = (ObjectReference)w.getValue(pf);
                System.out.println("PROXY " + proxy.referenceType().name());
                for (Method m : proxy.referenceType().methods()) {
                    if (filter.isEmpty() || m.name().toLowerCase().contains(filter.toLowerCase()))
                        System.out.println("P " + m.name() + m.signature());
                }
            }
            for (Method m : wt.methods()) {
                if (filter.isEmpty() || m.name().toLowerCase().contains(filter.toLowerCase()))
                    System.out.println(m.name() + m.signature());
            }
            bp.disable(); vm.resume();
        } finally { try { vm.dispose(); } catch (Throwable e2) {} }
    }
}
