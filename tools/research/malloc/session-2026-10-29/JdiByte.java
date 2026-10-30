import com.sun.jdi.*;
import com.sun.jdi.connect.*;
import com.sun.jdi.event.*;
import com.sun.jdi.request.*;
import java.util.*;

public class JdiByte {
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
        String path=a[0], text=a[1], mode=a.length>2?a[2]:"w";
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
            ReferenceType base=vm.classesByName("com.mediatek.twoworlds.factory.MtkTvFApiSystemBase").get(0);
            ObjectReference w=(ObjectReference)base.getValue(base.fieldByName("mtkFApiSystemImp"));
            Method m=find(w.referenceType(),"writeFileByte","(Ljava/lang/String;Ljava/util/ArrayList;Ljava/lang/String;)I");
            System.out.println("method="+m);
            ReferenceType alC=vm.classesByName("java.util.ArrayList").get(0);
            ObjectReference al=((ClassType)alC).newInstance(thr,find(alC,"<init>","()V"),Collections.emptyList(),0);
            Method add=find(alC,"add","(Ljava/lang/Object;)Z");
            ClassType byteC=(ClassType)vm.classesByName("java.lang.Byte").get(0);
            Method bctor=find(byteC,"<init>","(B)V");
            for(byte b: text.getBytes("ISO-8859-1")) {
                ObjectReference bo=byteC.newInstance(thr,bctor,Collections.singletonList(vm.mirrorOf(b)),0);
                al.invokeMethod(thr,add,Collections.singletonList((Value)bo),0);
            }
            try {
                Value r=w.invokeMethod(thr,m,Arrays.asList(vm.mirrorOf(path),(Value)al,vm.mirrorOf(mode)),0);
                System.out.println("writeFileByte="+r);
            } catch (InvocationException ie) {
                ObjectReference e=ie.exception(); System.out.println("EX "+e.referenceType().name());
                try{Field f=e.referenceType().fieldByName("detailMessage");if(f!=null)System.out.println("msg="+e.getValue(f));}catch(Throwable x){}
            }
            bp.disable(); vm.resume();
        } finally { try{vm.dispose();}catch(Throwable e2){} }
    }
}
