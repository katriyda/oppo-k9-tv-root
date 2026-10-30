import com.sun.jdi.*;
import com.sun.jdi.connect.*;
import java.util.*;

public class JdiInfo {
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
        System.out.println("attached " + vm.name() + " canGetInstanceInfo=" + vm.canGetInstanceInfo());
        try {
            vm.suspend();
            List<ReferenceType> hits = new ArrayList<>();
            for (ReferenceType rt : vm.allClasses()) {
                String n = rt.name();
                if (n.contains("MtkTvFApi") || n.contains("twoworlds.factory")) hits.add(rt);
            }
            System.out.println("classes=" + hits.size());
            for (ReferenceType rt : hits) {
                System.out.println("CLASS " + rt.name());
                if (!(rt instanceof ClassType)) continue;
                ClassType ct = (ClassType) rt;
                try {
                    List<ObjectReference> inst = ct.instances(20);
                    System.out.println("  instances=" + inst.size());
                    for (ObjectReference o : inst) System.out.println("    " + o);
                } catch (Throwable t) {
                    System.out.println("  instances err " + t);
                }
                if (rt.name().endsWith("MtkTvFApiSystemBase") || rt.name().endsWith("MtkTvFApiSystem")) {
                    for (Field f : rt.allFields()) {
                        if (f.isStatic()) System.out.println("  STATIC FIELD " + f.typeName() + " " + f.name());
                    }
                    for (Method m : rt.methodsByName("sendCmdToFactorySvc")) {
                        System.out.println("  METHOD " + m.signature() + " static=" + m.isStatic());
                    }
                    for (Method m : rt.methodsByName("getInstance")) {
                        System.out.println("  GETINSTANCE " + m.signature() + " static=" + m.isStatic());
                    }
                }
            }
        } finally {
            vm.resume();
            vm.dispose();
        }
    }
}
