import com.sun.jdi.*;
import com.sun.jdi.connect.*;
import com.sun.jdi.event.*;
import com.sun.jdi.request.*;
import java.util.*;

public class JdiRemove {
    static Method findMethod(ReferenceType t, String name, String sig) {
        for (Method m : t.methodsByName(name)) if (m.signature().equals(sig)) return m;
        return null;
    }
    public static void main(String[] args) throws Exception {
        AttachingConnector conn=null;
        for (AttachingConnector c: Bootstrap.virtualMachineManager().attachingConnectors())
            if (c.name().equals("com.sun.jdi.SocketAttach")) conn=c;
        Map<String,Connector.Argument> ca=conn.defaultArguments();
        ca.get("hostname").setValue("localhost"); ca.get("port").setValue("8700"); ca.get("timeout").setValue("5000");
        VirtualMachine vm=conn.attach(ca);
        try {
            ReferenceType svc=vm.classesByName("com.cvte.factory.service.MainService").get(0);
            Method onStart=findMethod(svc,"onStartCommand","(Landroid/content/Intent;II)I");
            BreakpointRequest bp=vm.eventRequestManager().createBreakpointRequest(onStart.location());
            bp.setSuspendPolicy(EventRequest.SUSPEND_EVENT_THREAD); bp.enable();
            Thread trigger=new Thread(()->{ try { Thread.sleep(1200); new ProcessBuilder("adb","-s","<TV_IP>:5555","shell","am","startservice","-n","com.cvte.factory.service/.MainService","-a","cvte.factory.intent.action.APIInit").inheritIO().start().waitFor(); } catch(Exception e) { e.printStackTrace(); } });
            trigger.start(); vm.resume();
            EventSet es=vm.eventQueue().remove(15000);
            ThreadReference thr=null;
            if (es!=null) for (Event ev: es) if (ev instanceof BreakpointEvent) thr=((BreakpointEvent)ev).thread();
            if (thr==null) { System.out.println("no breakpoint"); vm.resume(); return; }
            ReferenceType base=vm.classesByName("com.mediatek.twoworlds.factory.MtkTvFApiSystemBase").get(0);
            Value imp=base.getValue(base.fieldByName("mtkFApiSystemImp"));
            if (!(imp instanceof ObjectReference)) { System.out.println("no wrapper"); vm.resume(); return; }
            ObjectReference wrapper=(ObjectReference)imp;
            Method m=findMethod(wrapper.referenceType(),"removeFile","(Ljava/lang/String;)I");
            if (m==null) { System.out.println("removeFile not found"); vm.resume(); return; }
            for (String path: args) {
                try {
                    Value r=wrapper.invokeMethod(thr,m,Collections.singletonList(vm.mirrorOf(path)),0);
                    System.out.println(path+" -> "+r);
                } catch (InvocationException ie) { System.out.println(path+" -> exception "+ie.exception().referenceType().name()); }
            }
            bp.disable(); vm.resume();
        } finally { vm.dispose(); }
    }
}
