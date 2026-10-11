"""Execute TouchControls' actual gesture handlers with recording input stubs."""
from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parent.parent
out = root/'build/menu-touch-tests'
out.mkdir(parents=True, exist_ok=True)
source = (root/'port/android/app/src/main/java/com/halo/decomp/TouchControls.java').read_text()

def method(name):
    match = re.search(r'^    (?:@Override )?(?:public|private) (?:static )?\w+ '+name+r'\(', source, re.M)
    if not match:
        raise ValueError(name)
    start = source.index('{', match.start())
    depth, end = 1, start+1
    while depth:
        depth += (source[end] == '{')-(source[end] == '}')
        end += 1
    return source[match.start():end].replace('@Override ', '')

code = r'''
import java.util.*;
public class MenuTouchTest {
 static final int LEFT=16,LOOK=-5,TOGGLE=-3,EDIT=-4,EXPORT=-6,IMPORT=-7;
 static class TouchLayout {static final int CAMERA=18;}
 int cameraTaps;
 void nativeCameraMode(){cameraTaps++;}
 boolean editing,menusActive=true,visible=true,menuOverlayVisible,menuScrolled;
 int menuPointer=-1,menuGestureRevision,menuGesturePage,menuPage,revision;
 int lookPointer=-1,clicks,pointerClicks,stateBits,dragControl=-1;
 float scale=1,offsetX,offsetY,logicalWidth=960,menuStartX,menuStartY,menuLastY;
 float lookX,lookY,sensitivity=1,scroll;
 int[] axes=new int[6];
 static class Owners {
  TreeMap<Integer,Integer> map=new TreeMap<>();
  int get(int id,int fallback){return map.getOrDefault(id,fallback);}
  void put(int id,int value){map.put(id,value);}
  void delete(int id){map.remove(id);}
  int size(){return map.size();}
  int valueAt(int i){return new ArrayList<>(map.values()).get(i);}
  int indexOfValue(int value){return new ArrayList<>(map.values()).indexOf(value);}
 }
 Owners owners=new Owners();
 Map<Integer,float[]> buttonTouches=new HashMap<>();
 static class Layout {
  boolean overlayDisabled;
  int size(){return 3;} boolean shown(int i){return true;}
  int type(int i){return i==1?LEFT:i==2?18:0;}
  float x(int i){return i==1?100:i==2?650:800;} float y(int i){return 400;}
  float radius(int i){return 32;}
 }
 Layout layout=new Layout();
 static class Button {int bit,trigger=-1;}
 Button[] buttons=new Button[19];
 static class MotionEvent {
  static final int ACTION_DOWN=0,ACTION_UP=1,ACTION_MOVE=2,ACTION_CANCEL=3,ACTION_POINTER_DOWN=5,ACTION_POINTER_UP=6;
  int action,id;float x,y;
  MotionEvent(int a,float x,float y){action=a;this.x=x;this.y=y;}
  int getActionMasked(){return action;} int getActionIndex(){return 0;}
  int getPointerId(int i){return id;} int findPointerIndex(int id){return this.id==id?0:-1;}
  int getPointerCount(){return 1;} float getX(int i){return x;} float getY(int i){return y;}
 }
 int getWidth(){return 960;} int getHeight(){return 540;}
 float optionsX(){return 920;} float toolbarY(){return 38;}
 void invalidate(){} boolean performClick(){clicks++;return true;}
 boolean editTouch(MotionEvent e,int a,int i,int id,float x,float y){return true;}
 void nativeLook(float dx,float dy){throw new AssertionError("Menu enabled gameplay aiming");}
 void nativeState(int lx,int ly,int rx,int ry,int lt,int rt,int bits){stateBits=bits;}
 void nativeMenuPointer(float x,float y,boolean click,boolean back,float delta,boolean bar){
  if(click)pointerClicks++;scroll+=delta;
 }
 void reset(){owners.map.clear();buttonTouches.clear();menuPointer=lookPointer=-1;Arrays.fill(axes,0);publish();}
 void event(int action,float x,float y){onTouchEvent(new MotionEvent(action,x,y));}
 void tap(float x,float y){event(0,x,y);event(1,x,y);}
 static void check(boolean ok,String message){if(!ok)throw new AssertionError(message);}
'''
code += '\n'.join(method(name) for name in ('inside','held','overlayVisible','hit','moveStick','publish','onTouchEvent','menuTouch'))
code += r'''
 public static void main(String[] args){
  MenuTouchTest t=new MenuTouchTest();for(int i=0;i<t.buttons.length;i++)t.buttons[i]=new Button();
  t.tap(480,38);check(t.menuOverlayVisible && t.pointerClicks==0,"Touch toggle must not click UI");
  t.event(0,800,400);check(t.stateBits==1,"Visible overlay A must navigate original menus");
  t.event(1,800,400);check(t.stateBits==0,"Overlay A must release");
  t.tap(480,38);check(!t.menuOverlayVisible && t.pointerClicks==0,"Hide toggle must not click UI");
  t.tap(320,220);check(t.pointerClicks==1,"Hidden overlay must allow direct taps");
  t.menuPage=6;t.pointerClicks=0;
  t.event(0,320,220);t.event(2,320,160);t.event(1,320,160);
  check(t.scroll>0 && t.pointerClicks==0,"Scrolling must not activate a row on release");
  t.menuPage=4;t.pointerClicks=0;
  t.event(0,320,220);t.event(2,340,220);t.revision++;t.event(2,380,220);t.event(1,380,220);
  check(t.pointerClicks==4,"Value slider must continue dragging across acknowledgement revisions");
  t.menuPage=13;t.pointerClicks=0;
  t.event(0,320,220);t.event(2,340,220);t.revision++;t.event(2,380,220);t.event(1,380,220);
  check(t.pointerClicks==4,"FOV slider must support continuous drag");
  t.menuPage=6;t.pointerClicks=0;
  t.event(0,320,220);t.revision++;t.event(1,320,220);
  check(t.pointerClicks==0,"Stale tap must not select a different page");
  t.menuPage=0;t.tap(480,38);t.event(0,800,400);t.event(3,800,400);
  check(t.stateBits==0 && t.owners.size()==0,"Cancellation must release controller buttons");
  t.editing=true;t.menuOverlayVisible=false;check(t.overlayVisible(),"Layout editor must always show controls");
  t.editing=false;t.layout.overlayDisabled=true;t.menuPage=0;t.pointerClicks=0;
  t.tap(480,38);check(t.pointerClicks==1 && !t.overlayVisible(),"Disabled Hide/Touch area must route to the native menu");
  t.stateBits=9;t.publish();check(t.stateBits==0,"Disabled overlay must release gameplay input");
  t.menusActive=false;t.tap(800,400);check(t.stateBits==0 && t.lookPointer==-1,"Disabled controls cannot press buttons or aim");
  t.menusActive=false;t.visible=true;t.layout.overlayDisabled=false;t.reset();
  t.event(0,650,400);check(t.cameraTaps==1,"Camera mode must activate on one touch");
  t.event(2,651,400);check(t.cameraTaps==1,"Holding camera must not repeat");
  t.event(1,651,400);t.tap(650,400);check(t.cameraTaps==2,"Next tap must switch again");
  t.menusActive=true;t.menuOverlayVisible=true;t.tap(650,400);check(t.cameraTaps==2,"Menus must suppress camera commands");
  System.out.println("Menu touch toggle, direct taps, overlay buttons, scrolling, slider drag and cancellation passed");
 }
}
'''
(out/'MenuTouchTest.java').write_text(code, encoding='utf-8')
subprocess.run(['javac','-d',str(out),str(out/'MenuTouchTest.java')],check=True)
subprocess.run(['java','-cp',str(out),'MenuTouchTest'],check=True)
