import com.sun.jdi.*;
import com.sun.jdi.connect.*;
import java.util.*;

public class JdiMethods {
    public static void main(String[] a) throws Exception {
        String cls = a[0];
        String filter = a.length > 1 ? a[1] : "";
        AttachingConnector conn = null;
        for (AttachingConnector c : Bootstrap.virtualMachineManager().attachingConnectors())
            if (c.name().equals("com.sun.jdi.SocketAttach")) conn = c;
        Map<String,Connector.Argument> ca = conn.defaultArguments();
        ca.get("hostname").setValue("localhost"); ca.get("port").setValue("8700"); ca.get("timeout").setValue("5000");
        VirtualMachine vm = conn.attach(ca);
        try {
            for (ReferenceType t : vm.classesByName(cls)) {
                System.out.println("TYPE " + t.name());
                for (Field f : t.fields()) System.out.println("FIELD " + f.name() + " " + f.typeName());
                for (Method m : t.methods()) {
                    if (filter.isEmpty() || m.name().toLowerCase().contains(filter.toLowerCase()))
                        System.out.println(m.name() + m.signature());
                }
            }
        } finally { vm.dispose(); }
    }
}
