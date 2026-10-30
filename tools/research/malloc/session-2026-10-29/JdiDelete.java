import com.sun.jdi.*;
import com.sun.jdi.connect.*;
import com.sun.jdi.event.*;
import com.sun.jdi.request.*;
import java.util.*;

public class JdiDelete {
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
        AttachingConnector conn=null;
        for (AttachingConnector c: Bootstrap.virtualMachineManager().attachingConnectors())
            if (c.name().equals("com.sun.jdi.SocketAttach")) conn=c;
        Map<String,Connector.Argument> ca=conn.defaultArguments();
        ca.get("hostname").setValue("localhost"); ca.get("port").setValue("8700"); ca.get("timeout").setValue("5000");
        VirtualMachine vm=conn.attach(ca);
        try {
            ReferenceType svc=vm.classesByName("com.cvte.factory.service.MainService").get(0);
            Method on=find(svc,"onStartCommand","(Landroid/content/Intent;II)I");
            BreakpointRequest bp=vm.eventRequestManager().createBreakpointRequest(on.location());
            bp.setSuspendPolicy(EventRequest.SUSPEND_EVENT_THREAD); bp.enable();
            Thread t=new Thread(()->{try{Thread.sleep(1200);
                new ProcessBuilder("adb","-s","<TV_IP>:5555","shell","am","startservice",
                    "-n","com.cvte.factory.service/.MainService","-a","cvte.factory.intent.action.APIInit")
                    .inheritIO().start().waitFor();}catch(Exception e){e.printStackTrace();}});
            t.start(); vm.resume();
            ThreadReference thr=waitBreak(vm,15000);
            if(thr==null){System.out.println("no breakpoint"); vm.resume(); return;}
            ReferenceType fc=vm.classesByName("java.io.File").get(0);
            Method ctor=find(fc,"<init>","(Ljava/lang/String;)V"), del=find(fc,"delete","()Z");
            for(String p:a){
                ObjectReference f=((ClassType)fc).newInstance(thr,ctor,Collections.singletonList(vm.mirrorOf(p)),0);
                Value r=f.invokeMethod(thr,del,Collections.emptyList(),0);
                System.out.println(p+" delete="+r);
            }
            bp.disable(); vm.resume();
        } finally { try{vm.dispose();}catch(Throwable e2){} }
    }
}
