/* Stock layout recognition, before canvas conversion. Unit HUDs include
 * bitmap geometry; weapon/grenade panels use their authored placements so
 * replacement artwork does not move an otherwise stock counter. Handles,
 * padding and colors are excluded. Grenade frames follow the counter panel,
 * independently of its overlaid icon. Unknown layouts retain the MCC frame.
 * This is a visual compatibility fingerprint, not a map identity or trust boundary. */
struct mcc_hud_layout {
    uint64_t hash;
    int panel;
};

static void mcc_hud_layout_bytes(struct mcc_hud_layout *layout,void const *data,unsigned size)
{
    unsigned char const *bytes=data;
    while (size--) {layout->hash^=*bytes++;layout->hash*=UINT64_C(1099511628211);}
}

static void mcc_hud_layout_placement(struct mcc_hud_layout *layout,unsigned char const *p)
{
    mcc_hud_layout_bytes(layout,p,14);
}

static int mcc_hud_layout_bitmap(struct mcc_runtime *r,struct mcc_hud_layout *layout,uint32_t handle)
{
    uint32_t *entry,n,count,i,sprites;
    unsigned char *p,*data,*items;
    unsigned char half;
    uint64_t hash=layout->hash;
    if (handle==UINT32_MAX) {
        if (!layout->panel) mcc_hud_layout_bytes(layout,&handle,4);
        return 1;
    }
    if ((handle&65535)>=r->report.tag_count) return 0;
    entry=(uint32_t *)(r->tag_index+(handle&65535)*32);
    if (entry[0]!='bitm' || entry[3]!=handle) return 0;
    for (n=0;n<256;n++) {
        if (entry[4]>UINT32_MAX-n) return 0;
        p=mcc_runtime_pointer(r,entry[4]+n,1);
        if (!p) return 0;
        /* Validate names without using identity to select the safe frame. */
        if (!*p) break;
    }
    if (n==256) return 0;
    p=mcc_runtime_pointer(r,entry[5],sizeof(struct bitmap_group));
    if (!p) return 0;
    half=(((struct bitmap_group *)p)->flags&(0x10|0x80))!=0;
    mcc_hud_layout_bytes(layout,&half,1);
    data=mcc_hud_block(r,p+0x60,0x30,&count);
    if (!data) return 0;
    mcc_hud_layout_bytes(layout,&count,4);
    for (n=0;n<count;n++) {
        mcc_hud_layout_bytes(layout,data+n*0x30+4,8);
        mcc_hud_layout_bytes(layout,data+n*0x30+16,4); /* registration point */
    }
    data=mcc_hud_block(r,p+0x54,0x40,&count);
    if (!data) return 0;
    mcc_hud_layout_bytes(layout,&count,4);
    for (n=0;n<count;n++) {
        mcc_hud_layout_bytes(layout,data+n*0x40+0x20,4);
        items=mcc_hud_block(r,data+n*0x40+0x34,0x20,&sprites);
        if (!items) return 0;
        mcc_hud_layout_bytes(layout,&sprites,4);
        for (i=0;i<sprites;i++) {
            mcc_hud_layout_bytes(layout,items+i*0x20,2);
            mcc_hud_layout_bytes(layout,items+i*0x20+8,24);
        }
    }
    if (layout->panel) layout->hash=hash;
    return 1;
}

static int mcc_hud_layout_element(struct mcc_runtime *r,struct mcc_hud_layout *layout,unsigned char *p,int meter)
{
    mcc_hud_layout_placement(layout,p);
    if (!layout->panel) mcc_hud_layout_bytes(layout,p+(meter ? 0x46 : 0x54),2);
    /* A new multitexture overlay makes this an authored layout. */
    mcc_hud_layout_bytes(layout,p+0x58,4);
    return mcc_hud_layout_bitmap(r,layout,mcc_hud_word(p+0x30));
}

