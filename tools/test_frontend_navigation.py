"""Landing-screen routes and the production scrolling/lifecycle helpers."""
import xml.etree.ElementTree as ET

import frontend_menu
import frontend_navigation
from test_frontend_menu import HEADER, ROOT, RUNTIME_STUBS, without_includes
from test_hud_button_style import run_c


def test_routes_use_existing_services_and_profile_gate():
    doc = ET.fromstring(frontend_menu.document())
    widgets = {w.get('name'): w for w in doc.findall('widget')}
    for page, (_, _, rows) in frontend_navigation.SCREENS.items():
        base = 'modern/' + page
        root = widgets[base + '/screen']
        assert root.get('width') == '640'
        assert root.find("on[@event='created']").get('run') == 'main menu intialize'
        assert widgets[base + '/help_panel'].get('flags') == 'nifty_fx'
        for _, _, key, destination, callbacks, _ in rows:
            row = widgets[base + '/row_' + key]
            assert len('row_' + key) <= 31
            assert row.get('height') == '26'
            assert row.find('child').get('y') == '3'
            for event in ('a', 'start'):
                handlers = row.findall(f"on[@event='{event}']")
                assert [h.get('run') for h in handlers if h.get('run')] == callbacks
                assert handlers[-1].get('open') == destination
        assert 'mcc' not in ET.tostring(root).decode().lower()
    # Native mode dispatch distinguishes LAN, invite and public internet by
    # instance name; losing these tokens silently selects the wrong transport.
    for key in ('join_lan_item', 'join_direct_item'):
        row = widgets['modern/multiplayer/row_' + key]
        assert row.find("on[@event='a']").get('run') == 'mp type set mode'
    for key in ('create_lan_item', 'create_internet_item'):
        row = widgets['modern/host/row_' + key]
        handlers = row.findall("on[@event='a']")
        assert handlers[0].get('run') == 'join controller to mp game'
        assert handlers[1].get('run') == 'mp type set mode'
        assert handlers[1].get('branch') == 'true'
    assert not any(name.startswith("modern/main/") for name in widgets)



def test_navigation_focus_scroll_reset_and_gameplay_guards(tmp_path):
    stubs = RUNTIME_STUBS.replace('return id==1?', 'return id==8?"pc\\\\modern\\\\multiplayer\\\\screen":id==1?')
    stubs = stubs.replace('(void)root;(void)name;(void)n;return 0;',
        'struct widget_instance *child,*found;(void)n;'
        'if(root->name&&!strcmp(root->name,name))return root;'
        'for(child=root->child;child;child=child->next){found=named(child,name,0);if(found)return found;}return 0;')
    run_c(tmp_path, stubs + HEADER +
          (ROOT / 'port/linux/game/frontend_pages.inl').read_text() +
          without_includes('port/linux/game/frontend_menu_runtime.inl') + r'''
int main(void) {
    struct widget_instance root={0},list={0},section={0},rows[20]={{0}};
    int i,deleted=0;
    root.definition_tag_index=8;root.child=&list;list.parent=&root;list.name="options";
    list.child=&section;section.name="section_0";section.parent=&list;section.vertical_offset=82;
    section.next=rows;
    for(i=0;i<20;i++) {
        rows[i].name="row_test";rows[i].visible=1;rows[i].parent=&list;
        rows[i].next=i<19?&rows[i+1]:0;rows[i].vertical_offset=110+30*i;
    }
    assert(frontend_menu_event(&root,"port frontend navigation init",&deleted));
    assert(list.focused_child==rows);
    frontend_navigation_update(&list);
    assert(frontend_menu_row_drawable(rows)&&!frontend_menu_row_drawable(rows+9));
    list.focused_child=rows+19;frontend_navigation_update(&list);
    assert(rows[19].vertical_offset==356&&frontend_menu_row_drawable(rows+19));
    assert(!frontend_menu_row_drawable(rows)&&!frontend_menu_row_drawable(&section));
    list.focused_child=rows;frontend_navigation_update(&list);
    assert(rows[0].vertical_offset==110&&section.vertical_offset==82);
    for(i=0;i<3;i++) {
        available=i!=0;main_menu=i!=1;network=i==2;
        list.focused_child=rows+19;
        frontend_navigation_update(&list);
        assert(rows[0].vertical_offset==110);
        assert(!frontend_menu_event(rows,"port frontend navigation init",&deleted));
    }
    return 0;
}
''')
