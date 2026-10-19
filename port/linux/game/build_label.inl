/* The executable's release number beneath the original Xbox build label.
The generated widget owns its text; the stock label and its input stay intact. */

#ifndef HALO_BUILD_NUMBER
#define HALO_BUILD_NUMBER 0
#endif

static boolean build_label_event(struct widget_instance *widget)
{
	char const *name;
	char label[32] = "OpenCE build local";
	wchar_t wide_label[32];

	if (!widget || widget->type != 1)
		return FALSE;
	name = tag_get_name(widget->definition_tag_index);
	if (!name || strcmp(name, "pc\\main_menu\\port_build_number"))
		return FALSE;
	#if HALO_BUILD_NUMBER > 0
	snprintf(label, sizeof(label), "OpenCE build %d", HALO_BUILD_NUMBER);
	#endif
	halo_menus_utf16(label, (unsigned short *)wide_label, NUMBEROF(wide_label));
	text_set(widget, wide_label);
	return TRUE;
}
