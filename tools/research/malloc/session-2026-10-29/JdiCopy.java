import com.sun.jdi.*;
import com.sun.jdi.connect.*;
import com.sun.jdi.event.*;
import com.sun.jdi.request.*;
import java.util.*;
public class JdiCopy {
 static Method find(ReferenceType t,String n,String s){for(Method m:t.methodsByName(n))if(m.signature().equals(s))return m;return null;}
 static void ex(InvocationException ie){ObjectReference e=ie.exception();System.out.println("EX "+e.referenceType().name());try{Field f=e.referenceType().fieldByName("detailMessage");if(f!=null)System.out.println(" msg="+e.getValue(f));}catch(Throwable t){}}
 public static void main(String[] a) throws Exception {
  AttachingConnector conn=null;for(AttachingConnector c:Bootstrap.virtualMachineManager().attachingConnectors())if(c.name().equals("com.sun.jdi.SocketAttach"))conn=c;
  Map<String,Connector.Argument> ca=conn.defaultArguments();ca.get("hostname").setValue("localhost");ca.get("port").setValue("8700");ca.get("timeout").setValue("5000");VirtualMachine vm=conn.attach(ca);
  try{
   ReferenceType svc=vm.classesByName("com.cvte.factory.service.MainService").get(0);Method on=find(svc,"onStartCommand","(Landroid/content/Intent;II)I");
   BreakpointRequest bp=vm.eventRequestManager().createBreakpointRequest(on.location());bp.setSuspendPolicy(EventRequest.SUSPEND_EVENT_THREAD);bp.enable();
   Thread t=new Thread(()->{try{Thread.sleep(1200);new ProcessBuilder("adb","-s","<TV_IP>:5555","shell","am","startservice","-n","com.cvte.factory.service/.MainService","-a","cvte.factory.intent.action.APIInit").inheritIO().start().waitFor();}catch(Exception e){e.printStackTrace();}});t.start();vm.resume();
   EventSet es=vm.eventQueue().remove(15000);ThreadReference thr=null;if(es!=null)for(Event ev:es)if(ev instanceof BreakpointEvent)thr=((BreakpointEvent)ev).thread();if(thr==null){System.out.println("no bp");vm.resume();return;}
   ReferenceType base=vm.classesByName("com.mediatek.twoworlds.factory.MtkTvFApiSystemBase").get(0);Value imp=base.getValue(base.fieldByName("mtkFApiSystemImp"));ObjectReference w=(ObjectReference)imp;
   Method m=find(w.referenceType(),"copyFile","(Ljava/lang/String;Ljava/lang/String;Z)I");System.out.println("copyFile method="+m);
   if(m!=null){Value r=w.invokeMethod(thr,m,Arrays.asList(vm.mirrorOf(a[0]),vm.mirrorOf(a[1]),vm.mirrorOf(a.length>2&&Boolean.parseBoolean(a[2]))),0);System.out.println("copyFile="+r);}
   bp.disable();vm.resume();
  }catch(InvocationException ie){ex(ie);}finally{vm.dispose();}
 }
}
