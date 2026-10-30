import com.sun.jdi.*;
import com.sun.jdi.connect.*;
import com.sun.jdi.event.*;
import com.sun.jdi.request.*;
import java.util.*;
import java.nio.file.*;
public class JdiMemfd {
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
   ClassType os=(ClassType)vm.classesByName("android.system.Os").get(0);
   for(Method m:os.methodsByName("memfd_create")) System.out.println("mf="+m.signature());
   Method mc=find(os,"memfd_create","(Ljava/lang/String;I)Ljava/io/FileDescriptor;");System.out.println("mc="+mc);
   ObjectReference fdobj=(ObjectReference)os.invokeMethod(thr,mc,Arrays.asList(vm.mirrorOf("libx"),vm.mirrorOf(0)),0);
   ReferenceType fdC=vm.classesByName("java.io.FileDescriptor").get(0);Field df=fdC.fieldByName("descriptor");int fd=((IntegerValue)fdobj.getValue(df)).value();System.out.println("fd="+fd);
   Method ft=find(os,"ftruncate","(Ljava/io/FileDescriptor;J)V");System.out.println("ft="+ft); if(ft!=null) os.invokeMethod(thr,ft,Arrays.asList((Value)fdobj,vm.mirrorOf((long)data.length)),0);
   Method wr=find(os,"write","(Ljava/io/FileDescriptor;[BII)I");System.out.println("wr="+wr);ArrayType at=(ArrayType)wr.argumentTypes().get(1);ArrayReference arr=at.newInstance(data.length);List<Value> vals=new ArrayList<>();for(byte b:data)vals.add(vm.mirrorOf(b));arr.setValues(vals);
   Value n=os.invokeMethod(thr,wr,Arrays.asList((Value)fdobj,(Value)arr,vm.mirrorOf(0),vm.mirrorOf(data.length)),0);System.out.println("write="+n);
   String path="/proc/self/fd/"+fd;
   ClassType sys=(ClassType)vm.classesByName("java.lang.System").get(0);
   try{ sys.invokeMethod(thr,find(sys,"load","(Ljava/lang/String;)V"),Collections.singletonList(vm.mirrorOf(path)),0);System.out.println("loaded"); }catch(InvocationException ie){ex(ie);}
   bp.disable();vm.resume();
  }finally{vm.dispose();}
 }
}
