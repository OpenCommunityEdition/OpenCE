"""Additive Xbox-style MCC browser, using existing redraws and native services."""
import xml.etree.ElementTree as ET
from pathlib import Path

NATIVE = 'ui\\shell\\main_menu\\'

BASE = 'mcc_xbox/'
ROUTES = ('campaign', 'connected', 'split', 'connected_postgame', 'split_postgame')


def append(doc):
    def widget(name, **attrs):
        return ET.SubElement(doc, 'widget', name=BASE+name,
                             **{k: str(v) for k, v in attrs.items()})

    def child(parent, name, x=0, y=0):
        ET.SubElement(parent, 'child', widget=BASE+name, x=str(x), y=str(y))

    def on(parent, event, action):
        ET.SubElement(parent, 'on', event=event, run='port mcc xbox '+action)

    def text(name, caption, x, y, width, height, small=False):
        node = widget(name, type='text', left=x, top=y,
                      width=width, height=height, font='ui\\small_ui' if small else 'ui\\large_ui',
                      color='#FF2896FF', text_flags='no_focus_test')
        if caption is not None: node.set('text', caption)
        return node

    for route in ROUTES:
        root = widget(route, width=640, height=480, bitmap='ui\\shell\\bitmaps\\gradient',
                      flags='pass_unhandled_to_focused_child')
        on(root, 'created', 'init')
        for event in ('a', 'start'): on(root, event, 'choose')
        for event in ('b', 'back'): on(root, event, 'back')
        for event in ('left', 'stick_left'): on(root, event, 'left')
        for event in ('right', 'stick_right'): on(root, event, 'right')
        for name in ('heading_campaigns', 'heading_levels', 'heading_maps', 'heading_difficulty'):
            child(root, name)
        for i in range(3): child(root, 'card_'+str(i))
        child(root, 'status')
        child(root, 'previous')
        child(root, 'next')
        ET.SubElement(root, 'child', widget=NATIVE+'button_key')
        child(root, 'difficulty_list')
    widget('heading_campaigns', left=35, top=11, width=605, height=59, bitmap='mcc_xbox/heading_campaigns')
    bitmap = ET.SubElement(doc, 'bitmap', name='mcc_xbox/heading_campaigns')
    ET.SubElement(bitmap, 'frame', png='modern/mcc_campaigns.png', width='512', height='64')
    for name, path, width in (
            ('heading_levels', 'solo_level_select\\header_load_level', 306),
            ('heading_maps', 'multiplayer_type_select\\mp_map_select\\header_select_map', 605),
            ('heading_difficulty', 'difficulty_select\\header_choose_difficulty', 390)):
        widget(name, left=35, top=11, width=width, height=59, bitmap=NATIVE+path)
    text('status', None, 64, 414, 280, 28, True)
    for i in range(3):
        x = 72+167*i
        name = 'card_'+str(i)
        card = widget(name, left=x, top=75, width=(165, 163, 162)[i], height=330 if i==0 else 326, flags='nifty_fx',
                      bitmap='ui\\shell\\bitmaps\\spinner_list_3_wide_item_background')
        on(card, 'left_mouse', 'card')
        for leaf in ('picture', 'mcc_folder_name', 'mcc_folder_help'):
            child(card, name+'/'+leaf)
        widget(name+'/picture', left=x+13, top=87, width=140, height=114,
               bitmap='ui\\shell\\bitmaps\\mp_map_grafix')
        text(name+'/mcc_folder_name', None, x+11, 204, 144+i, 43)
        text(name+'/mcc_folder_help', None, x+11, 258, 144+i, 136, True)
        for badge in range(3):
            child(card, name+'/badge_'+str(badge))
            widget(name+'/badge_'+str(badge), left=x+10+50*badge, top=343, width=44, height=48,
                   bitmap=NATIVE+'difficulty_select\\difficulty_options_small')
    for name, direction, x, width in (('previous', 'left', 50, 18), ('next', 'right', 575, 35)):
        arrow = widget(name, left=x, top=174, width=width, height=170,
                       bitmap='ui\\shell\\bitmaps\\arrow_big_'+direction)
        on(arrow, 'left_mouse', 'left' if name=='previous' else 'right')
    # Appended instances only: original Xbox tag definitions are never edited.
    for name, caption in (('campaign_entry', '=MCC CAMPAIGNS'), ('multiplayer_entry', '=MCC MAPS')):
        entry = text(name, caption, 64, 414, 280, 28)
        entry.set('text_x', '22')
        ET.SubElement(entry, 'child', widget='frontend_legacy/footer_glyph_x', x='64', y='414')
        on(entry, 'left_mouse', 'open')

    # Profile and difficulty presentation borrow unchanged native UI components
    # through their existing services. Only the MCC wrappers decide destinations.
    profile = widget('profiles', width=640, height=480, bitmap='ui\\shell\\bitmaps\\gradient',
                     flags='pass_unhandled_to_focused_child')
    ET.SubElement(profile, 'on', event='created', run='sp reset controller choices')
    for event in ('a', 'start'): on(profile, event, 'profile')
    for event in ('b', 'back'): ET.SubElement(profile, 'on', event=event, back='true')
    for path in ('player_profiles_select\\player_profile_select_list',
                 'player_profiles_select\\header_select_profile', 'button_key'):
        ET.SubElement(profile, 'child', widget=NATIVE+path)
    ET.SubElement(profile, 'on', event='y', run='create&edit player profile',
                  open=NATIVE+'settings_select\\player_setup\\create_and_edit_player_profile_screen')
    ET.SubElement(profile, 'child', widget=NATIVE+'y_butn', x='78', y='414')
    ET.SubElement(profile, 'child', widget=NATIVE+'=create_new', x='100', y='414')

    difficulty = widget('difficulty_list', type='column_list', width=640, height=480,
                        flags='pass_unhandled_to_focused_child up_down_tabs_items',
                        description=NATIVE+'difficulty_select\\difficulty_ext_desc')
    ET.SubElement(difficulty, 'data', input='difficulty menu update desc')
    ET.SubElement(difficulty, 'on', event='created', run='difficulty menu init')
    for index in range(4):
        name = 'difficulty_'+str(index)
        child(difficulty, name)
        row = widget(name, type='text', left=51, top=78+33*index, width=232, height=32,
                     bitmap='ui\\shell\\bitmaps\\list_item_bkd'+('_top' if index==0 else ''),
                     string_list=NATIVE+'difficulty_select\\difficulty_options', string_index=index,
                     font='ui\\large_ui', color='#FF0080FF', text_x=13, text_y=5)
        for event in ('a', 'start'): on(row, event, 'choose')
        ET.SubElement(row, 'on', event='left_mouse', run='mouse emit accept event')


def generate_art(target):
    """The established OpenCE heading face in the native header's padded box."""
    from PIL import Image, ImageDraw, ImageFont
    root = Path(__file__).resolve().parents[1]
    font = ImageFont.truetype(str(root/'port/assets/fonts/OpenCE-Regular.ttf'), 128,
                              layout_engine=ImageFont.Layout.BASIC)
    caption = 'MCC CAMPAIGNS'
    box = font.getbbox(caption)
    ink = Image.new('RGBA', (box[2]-box[0], box[3]-box[1]))
    ImageDraw.Draw(ink).text((-box[0], -box[1]), caption, font=font, fill=(40, 150, 255, 255))
    ink = ink.resize((round(22*ink.width/ink.height)*4, 88), Image.Resampling.LANCZOS)
    image = Image.new('RGBA', (512*4, 64*4))
    image.alpha_composite(ink, (29*4, 32*4))
    image.save(target/'mcc_campaigns.png')