static uint64_t mcc_hud_layout_visit(struct mcc_runtime *r,uint32_t const *entry)
{
    struct mcc_hud_layout layout={UINT64_C(14695981039346656037),0};
    unsigned char *p,*block,*items;
    uint32_t i,k,n,count,item_count;
    uint64_t counter=0;
    layout.panel=entry[0]=='wphi' || entry[0]=='grhi';
    if (entry[0]!='unhi' && entry[0]!='wphi' && entry[0]!='grhi') return 0;
    p=mcc_runtime_pointer(r,entry[5],entry[0]=='unhi' ? 0x56C : entry[0]=='wphi' ? 0x17C : 0x1F8);
    if (!p) return 0;
    mcc_hud_layout_bytes(&layout,entry,4);
    mcc_hud_layout_bytes(&layout,p+(entry[0]=='wphi' ? 0x3C : 0),2);
    if (entry[0]=='unhi') {
        static unsigned const offsets[]={0x24,0x8C,0xF4,0x17C,0x1E4,0x26C,0x2D4};
        for (i=0;i<NUMBEROF(offsets);i++)
            if (!mcc_hud_layout_element(r,&layout,p+offsets[i],i==2 || i==4)) return 0;
        mcc_hud_layout_placement(&layout,p+0x35C);
        for (k=0;k<2;k++) {
            block=mcc_hud_block(r,p+(k ? 0x3CC : 0x3A4),k ? 0x144 : 0x84,&count);
            if (!block) return 0;
            mcc_hud_layout_bytes(&layout,&count,4);
            for (i=0;i<count;i++) {
                items=block+i*(k ? 0x144 : 0x84);
                if (!mcc_hud_layout_element(r,&layout,items+(k ? 0x14 : 0),0)) return 0;
                if (k && !mcc_hud_layout_element(r,&layout,items+0x7C,1)) return 0;
            }
        }
    } else if (entry[0]=='wphi') {
        for (k=0;k<5;k++) {
            unsigned stride=k<2 ? 0xB4 : k==2 ? 0xA0 : 0x68;
            /* Crosshairs are centered independently of the corner safe frame. */
            if (k==3) continue;
            block=mcc_hud_block(r,p+0x60+k*12,stride,&count);
            if (!block) return 0;
            mcc_hud_layout_bytes(&layout,&count,4);
            for (i=0;i<count;i++) {
                unsigned char *element=block+i*stride;
                mcc_hud_layout_bytes(&layout,element,8); /* type, state, child anchor */
                if (k<2) {
                    if (!mcc_hud_layout_element(r,&layout,element+0x24,k==1)) return 0;
                } else if (k==2) {
                    mcc_hud_layout_placement(&layout,element+0x24);
                    mcc_hud_layout_bytes(&layout,element+0x68,3); /* digit layout */
                } else {
                    if (!mcc_hud_layout_bitmap(r,&layout,mcc_hud_word(element+0x30))) return 0;
                    items=mcc_hud_block(r,element+0x34,0x88,&item_count);
                    if (!items) return 0;
                    mcc_hud_layout_bytes(&layout,&item_count,4);
                    for (n=0;n<item_count;n++) {
                        unsigned char *item=items+n*0x88;
                        mcc_hud_layout_placement(&layout,item);
                        mcc_hud_layout_bytes(&layout,item+0x44,12);
                    }
                }
            }
        }
    } else {
        if (!mcc_hud_layout_element(r,&layout,p+0x24,0) ||
            !mcc_hud_layout_element(r,&layout,p+0x8C,0)) return 0;
        mcc_hud_layout_placement(&layout,p+0xF4);
        mcc_hud_layout_bytes(&layout,p+0x138,3);
        /* A replacement ability icon can change its own size and offsets
         * without moving the stock counter panel. Still validate every
         * overlay before conversion applies the panel's frame. */
        counter=layout.hash;
        if (!mcc_hud_layout_bitmap(r,&layout,mcc_hud_word(p+0x158))) return 0;
        items=mcc_hud_block(r,p+0x15C,0x88,&count);
        if (!items) return 0;
        mcc_hud_layout_bytes(&layout,&count,4);
        for (n=0;n<count;n++) {
            mcc_hud_layout_placement(&layout,items+n*0x88);
            mcc_hud_layout_bytes(&layout,items+n*0x88+0x44,12);
        }
    }
    return entry[0]=='grhi' ? counter : layout.hash;
}

static int mcc_hud_layout_stock(uint64_t hash)
{
    /* Populated from verified stock layouts; see docs/mcc_maps.md. */
    static uint64_t const stock[]={
        UINT64_C(0xfe9912f6fa6cee40), /* weapons/assault rifle/assault rifle */
        UINT64_C(0x617a95573df35cff), /* ui/hud/master rounds */
        UINT64_C(0x599b5d8c29b488d9), /* empty weapon and vehicle layouts */
        UINT64_C(0xf6a39444be51747a), /* ui/hud/cyborg */
        UINT64_C(0xdaed63dbb6c7db60), /* ui/hud/cyborg_mp */
        UINT64_C(0x3416e23306e80bdf), /* ui/hud/warthog driver */
        UINT64_C(0xf2a375ea19143970), /* ui/hud/passenger_2 */
        UINT64_C(0x84cbe724c1382a5e), /* ui/hud/gunner */
        UINT64_C(0x7d7a665fcd09ce64), /* ui/hud/scorpion */
        UINT64_C(0x1a9eab08152e6220), /* ui/hud/passenger_4 */
        UINT64_C(0xd2b79506dad6626c), /* ui/hud/passenger */
        UINT64_C(0x7ec633592a6bdf74), /* ui/hud/passenger_3 */
        UINT64_C(0xd2df5105c6b535f1), /* weapons/pistol/pistol */
        UINT64_C(0xf4c02f83062d6b36), /* ui/hud/master plasma */
        UINT64_C(0x977160e4edc42603), /* stock grenade counter panel */
        UINT64_C(0x0274dc4ea397dc8a), /* ui/hud/banshee */
        UINT64_C(0xaf47fc9928b6b6de), /* ui/hud/ghost */
        UINT64_C(0x27101a9941fef0fd), /* rocket launcher / needler */
        UINT64_C(0x13822baa01abf9cb), /* weapons/sniper rifle/sniper rifle */
        UINT64_C(0x9db3034c280a8425), /* weapons/shotgun/shotgun */
        UINT64_C(0x6c16cbbd63242ec0), /* ui/hud/master fuel */
    };
    unsigned i;
    for (i=0;i<NUMBEROF(stock);i++) if (hash==stock[i]) return hash!=0;
    return 0;
}
