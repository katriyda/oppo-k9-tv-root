import com.sun.jdi.*;
import com.sun.jdi.connect.*;
import com.sun.jdi.event.*;
import com.sun.jdi.request.*;
import java.util.*;

public class JdiCall {
    static Method findMethod(ReferenceType t, String name, String sig) {
        for (Method m : t.methodsByName(name)) {
            if (m.signature().equals(sig)) return m;
        }
        return null;
    }

    static void describe(InvocationException ie) {
        ObjectReference ex = ie.exception();
        System.out.println("  exception class=" + ex.referenceType().name());
        try {
            Field f = ex.referenceType().fieldByName("detailMessage");
            if (f != null) System.out.println("  message=" + ex.getValue(f));
        } catch (Throwable t) { System.out.println("  message read err " + t); }
    }

    public static void main(String[] args) throws Exception {
        int cmd = args.length > 0 ? Integer.parseInt(args[0]) : 1;
        String command;
        if (args.length > 1) {
            command = args[1];
        } else {
            command = new String(java.nio.file.Files.readAllBytes(java.nio.file.Paths.get("/tmp/jdi_payload.txt")),
                    java.nio.charset.StandardCharsets.UTF_8).trim();
        }

        AttachingConnector conn = null;
        for (AttachingConnector c : Bootstrap.virtualMachineManager().attachingConnectors()) {
            if (c.name().equals("com.sun.jdi.SocketAttach")) conn = c;
        }
        Map<String, Connector.Argument> cargs = conn.defaultArguments();
        cargs.get("hostname").setValue("localhost");
        cargs.get("port").setValue("8700");
        cargs.get("timeout").setValue("5000");

        VirtualMachine vm = conn.attach(cargs);
        System.out.println("attached: " + vm.name());
        try {
            ReferenceType svc = vm.classesByName("com.cvte.factory.service.MainService").get(0);
            Method onStart = findMethod(svc, "onStartCommand", "(Landroid/content/Intent;II)I");
            Location loc = onStart.location();
            EventRequestManager erm = vm.eventRequestManager();
            BreakpointRequest bp = erm.createBreakpointRequest(loc);
            bp.setSuspendPolicy(EventRequest.SUSPEND_EVENT_THREAD);
            bp.enable();

            Thread trigger = new Thread(() -> {
                try {
                    Thread.sleep(1200);
                    Process p = new ProcessBuilder("adb", "-s", "<TV_IP>:5555", "shell",
                            "am", "startservice", "-n", "com.cvte.factory.service/.MainService",
                            "-a", "cvte.factory.intent.action.APIInit").inheritIO().start();
                    p.waitFor();
                } catch (Exception e) { e.printStackTrace(); }
            });
            trigger.start();

            vm.resume();
            EventSet es = vm.eventQueue().remove(15000);
            if (es == null) { System.out.println("no event"); return; }
            ThreadReference thr = null;
            for (Event ev : es) {
                if (ev instanceof BreakpointEvent) thr = ((BreakpointEvent) ev).thread();
            }
            if (thr == null) { System.out.println("no breakpoint"); vm.resume(); return; }
            System.out.println("thread: " + thr.name());

            ReferenceType base = vm.classesByName("com.mediatek.twoworlds.factory.MtkTvFApiSystemBase").get(0);
            Field impF = base.fieldByName("mtkFApiSystemImp");
            Value imp = base.getValue(impF);
            System.out.println("imp=" + imp);
            if (!(imp instanceof ObjectReference)) { System.out.println("no wrapper"); bp.disable(); vm.resume(); return; }
            ObjectReference wrapper = (ObjectReference) imp;
            System.out.println("wrapper class=" + wrapper.referenceType().name());

            Method target = findMethod(wrapper.referenceType(), "sendCmdToFactorySvc", "(ILjava/lang/String;)I");
            ReferenceType targetType = wrapper.referenceType();
            if (target == null) {
                for (ReferenceType rt : vm.allClasses()) {
                    if (rt.name().equals("com.mediatek.twoworlds.factory.interfaces.IMtkTvFApiSystemWrapper")) {
                        target = findMethod(rt, "sendCmdToFactorySvc", "(ILjava/lang/String;)I");
                        targetType = rt;
                        break;
                    }
                }
            }
            if (target == null) { System.out.println("method not found"); bp.disable(); vm.resume(); return; }
            System.out.println("invoking " + targetType.name() + "." + target.name() + " cmd=" + cmd + " payload=" + command);

            try {
                Value ret = wrapper.invokeMethod(thr, target,
                        Arrays.asList(vm.mirrorOf(cmd), vm.mirrorOf(command)), 0);
                System.out.println("return=" + ret);
            } catch (InvocationException ie) {
                describe(ie);
            }

            bp.disable();
            vm.resume();
            System.out.println("resumed");
        } finally {
            vm.dispose();
        }
    }
}
