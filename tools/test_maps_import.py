"""Exercise the launcher's real copy/promotion logic with a fake document provider."""
from pathlib import Path
import re, subprocess
root = Path(__file__).resolve().parent.parent
out = root / 'build/maps-import-tests'
out.mkdir(parents=True, exist_ok=True)
source = (root / 'port/android/app/src/main/java/com/halo/decomp/LauncherActivity.java').read_text()
def extract(pattern):
    match = re.search(pattern, source)
    start = source.index('{', match.start()); end = start + 1; depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}'); end += 1
    return source[match.start():end]
uri = out / 'android/net/Uri.java'
uri.parent.mkdir(parents=True, exist_ok=True)
uri.write_text('package android.net; public class Uri {public final String id; public Uri(String id){this.id=id;}}')
code = r'''
import android.net.Uri;
import java.io.*;import java.nio.file.*;import java.util.*;
public class MapsImportTest {
 File dataRoot;String failure;boolean started;
 Map<String,ArrayList<MapDocument>> folders=new HashMap<>();Map<String,byte[]> files=new HashMap<>();
 static class DocumentsContract {
  static String getTreeDocumentId(Uri u){return u.id;}
  static Uri buildDocumentUriUsingTree(Uri u,String id){return new Uri(id);}
 }
 static class Handler {void post(Runnable action){action.run();}} Handler handler=new Handler();
 class Resolver {InputStream openInputStream(Uri uri)throws IOException {
  if(!files.containsKey(uri.id))throw new IOException("Provider read failed");
  return new ByteArrayInputStream(files.get(uri.id));
 }}
 Resolver getContentResolver(){return new Resolver();}
 ArrayList<MapDocument> children(Uri uri)throws IOException {
  if(!folders.containsKey(uri.id))throw new IOException("Folder inaccessible");return folders.get(uri.id);
 }
 void report(String text,int progress){}void fail(String text){failure=text;}void startGame(){started=true;}
 void folder(String id,MapDocument... entries){folders.put(id,new ArrayList<>(Arrays.asList(entries)));}
 MapDocument file(String id,String name){files.put(id,new byte[]{1,2,3});return new MapDocument(new Uri(id),name,false);}
 MapDocument dir(String id,String name){return new MapDocument(new Uri(id),name,true);}
 static void check(boolean ok,String why){if(!ok)throw new AssertionError(why);}
'''
code += extract(r'    private static final class MapDocument')
for name in ['validDocumentName','hasUiMap','copyMaps','removeImport','importMaps','haveData']:
    code += extract(r'    private \w+ '+name+r'\(')
code += r'''
 public static void main(String[] args)throws Exception {
  MapsImportTest t=new MapsImportTest();t.dataRoot=Files.createTempDirectory("halo-maps-import").toFile();
  try {
   File old=new File(t.dataRoot,"maps");old.mkdir();Files.writeString(new File(old,"keep.txt").toPath(),"old");
   t.folder("empty");t.importMaps(new Uri("empty"));
   check(!t.started&&t.failure!=null&&new File(old,"keep.txt").exists(),"Missing ui.map preserves existing data");
   t.failure=null;t.folder("bad",t.file("ui","ui.map"),new MapDocument(new Uri("missing"),"a10.map",false));
   t.importMaps(new Uri("bad"));
   check(!t.started&&t.failure!=null&&new File(old,"keep.txt").exists(),"Provider failure preserves existing maps");
   check(t.dataRoot.list().length==1,"Failed import removes partial staging files");
   t.failure=null;t.folder("maps",t.file("ui","UI.MAP"),t.file("level","A10.MAP"),t.dir("sounds","Sounds"));
   t.folder("sounds",t.file("sound","Test.ogg"));t.folder("parent",t.dir("maps","MAPS"));
   t.importMaps(new Uri("parent"));
   check(t.started&&t.failure==null&&t.haveData(),"Parent/maps import starts the game only when complete");
   check(new File(old,"a10.map").isFile()&&new File(old,"sounds/test.ogg").isFile(),"Uppercase map names and nested files are copied");
   check(t.dataRoot.list().length==1,"Successful import leaves no staging directories");
   t.started=false;t.folder("direct",t.file("ui2","ui.map"));t.importMaps(new Uri("direct"));
   check(t.started&&t.haveData(),"Direct maps folder is accepted");
   t.started=false;t.failure=null;t.folder("unsafe",t.file("ui3","ui.map"),t.file("escape","../escaped.map"));
   t.importMaps(new Uri("unsafe"));
   check(!t.started&&t.failure!=null&&t.haveData()&&!new File(t.dataRoot,"escaped.map").exists(),"Invalid provider filenames cannot escape staging");
   System.out.println("Extracted maps: direct/parent folder, nested files, case normalization, invalid input and interrupted copy passed");
  } finally {t.removeImport(t.dataRoot);}
 }
}
'''
(out/'MapsImportTest.java').write_text(code)
subprocess.run(['javac','-d',str(out),str(uri),str(out/'MapsImportTest.java')],check=True)
subprocess.run(['java','-cp',str(out),'MapsImportTest'],check=True)
