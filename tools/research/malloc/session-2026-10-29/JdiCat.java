import com.sun.jdi.*;
import com.sun.jdi.connect.*;
import com.sun.jdi.event.*;
import com.sun.jdi.request.*;
import java.util.*;

public class JdiCat {
    static Method find(ReferenceType t, String n, String s) { for (Method m: t.methodsByName(n)) if (m.signature().equals(s)) return m; return null; }
    public static void main(String[] args) throws Exception {
        String path=args[0]; int max=args.length>1?Integer.parseInt(args[1]):8192;
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
            ReferenceType fc=vm.classesByName("java.io.File").get(0);
            ObjectReference file=((ClassType)fc).newInstance(thr,find(fc,"<init>","(Ljava/lang/String;)V"),Collections.singletonList(vm.mirrorOf(path)),0);
            ReferenceType fisC=vm.classesByName("java.io.FileInputStream").get(0);
            ObjectReference fis=((ClassType)fisC).newInstance(thr,find(fisC,"<init>","(Ljava/io/File;)V"),Collections.singletonList((Value)file),0);
            Method read=find(fisC,"read","([B)I"); ArrayType at=(ArrayType)read.argumentTypes().get(0); ArrayReference buf=at.newInstance(max);
            Value n=fis.invokeMethod(thr,read,Collections.singletonList((Value)buf),0);
            int len=((IntegerValue)n).value(); System.out.println("read="+len);
            if (len>0) {
                byte[] bytes=new byte[len]; for (int i=0;i<len;i++) bytes[i]=((ByteValue)buf.getValue(i)).value();
                System.out.print(new String(bytes,"UTF-8"));
            }
            fis.invokeMethod(thr,find(fisC,"close","()V"),Collections.emptyList(),0); bp.disable(); vm.resume();
        } finally { vm.dispose(); }
    }
}
