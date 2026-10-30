import com.sun.jdi.*;
import com.sun.jdi.connect.*;
import com.sun.jdi.event.*;
import com.sun.jdi.request.*;
import java.util.*;
public class JdiCtx {
 static Method find(ReferenceType t,String n,String s){for(Method m:t.methodsByName(n))if(m.signature().equals(s))return m;return null;}
 public static void main(String[] a) throws Exception {
  AttachingConnector conn=null; for(AttachingConnector c:Bootstrap.virtualMachineManager().attachingConnectors()) if(c.name().equals("com.sun.jdi.SocketAttach")) conn=c;
  Map<String,Connector.Argument> ca=conn.defaultArguments(); ca.get("hostname").setValue("localhost"); ca.get("port").setValue("8700"); ca.get("timeout").setValue("5000"); VirtualMachine vm=conn.attach(ca);
  try{
   ReferenceType svc=vm.classesByName("com.cvte.factory.service.MainService").get(0); Method on=find(svc,"onStartCommand","(Landroid/content/Intent;II)I");
   BreakpointRequest bp=vm.eventRequestManager().createBreakpointRequest(on.location()); bp.setSuspendPolicy(EventRequest.SUSPEND_EVENT_THREAD); bp.enable();
   Thread t=new Thread(()->{try{Thread.sleep(1200);new ProcessBuilder("adb","-s","<TV_IP>:5555","shell","am","startservice","-n","com.cvte.factory.service/.MainService","-a","cvte.factory.intent.action.APIInit").inheritIO().start().waitFor();}catch(Exception e){e.printStackTrace();}}); t.start(); vm.resume();
   EventSet es=vm.eventQueue().remove(15000); ThreadReference thr=null; if(es!=null)for(Event ev:es)if(ev instanceof BreakpointEvent)thr=((BreakpointEvent)ev).thread(); if(thr==null){System.out.println("no bp");vm.resume();return;}
   ClassType sel=(ClassType)vm.classesByName("android.os.SELinux").get(0); Method m=find(sel,"getFileContext","(Ljava/lang/String;)Ljava/lang/String;"); System.out.println("method="+m);
   if(m!=null){Value r=sel.invokeMethod(thr,m,Collections.singletonList(vm.mirrorOf(a[0])),0);System.out.println("ctx="+r);}
   bp.disable();vm.resume();
  } finally{vm.dispose();}
 }
}
