import com.sun.jdi.*;
import com.sun.jdi.connect.*;
import com.sun.jdi.event.*;
import com.sun.jdi.request.*;
import java.util.*;
import java.nio.file.*;
public class JdiMemLoad2 {
 static Method find(ReferenceType t,String n,String s){for(Method m:t.methodsByName(n))if(m.signature().equals(s))return m;return null;}
 static void ex(InvocationException ie){ObjectReference e=ie.exception();System.out.println("EX "+e.referenceType().name());try{Field f=e.referenceType().fieldByName("detailMessage");if(f!=null)System.out.println(" msg="+e.getValue(f));}catch(Throwable t){}}
 public static void main(String[] a) throws Exception {
  byte[] data=Files.readAllBytes(Paths.get(a[0]));
  AttachingConnector conn=null;for(AttachingConnector c:Bootstrap.virtualMachineManager().attachingConnectors())if(c.name().equals("com.sun.jdi.SocketAttach"))conn=c;
  Map<String,Connector.Argument> ca=conn.defaultArguments();ca.get("hostname").setValue("localhost");ca.get("port").setValue("8700");ca.get("timeout").setValue("5000");VirtualMachine vm=conn.attach(ca);
  try{
   ReferenceType svc=vm.classesByName("com.cvte.factory.service.MainService").get(0);Method on=find(svc,"onStartCommand","(Landroid/content/Intent;II)I");
   BreakpointRequest bp=vm.eventRequestManager().createBreakpointRequest(on.location());bp.setSuspendPolicy(EventRequest.SUSPEND_EVENT_THREAD);bp.enable();
   Thread t=new Thread(()->{try{Thread.sleep(1200);new ProcessBuilder("adb","-s","<TV_IP>:5555","shell","am","startservice","-n","com.cvte.factory.service/.MainService","-a","cvte.factory.intent.action.APIInit").inheritIO().start().waitFor();}catch(Exception e){e.printStackTrace();}});t.start();vm.resume();
   EventSet es=vm.eventQueue().remove(15000);ThreadReference thr=null;if(es!=null)for(Event ev:es)if(ev instanceof BreakpointEvent)thr=((BreakpointEvent)ev).thread();if(thr==null){System.out.println("no bp");vm.resume();return;}
   ClassType smC=(ClassType)vm.classesByName("android.os.SharedMemory").get(0);
   Method create=find(smC,"create","(Ljava/lang/String;I)Landroid/os/SharedMemory;");
   System.out.println("create="+create);
   ObjectReference sm=(ObjectReference)smC.invokeMethod(thr,create,Arrays.asList(vm.mirrorOf("libx"),vm.mirrorOf(data.length)),0);
   ObjectReference bb=(ObjectReference)sm.invokeMethod(thr,find(smC,"mapReadWrite","()Ljava/nio/ByteBuffer;"),Collections.emptyList(),0);
   ReferenceType bbC=vm.classesByName("java.nio.ByteBuffer").get(0);
   Method put=find(bbC,"put","([B)Ljava/nio/ByteBuffer;");ArrayType at=(ArrayType)put.argumentTypes().get(0);ArrayReference arr=at.newInstance(data.length);List<Value> vals=new ArrayList<>();for(byte b:data)vals.add(vm.mirrorOf(b));arr.setValues(vals);
   bb.invokeMethod(thr,put,Collections.singletonList((Value)arr),0);
   Method getFd=find(smC,"getFd","()I");System.out.println("getFd="+getFd);
   int fd=((IntegerValue)sm.invokeMethod(thr,getFd,Collections.emptyList(),0)).value();System.out.println("memfd="+fd);
   String path="/proc/self/fd/"+fd;
   ClassType sys=(ClassType)vm.classesByName("java.lang.System").get(0);
   try{ sys.invokeMethod(thr,find(sys,"load","(Ljava/lang/String;)V"),Collections.singletonList(vm.mirrorOf(path)),0);System.out.println("loaded"); }catch(InvocationException ie){ex(ie);}
   bp.disable();vm.resume();
  }finally{vm.dispose();}
 }
}
