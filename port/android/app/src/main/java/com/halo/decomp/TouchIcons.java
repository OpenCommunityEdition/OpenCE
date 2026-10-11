package com.halo.decomp;

import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;

/** Original vector pictograms with the angular, cyan Halo HUD treatment. */
final class TouchIcons {
    private static void lines(Canvas c, Paint p, float... points) {
        Path path = new Path();
        path.moveTo(points[0], points[1]);
        for (int i=2; i<points.length; i+=2) path.lineTo(points[i], points[i+1]);
        c.drawPath(path, p);
    }
    private static void arrow(Canvas c, Paint p) {
        lines(c,p,0,12,0,-12,-6,-6,0,-12,6,-6);
    }
    private static void spartan(Canvas c, Paint p, boolean crouched) {
        c.drawCircle(0,-11,3,p);
        if (crouched) {
            lines(c,p,0,-7,-4,1,6,4,2,12,10,12);
            lines(c,p,-4,1,-10,7,-5,12);
            lines(c,p,-1,-4,7,0,12,-3);
        } else {
            lines(c,p,0,-7,0,2,-7,12,-12,12);
            lines(c,p,0,2,7,9,12,6);
            lines(c,p,-11,-7,-5,-2,0,-6,6,-3,11,-9);
        }
    }
    static void draw(Canvas c, Paint p, int type, float x, float y, float radius, boolean active) {
        c.save(); c.translate(x,y); c.scale(radius/22f,radius/22f);
        p.setStyle(Paint.Style.STROKE); p.setStrokeWidth(1.8f);
        p.setStrokeJoin(Paint.Join.MITER); p.setStrokeCap(Paint.Cap.SQUARE);
        p.setColor(active ? 0xffe4faff : 0xffbceaff);
        switch(type) {
        case 0: // jump
            spartan(c,p,false); lines(c,p,-14,0,-14,-13,-18,-9,-14,-13,-10,-9); break;
        case 1: // melee: armoured fist
            lines(c,p,-11,2,-12,-5,-8,-9,-4,-8,0,-10,4,-8,8,-8,12,-4,10,5,5,10,-4,10,-11,2);
            lines(c,p,-8,-7,-7,-1,-3,-2,-3,-7,1,-8,2,-2,6,-3,7,-7); break;
        case 2: // reload / interact
            c.drawArc(-12,-12,12,12,35,275,false,p);
            lines(c,p,12,-7,12,0,5,-1);
            lines(c,p,-3,-6,3,-6,3,7,-3,7,-3,-6); break;
        case 3: // swap weapons
            lines(c,p,-14,-6,12,-6,7,-11,12,-6,7,-1);
            lines(c,p,14,6,-12,6,-7,1,-12,6,-7,11); break;
        case 4: case 17: // assault rifle silhouette
            lines(c,p,-15,-5,-5,-5,-5,-9,6,-9,6,-5,15,-5,15,-1,4,-1,1,9,-4,9,-2,1,-8,1,-14,6,-15,-5);
            lines(c,p,1,1,6,1,8,5,3,5); lines(c,p,-3,-12,4,-12); break;
        case 5: // fragmentation grenade
            lines(c,p,-4,-8,-4,-13,3,-13,3,-8,9,-2,9,8,4,13,-4,13,-9,8,-9,-2,-4,-8);
            lines(c,p,3,-12,10,-9,11,-3); lines(c,p,-7,0,7,0); lines(c,p,-7,6,7,6); break;
        case 6: spartan(c,p,true); break;
        case 7: // scope reticle
            c.drawCircle(0,0,11,p); lines(c,p,-16,0,-5,0); lines(c,p,5,0,16,0);
            lines(c,p,0,-16,0,-5); lines(c,p,0,5,0,16); c.drawCircle(0,0,2,p); break;
        case 8: // flashlight
            lines(c,p,-13,-4,-3,-4,3,-9,3,9,-3,4,-13,4,-13,-4);
            lines(c,p,7,-6,14,-10); lines(c,p,8,0,16,0); lines(c,p,7,6,14,10); break;
        case 9: // swap frag / plasma grenades
            c.drawCircle(-6,0,5,p); lines(c,p,-8,-5,-8,-9,-4,-9,-4,-5);
            c.drawOval(3,-5,12,6,p); lines(c,p,6,-5,6,-8,9,-8,9,-5);
            lines(c,p,-12,12,10,12,6,8); lines(c,p,12,-13,-10,-13,-6,-9); break;
        case 10: // pause
            c.drawRect(-9,-12,-4,12,p); c.drawRect(4,-12,9,12,p); break;
        case 11: lines(c,p,12,0,-12,0,-4,-8,-12,0,-4,8); break;
        case 12: case 13: case 14: case 15:
            c.rotate(type==12 ? 0 : type==13 ? 180 : type==14 ? -90 : 90); arrow(c,p); break;
        case 16: // movement stick
            for (int i=0;i<4;i++) { c.save(); c.rotate(i*90); lines(c,p,-3,-9,0,-12,3,-9); c.restore(); }
            c.drawCircle(0,0,3,p); break;
        case 18: // camera modes
            lines(c,p,-12,-7,-5,-7,-2,-11,4,-11,7,-7,12,-7,12,7,-12,7,-12,-7);
            c.drawCircle(0,0,4,p); lines(c,p,-13,13,11,13,7,9); break;
        case 19: // show/hide overlay
            lines(c,p,-15,0,-7,-7,7,-7,15,0,7,7,-7,7,-15,0);
            c.drawCircle(0,0,4,p); if(active) lines(c,p,-13,13,13,-13); break;
        }
        p.setStyle(Paint.Style.FILL); c.restore();
    }
}
