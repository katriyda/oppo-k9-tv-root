import com.sun.jdi.*;
import com.sun.jdi.connect.*;
import com.sun.jdi.event.*;
import com.sun.jdi.request.*;
import java.util.*;

public class JdiHal {
    static Method find(ReferenceType t, String n, String s) { for (Method m: t.methodsByName(n)) if (m.signature().equals(s)) return m; return null; }
    static void ex(InvocationException ie) {
        ObjectReference e=ie.exception(); System.out.println("EX "+e.referenceType().name());
        try { Field f=e.referenceType().fieldByName("detailMessage"); if (f!=null) System.out.println("  msg="+e.getValue(f)); } catch(Throwable t){}
    }
    public static void main(String[] args) throws Exception {
        String action=args[0], path=args[1];
        AttachingConnector conn=null; for (AttachingConnector c: Bootstrap.virtualMachineManager().attachingConnectors()) if (c.name().equals("com.sun.jdi.SocketAttach")) conn=c;
        Map<String,Connector.Argument> ca=conn.defaultArguments(); ca.get("hostname").setValue("localhost"); ca.get("port").setValue("8700"); ca.get("timeout").setValue("5000");
        VirtualMachine vm=conn.attach(ca);
        try {
            ReferenceType svc=vm.classesByName("com.cvte.factory.service.MainService").get(0);
            Method onStart=find(svc,"onStartCommand","(Landroid/content/Intent;II)I");
            BreakpointRequest bp=vm.eventRequestManager().createBreakpointRequest(onStart.location()); bp.setSuspendPolicy(EventRequest.SUSPEND_EVENT_THREAD); bp.enable();
            Thread trigger=new Thread(()->{ try { Thread.sleep(1200); new ProcessBuilder("adb","-s","<TV_IP>:5555","shell","am","startservice","-n","com.cvte.factory.service/.MainService","-a","cvte.factory.intent.action.APIInit").inheritIO().start().waitFor(); } catch(Exception e) { e.printStackTrace(); } });
            trigger.start(); vm.resume();
            EventSet es=vm.eventQueue().remove(15000); ThreadReference thr=null; if (es!=null) for (Event ev: es) if (ev instanceof BreakpointEvent) thr=((BreakpointEvent)ev).thread();
            if (thr==null) { System.out.println("no breakpoint"); vm.resume(); return; }
            ReferenceType base=vm.classesByName("com.mediatek.twoworlds.factory.MtkTvFApiSystemBase").get(0);
            Value imp=base.getValue(base.fieldByName("mtkFApiSystemImp"));
            if (!(imp instanceof ObjectReference)) { System.out.println("no wrapper"); vm.resume(); return; }
            ObjectReference w=(ObjectReference)imp;
            if (action.equals("read")) {
                Method m=find(w.referenceType(),"readFile","(Ljava/lang/String;IJ)Ljava/lang/String;");
                if (m==null) { System.out.println("readFile missing"); vm.resume(); return; }
                Value r=w.invokeMethod(thr,m,Arrays.asList(vm.mirrorOf(path),vm.mirrorOf(0),vm.mirrorOf(16384L)),0);
                System.out.println("readFile="+r);
            } else {
                String content=args[2];
                Method m=find(w.referenceType(),"writeFile","(Ljava/lang/String;Ljava/lang/String;ZZ)I");
                if (m==null) { System.out.println("writeFile missing"); vm.resume(); return; }
                Value r=w.invokeMethod(thr,m,Arrays.asList(vm.mirrorOf(path),vm.mirrorOf(content),vm.mirrorOf(args.length>3?Boolean.parseBoolean(args[3]):false),vm.mirrorOf(args.length>4?Boolean.parseBoolean(args[4]):false)),0);
                System.out.println("writeFile="+r);
            }
            bp.disable(); vm.resume();
        } catch (InvocationException ie) { ex(ie); } finally { vm.dispose(); }
    }
}
