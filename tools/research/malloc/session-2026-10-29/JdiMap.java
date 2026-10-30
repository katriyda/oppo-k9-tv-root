import com.sun.jdi.*;
import com.sun.jdi.connect.*;
import com.sun.jdi.event.*;
import com.sun.jdi.request.*;
import java.util.*;

public class JdiMap {
    static Method find(ReferenceType t, String n, String s) {
        for (Method m : t.methodsByName(n)) if (m.signature().equals(s)) return m;
        return null;
    }
    static ThreadReference waitBreak(VirtualMachine vm) throws Exception {
        EventSet es=vm.eventQueue().remove(15000);
        if (es==null) return null;
        for (Event ev: es) if (ev instanceof BreakpointEvent) return ((BreakpointEvent)ev).thread();
        return null;
    }
    public static void main(String[] args) throws Exception {
        String path=args.length>0?args[0]:"/dev/miomap";
        long off=args.length>1?Long.decode(args[1]):0;
        long len=args.length>2?Long.decode(args[2]):4096;
        AttachingConnector conn=null;
        for (AttachingConnector c: Bootstrap.virtualMachineManager().attachingConnectors())
            if (c.name().equals("com.sun.jdi.SocketAttach")) conn=c;
        Map<String,Connector.Argument> ca=conn.defaultArguments();
        ca.get("hostname").setValue("localhost"); ca.get("port").setValue("8700"); ca.get("timeout").setValue("5000");
        VirtualMachine vm=conn.attach(ca);
        try {
            ReferenceType svc=vm.classesByName("com.cvte.factory.service.MainService").get(0);
            Method onStart=find(svc,"onStartCommand","(Landroid/content/Intent;II)I");
            BreakpointRequest bp=vm.eventRequestManager().createBreakpointRequest(onStart.location());
            bp.setSuspendPolicy(EventRequest.SUSPEND_EVENT_THREAD); bp.enable();
            Thread trigger=new Thread(()->{ try { Thread.sleep(1200); new ProcessBuilder("adb","-s","<TV_IP>:5555","shell","am","startservice","-n","com.cvte.factory.service/.MainService","-a","cvte.factory.intent.action.APIInit").inheritIO().start().waitFor(); } catch(Exception e) { e.printStackTrace(); } });
            trigger.start(); vm.resume();
            ThreadReference thr=waitBreak(vm);
            if (thr==null) { System.out.println("no breakpoint"); vm.resume(); return; }
            ReferenceType fileC=vm.classesByName("java.io.File").get(0);
            ObjectReference file=((ClassType)fileC).newInstance(thr,find(fileC,"<init>","(Ljava/lang/String;)V"),Collections.singletonList(vm.mirrorOf(path)),0);
            ReferenceType fisC=vm.classesByName("java.io.FileInputStream").get(0);
            ObjectReference fis=((ClassType)fisC).newInstance(thr,find(fisC,"<init>","(Ljava/io/File;)V"),Collections.singletonList((Value)file),0);
            ObjectReference ch=(ObjectReference)fis.invokeMethod(thr,find(fisC,"getChannel","()Ljava/nio/channels/FileChannel;"),Collections.emptyList(),0);
            ReferenceType mm=vm.classesByName("java.nio.channels.FileChannel$MapMode").get(0);
            Value ronly=mm.getValue(mm.fieldByName("READ_ONLY"));
            ReferenceType fc=vm.classesByName("java.nio.channels.FileChannel").get(0);
            Method map=find(fc,"map","(Ljava/nio/channels/FileChannel$MapMode;JJ)Ljava/nio/MappedByteBuffer;");
            System.out.println("path="+path+" off="+off+" len="+len);
            try {
                Value res=ch.invokeMethod(thr,map,Arrays.asList(ronly,vm.mirrorOf(off),vm.mirrorOf(len)),0);
                System.out.println("map="+res);
                if (res instanceof ObjectReference) {
                    ObjectReference mb=(ObjectReference)res;
                    ReferenceType bb=vm.classesByName("java.nio.Buffer").get(0);
                    System.out.println("capacity="+mb.invokeMethod(thr,find(bb,"capacity","()I"),Collections.emptyList(),0));
                    System.out.println("limit="+mb.invokeMethod(thr,find(bb,"limit","()I"),Collections.emptyList(),0));
                    // don't read yet by default
                    if (args.length>3) {
                        Method get=find(vm.classesByName("java.nio.ByteBuffer").get(0),"get","(I)B");
                        for (int i=0;i<Integer.parseInt(args[3]);i++) {
                            Value v=mb.invokeMethod(thr,get,Collections.singletonList(vm.mirrorOf(i)),0);
                            System.out.printf("%02x ", ((ByteValue)v).value() & 0xff);
                            if ((i&15)==15) System.out.println();
                        }
                    }
                }
            } catch (InvocationException ie) { System.out.println("invoke exception="+ie.exception().referenceType().name()); }
            bp.disable(); vm.resume();
        } finally { vm.dispose(); }
    }
}
