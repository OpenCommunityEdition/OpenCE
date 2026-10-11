"""Execute the real porting menu actions and publication with recording JNI stubs."""
from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parent.parent
out = root / 'build/porting-actions-tests'
out.mkdir(parents=True, exist_ok=True)
source = (root / 'port/android/app/src/main/java/com/halo/decomp/TouchControls.java').read_text()

def method(name):
    match = re.search(r'^    private void '+name+r'\(', source, re.M)
    start = source.index('{', match.start())
    depth, end = 1, start+1
    while depth:
        depth += (source[end] == '{')-(source[end] == '}')
        end += 1
    return source[match.start():end]

code = r'''
package com.halo.decomp;
import java.nio.file.*;
public class PortingActionsTest {
 static final int LEFT=TouchLayout.LEFT;
 TouchLayout layout=new TouchLayout();StartupCheats startupCheats;
 int menuContext=1,menuPage=6,revision,selectedControl,requestCount,lastCheat,lastStatus;
 boolean editing,hudEditing,visible;float sensitivity=1,appliedFov;
 String title;String[] labels;int[] actions,values;
 static class Owners {int size(){return 0;}} Owners owners=new Owners();
 static class Device {boolean hasVibrator(){return true;}} Device vibrator=new Device(),gyroscope=new Device();
 static class Toast {static final int LENGTH_SHORT=0,LENGTH_LONG=1;
  static Toast makeText(Object c,String s,int l){throw new AssertionError(s);}void show(){}}
 void openCreator(){} Object getContext(){return this;}void reset(){}void updateSensors(){}void cancelRumble(){}void invalidate(){}
 void exitHudEditing(){hudEditing=false;editing=false;menuPage=3;}
 boolean saveLayout(){return true;}String controlName(int i){return "Button "+i;}
 int nativeCheatStatus(int id){return lastStatus;}
 boolean nativeCheatRequest(int id,boolean enabled){requestCount++;lastCheat=id;lastStatus=enabled?1:0;return true;}
 void nativeFieldOfView(float degrees){appliedFov=degrees;}
 void nativeMenuPublish(int r,int p,boolean e,String t,String[] l,int[] a,int[] v){title=t;labels=l;actions=a;values=v;}
 static void check(boolean ok,String why){if(!ok)throw new AssertionError(why);}
'''
code += re.search(r'    private static final String\[\] CHEATS = .*?;', source, re.S).group(0)
code += method('applyMenuAction')+method('publishMenu')
code += r'''
 public static void main(String[] args)throws Exception {
  Path folder=Files.createTempDirectory("halo-actions-test"),init=folder.resolve("init.txt");
  try {
   PortingActionsTest t=new PortingActionsTest();t.startupCheats=new StartupCheats(init);
   t.publishMenu();check(t.title.equals("STARTUP CHEATS") && t.labels[0].equals("INVINCIBILITY [OFF]"),"Main menu shows startup defaults in capitals");
   t.lastStatus=-1;t.applyMenuAction(3000,0);t.applyMenuAction(3014,0);
   check(t.requestCount==0 && t.labels[0].endsWith("[ON]") && t.labels[14].endsWith("[ON]"),"Main menu writes selected cheats even without a player");
   String saved=Files.readString(init);
   t.menuContext=2;t.lastStatus=0;t.applyMenuAction(3000,0);t.applyMenuAction(3014,0);
   check(t.requestCount==2 && t.lastCheat==14 && saved.equals(Files.readString(init)),"Pause menu retains live cheats without changing init.txt");
   t.menuPage=1;t.publishMenu();check(t.title.equals("PORTING OPTIONS") && t.labels[1].equals("GENERAL") && t.labels[3].equals("BACK"),"General replaces Hardware");
   t.menuPage=3;t.publishMenu();check(t.title.equals("GENERAL") && t.actions[4]==13,"General contains FOV");
   check(t.labels[5].equals("EDIT IN-GAME UI") && t.actions[5]==14 && t.actions[6]==90,"General contains Edit in-game UI before Back");
   for(String label:t.labels)check(label.equals(label.toUpperCase(java.util.Locale.ROOT)),"Every General row is capitals: "+label);
   t.applyMenuAction(14,0);check(t.hudEditing && t.editing && t.menuPage==3,"Edit in-game UI opens its editor over General");
   t.applyMenuAction(30,0);check(t.hudEditing && t.layout.rumbleEnabled,"The editor takes no menu action but Back");
   t.applyMenuAction(90,0);check(!t.hudEditing && !t.editing && t.menuPage==3,"Back leaves the editor for General");
   t.applyMenuAction(13,0);check(t.values[0]>=0 && t.actions[0]==43,"FOV publishes a slider");
   t.applyMenuAction(43,1000);check(t.layout.fieldOfView==90 && t.appliedFov==90 && t.values[0]==1000,"FOV upper endpoint");
   t.applyMenuAction(43,-100);check(t.layout.fieldOfView==55 && t.appliedFov==55,"FOV clamps lower endpoint");
   t.applyMenuAction(90,0);check(t.menuPage==3,"FOV Back returns to General");
   t.menuPage=2;t.applyMenuAction(33,0);check(t.layout.overlayDisabled && t.labels[4].endsWith("ON"),"Disable all overlay toggles on");
   t.applyMenuAction(33,0);check(!t.layout.overlayDisabled,"Overlay can be restored through menus");
   System.out.println("Main/pause cheat routing, General, FOV endpoints and global overlay toggle passed");
  } finally {Files.deleteIfExists(init);Files.deleteIfExists(folder);}
 }
}
'''
(out / 'PortingActionsTest.java').write_text(code, encoding='utf-8')
java = root / 'port/android/app/src/main/java/com/halo/decomp'
subprocess.run(['javac', '-d', str(out), str(java / 'TouchLayout.java'), str(java / 'StartupCheats.java'), str(out / 'PortingActionsTest.java')], check=True)
subprocess.run(['java', '-cp', str(out), 'com.halo.decomp.PortingActionsTest'], check=True)
