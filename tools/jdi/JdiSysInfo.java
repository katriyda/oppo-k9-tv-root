import com.sun.jdi.*;
import com.sun.jdi.connect.*;
import java.util.*;

public class JdiSysInfo {
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
            ReferenceType base = null;
            for (ReferenceType rt : vm.classesByName("com.mediatek.twoworlds.factory.MtkTvFApiSystemBase")) base = rt;
            if (base == null) { System.out.println("base not loaded"); return; }
            for (Field f : base.allFields()) {
                if (f.isStatic() && f.name().toLowerCase().contains("imp")) {
                    Value v = base.getValue(f);
                    System.out.println("FIELD " + f.name() + " " + f.typeName() + " = " + v);
                    if (v instanceof ObjectReference) {
                        ObjectReference o = (ObjectReference) v;
                        System.out.println("  class=" + o.referenceType().name());
                    }
                }
            }
            for (Method m : base.methods()) {
                if (m.isStatic()) System.out.println("STATIC METHOD " + m.name() + m.signature());
            }
            for (ReferenceType rt : vm.allClasses()) {
                String n = rt.name();
                if (n.contains("twoworlds.factory") && !n.contains("Types") && !n.contains("Enum")) {
                    if (rt instanceof ClassType) {
                        ClassType ct = (ClassType) rt;
                        List<Field> sf = new ArrayList<>();
                        for (Field f : ct.allFields()) if (f.isStatic() && f.typeName().contains("IMtkTvFApiSystem")) sf.add(f);
                        List<ObjectReference> inst = null;
                        try { inst = ct.instances(5); } catch (Throwable t) {}
                        if (!sf.isEmpty() || (inst != null && !inst.isEmpty())) {
                            System.out.println("CLASS " + n + " inst=" + (inst == null ? -1 : inst.size()) + " staticRefs=" + sf.size());
                            for (Field f : sf) System.out.println("  " + f.typeName() + " " + f.name() + " = " + ct.getValue(f));
                        }
                    }
                }
            }
        } finally {
            vm.dispose();
        }
    }
}
