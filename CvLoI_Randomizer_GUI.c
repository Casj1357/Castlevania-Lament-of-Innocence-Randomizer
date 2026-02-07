// cvloi_randomizer_ui.c
/*
Compile: 
gcc CvLoI_Randomizer_GUI.c CvLoIRandomizerV5.c -o CvLoI_Randomizer_GUI \
    `pkg-config --cflags --libs gtk+-3.0 json-c`
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <assert.h>
#include <time.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <json-c/json.h> // Requires json-c librar
#include <pango/pango.h>
#include <glib.h>
#include "CvLoIRandomizer.h"

#define PRESET_FILE "preset.json"

/* Forward */
static void update_mode(GtkComboBoxText *combo, gpointer user_data);
static void on_enemy_random_toggled(GtkToggleButton *toggle, gpointer user_data);
//void run_randomizer(RandomizerSettings *s);

/* Widgets we'll need to show/hide/enable/disable */
typedef struct {
    GtkWidget *window;

    // top entries
    GtkWidget *entry_iso;
    GtkWidget *entry_seed;
    GtkWidget *entry_spoiler;

    // modes dropdown
    GtkWidget *mode_combo;

    // Item shuffle always forced on
    GtkWidget *chk_item_shuffle;
    GtkWidget *cmb_item_shuffle_mode;

    // Main options groups
    GtkWidget *chk_orbs_anywhere;
    GtkWidget *chk_key_items_not_on_drops;
    GtkWidget *chk_key_doors_not_require_default;
    GtkWidget *chk_warp_room;
    GtkWidget *chk_area_locking;
    GtkWidget *chk_torch_sanity;
	GtkWidget *chk_switch_shuffle;
	
	GtkWidget *chk_sphere_depth;
	GtkWidget *entry_sphere_depth;
	GtkWidget *label_sphere_depth;

    // Boss/Enemy
	GtkWidget *frame_be;
    GtkWidget *chk_boss_loadzones;
    GtkWidget *chk_doppelganger;
    GtkWidget *chk_enemy_random;
    GtkWidget *entry_enemy_percent;
    GtkWidget *label_enemy_percent_pct;
    GtkWidget *chk_enemy_hp;
    GtkWidget *chk_enemy_tolerance;

    // Changes / Subweapons / DEF / Starting Skills / QoL / Hints
    GtkWidget *frame_changes;
    GtkWidget *chk_modify_powerups;
    GtkWidget *chk_randomize_relic_mp;

    GtkWidget *frame_subweapons;
    GtkWidget *chk_subweapon_attacks;
    GtkWidget *chk_pumpkin_subweapons;
    GtkWidget *chk_subweapon_heart_costs;
    GtkWidget *chk_start_with_random_subweapon;

    GtkWidget *frame_def;
    GtkWidget *chk_armor_def_random;
    GtkWidget *chk_starting_def_random;

    GtkWidget *frame_starting_skills;
    GtkWidget *chk_show_starting_skills; // the control to hide/unhide the whole section
    // grid of skill checkboxes, arranged across then down
    GtkWidget *skill_grid;
    GPtrArray *skill_checkboxes; // dynamic list to control them

    GtkWidget *frame_qol;
    GtkWidget *chk_qol_wolfsfoot;
    GtkWidget *chk_qol_starting;
    GtkWidget *chk_qol_shop;
    GtkWidget *chk_qol_foods;
	GtkWidget *chk_qol_item_limits;
	GtkWidget *entry_item_limit;

    GtkWidget *frame_hints;
    GtkWidget *chk_hints;
    GtkWidget *chk_check_seed;

    // buttons
    GtkWidget *btn_random_seed;
    GtkWidget *btn_save_preset;
    GtkWidget *btn_submit;

} UIWidgets;

static UIWidgets ui = {0};

void show_message(const char *msg) {
    GtkWidget *dialog = gtk_message_dialog_new(
        NULL,
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "%s",
        msg
    );
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

#define SAVE_BOOL(name) \
    json_object_object_add(root, #name, \
        json_object_new_boolean(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->name))))

#define SAVE_INT(name) \
    json_object_object_add(root, #name, \
        json_object_new_int(gtk_combo_box_get_active(GTK_COMBO_BOX(ui->name))))

#define SAVE_STR(name) \
    json_object_object_add(root, #name, \
        json_object_new_string(gtk_entry_get_text(GTK_ENTRY(ui->name))))

#define LOAD_BOOL(name) \
    if (json_object_object_get_ex(root, #name, &val)) \
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui->name), json_object_get_boolean(val))

#define LOAD_INT(name) \
    if (json_object_object_get_ex(root, #name, &val)) \
        gtk_combo_box_set_active(GTK_COMBO_BOX(ui->name), json_object_get_int(val))

#define LOAD_STR(name) \
    if (json_object_object_get_ex(root, #name, &val)) \
        gtk_entry_set_text(GTK_ENTRY(ui->name), json_object_get_string(val))

static void save_preset(UIWidgets *ui) {
    json_object *root = json_object_new_object();

    /* Top-level text entries (seed intentionally excluded) */
    SAVE_STR(entry_iso);
    SAVE_STR(entry_spoiler);

    /* Combo */
    SAVE_INT(mode_combo);

    /* Item shuffle */
    SAVE_BOOL(chk_item_shuffle);
    SAVE_INT(cmb_item_shuffle_mode);

    /* Main options */
    SAVE_BOOL(chk_orbs_anywhere);
    SAVE_BOOL(chk_key_items_not_on_drops);
    SAVE_BOOL(chk_key_doors_not_require_default);
    SAVE_BOOL(chk_warp_room);
    SAVE_BOOL(chk_area_locking);
    SAVE_BOOL(chk_torch_sanity);
    SAVE_BOOL(chk_switch_shuffle);

    SAVE_BOOL(chk_sphere_depth);
    SAVE_STR(entry_sphere_depth);

    /* Boss / Enemy */
    SAVE_BOOL(chk_boss_loadzones);
    SAVE_BOOL(chk_doppelganger);
    SAVE_BOOL(chk_enemy_random);
    SAVE_STR(entry_enemy_percent);
    SAVE_BOOL(chk_enemy_hp);
    SAVE_BOOL(chk_enemy_tolerance);

    /* Changes / Stat / Subweapons / QoL / Hints */
    SAVE_BOOL(chk_modify_powerups);
    SAVE_BOOL(chk_randomize_relic_mp);

    /* Subweapons */
    SAVE_BOOL(chk_subweapon_attacks);
    SAVE_BOOL(chk_pumpkin_subweapons);
    SAVE_BOOL(chk_subweapon_heart_costs);
    SAVE_BOOL(chk_start_with_random_subweapon);

    /* DEF */
    SAVE_BOOL(chk_armor_def_random);
    SAVE_BOOL(chk_starting_def_random);

    /* Starting skills */
    SAVE_BOOL(chk_show_starting_skills);

    /* Dynamic skill list */
    json_object *skills = json_object_new_array();
    for (guint i = 0; i < ui->skill_checkboxes->len; i++) {
        GtkWidget *cb = g_ptr_array_index(ui->skill_checkboxes, i);
        json_object_array_add(
            skills,
            json_object_new_boolean(
                gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb))
            )
        );
    }
    json_object_object_add(root, "skill_checkboxes", skills);

    /* QoL */
    SAVE_BOOL(chk_qol_wolfsfoot);
    SAVE_BOOL(chk_qol_starting);
    SAVE_BOOL(chk_qol_shop);
    SAVE_BOOL(chk_qol_foods);
	SAVE_BOOL(chk_qol_item_limits);

    /* Hints */
    SAVE_BOOL(chk_hints);
    SAVE_BOOL(chk_check_seed);

    /* Write */
    json_object_to_file_ext(PRESET_FILE, root, JSON_C_TO_STRING_PRETTY);
    json_object_put(root);
}



static void on_save_preset(GtkButton *btn, gpointer data) {
    UIWidgets *ui = data;
    save_preset(ui);
	show_message("Preset saved successfully!");
}


static void load_preset(UIWidgets *ui) {
    FILE *file = fopen(PRESET_FILE, "r");
    if (!file) return;
    fclose(file);

    json_object *root = json_object_from_file(PRESET_FILE);
    if (!root) return;

    json_object *val;

    /* Top entries */
    LOAD_STR(entry_iso);
    LOAD_STR(entry_spoiler);

    /* Combo */
    LOAD_INT(mode_combo);

    /* Item shuffle */
    LOAD_BOOL(chk_item_shuffle);
    //LOAD_INT(cmb_item_shuffle_mode);

    /* Main options */
    LOAD_BOOL(chk_orbs_anywhere);
    LOAD_BOOL(chk_key_items_not_on_drops);
    LOAD_BOOL(chk_key_doors_not_require_default);
    LOAD_BOOL(chk_warp_room);
    LOAD_BOOL(chk_area_locking);
    LOAD_BOOL(chk_torch_sanity);
    LOAD_BOOL(chk_switch_shuffle);

    LOAD_BOOL(chk_sphere_depth);
    LOAD_STR(entry_sphere_depth);

    /* Boss / Enemy */
    LOAD_BOOL(chk_boss_loadzones);
    LOAD_BOOL(chk_doppelganger);
    LOAD_BOOL(chk_enemy_random);
    LOAD_STR(entry_enemy_percent);
    LOAD_BOOL(chk_enemy_hp);
    LOAD_BOOL(chk_enemy_tolerance);

    /* Changes / Stat / Subweapons / QoL / Hints */
    LOAD_BOOL(chk_modify_powerups);
    LOAD_BOOL(chk_randomize_relic_mp);

    /* Subweapons */
    LOAD_BOOL(chk_subweapon_attacks);
    LOAD_BOOL(chk_pumpkin_subweapons);
    LOAD_BOOL(chk_subweapon_heart_costs);
    LOAD_BOOL(chk_start_with_random_subweapon);

    /* DEF */
    LOAD_BOOL(chk_armor_def_random);
    LOAD_BOOL(chk_starting_def_random);

    /* Starting skills */
    LOAD_BOOL(chk_show_starting_skills);

    /* Dynamic skills */
    if (json_object_object_get_ex(root, "skill_checkboxes", &val)) {
        for (guint i = 0; i < ui->skill_checkboxes->len && 
                          i < json_object_array_length(val); i++) {

            GtkWidget *cb = g_ptr_array_index(ui->skill_checkboxes, i);
            json_object *v = json_object_array_get_idx(val, i);

            gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(cb),
                json_object_get_boolean(v)
            );
        }
    }

    /* QoL */
    LOAD_BOOL(chk_qol_wolfsfoot);
    LOAD_BOOL(chk_qol_starting);
    LOAD_BOOL(chk_qol_shop);
    LOAD_BOOL(chk_qol_foods);
	LOAD_BOOL(chk_qol_item_limits);

    /* Hints */
    LOAD_BOOL(chk_hints);
    LOAD_BOOL(chk_check_seed);

    json_object_put(root);
}


/* Helper to set widget visible and sensitive (some convenience) */
static void set_visible_sensitive(GtkWidget *w, gboolean visible, gboolean sensitive) {
    if (!w) return;
    gtk_widget_set_visible(w, visible);
    gtk_widget_set_sensitive(w, sensitive && visible);
}

/* Enforce item shuffle forced ON + disabled */
static void enforce_item_shuffle_forced() {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui.chk_item_shuffle), TRUE);
    gtk_widget_set_sensitive(ui.chk_item_shuffle, FALSE);
}

/* Mode implementations */
static void set_mode_default() {
    // Nothing hidden in default mode
    set_visible_sensitive(ui.chk_orbs_anywhere, TRUE, TRUE);
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui.chk_enemy_random)))
	{
		set_visible_sensitive(ui.chk_key_items_not_on_drops, TRUE, FALSE);
	}
	else
	{
		set_visible_sensitive(ui.chk_key_items_not_on_drops, TRUE, TRUE);
	}
    set_visible_sensitive(ui.chk_key_doors_not_require_default, TRUE, TRUE);
    set_visible_sensitive(ui.chk_warp_room, TRUE, TRUE);
    set_visible_sensitive(ui.chk_area_locking, TRUE, TRUE);
    set_visible_sensitive(ui.chk_torch_sanity, TRUE, TRUE);
	set_visible_sensitive(ui.chk_switch_shuffle, TRUE, TRUE);
	set_visible_sensitive(ui.chk_sphere_depth, TRUE, TRUE);
	if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui.chk_sphere_depth)))
	{
		set_visible_sensitive(ui.entry_sphere_depth, FALSE, FALSE);
		set_visible_sensitive(ui.label_sphere_depth, FALSE, FALSE);	
	}
	
    set_visible_sensitive(ui.chk_boss_loadzones, TRUE, TRUE);
    set_visible_sensitive(ui.chk_doppelganger, TRUE, TRUE);
    set_visible_sensitive(ui.chk_enemy_random, TRUE, TRUE);
    // percent controls depend on enemy random state
    gboolean enemy_on = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui.chk_enemy_random));
    set_visible_sensitive(ui.entry_enemy_percent, enemy_on, TRUE); //why does this not turn this hidden on initial loading?
    set_visible_sensitive(ui.label_enemy_percent_pct, enemy_on, TRUE);

    set_visible_sensitive(ui.chk_enemy_hp, TRUE, TRUE);
    set_visible_sensitive(ui.chk_enemy_tolerance, TRUE, TRUE);
	
	//Bosses and Enemies
	set_visible_sensitive(ui.frame_be, TRUE, TRUE);
	
    // Changes & Subweapons visible & toggleable
    set_visible_sensitive(ui.frame_changes, TRUE, TRUE);
    set_visible_sensitive(ui.frame_subweapons, TRUE, TRUE);

    // DEF visible
    set_visible_sensitive(ui.frame_def, TRUE, TRUE);

    // Starting skills section visible and controlled by show checkbox
    set_visible_sensitive(ui.frame_starting_skills, TRUE, TRUE);
    gboolean show_skills = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui.chk_show_starting_skills));
    set_visible_sensitive(ui.skill_grid, show_skills, TRUE);

	for (guint i = 0; i < ui.skill_checkboxes->len; i++) {
		GtkWidget *chk = g_ptr_array_index(ui.skill_checkboxes, i);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chk), FALSE);
		gtk_widget_set_sensitive(chk, TRUE);
	}

	set_visible_sensitive(ui.entry_item_limit, FALSE, FALSE);
	
    // QoL visible
    set_visible_sensitive(ui.frame_qol, TRUE, TRUE);

    // Hints hidden in default per prior decision
    set_visible_sensitive(ui.frame_hints, TRUE, TRUE); // user later requested hints not hidden in default; keep visible but checkboxes default per settings
    set_visible_sensitive(ui.chk_hints, TRUE, TRUE);
    // check_seed should be hidden in default
    set_visible_sensitive(ui.chk_check_seed, TRUE, TRUE);

    // Ensure start-with-random-subweapon visible in default
    set_visible_sensitive(ui.chk_start_with_random_subweapon, TRUE, TRUE);

    enforce_item_shuffle_forced();
}

static void set_mode_joachim() {
    // Mode 2: Joachim
    // Hide Starting Skills, Subweapons, Changes, AreaLocking, Torch-Sanity
    set_visible_sensitive(ui.frame_starting_skills, FALSE, FALSE);
    set_visible_sensitive(ui.skill_grid, FALSE, FALSE);
	
	set_visible_sensitive(ui.chk_switch_shuffle, TRUE, TRUE);
	
	//Bosses and Enemies
	set_visible_sensitive(ui.frame_be, TRUE, TRUE);
	
    set_visible_sensitive(ui.frame_subweapons, FALSE, FALSE);
    set_visible_sensitive(ui.frame_changes, FALSE, FALSE);

	set_visible_sensitive(ui.chk_key_doors_not_require_default, FALSE, FALSE);
	set_visible_sensitive(ui.chk_warp_room, FALSE, FALSE);
    set_visible_sensitive(ui.chk_area_locking, FALSE, FALSE);
    set_visible_sensitive(ui.chk_torch_sanity, FALSE, FALSE);
	set_visible_sensitive(ui.chk_orbs_anywhere, FALSE, FALSE);
	
	set_visible_sensitive(ui.chk_sphere_depth, FALSE, FALSE);
	
	set_visible_sensitive(ui.frame_def, FALSE, FALSE);
	set_visible_sensitive(ui.frame_qol, FALSE, FALSE);

    // Boss Loadzones, Doppelganger, Enemy Randomization visible, checked and insensitive
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui.chk_boss_loadzones), TRUE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui.chk_doppelganger), TRUE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui.chk_enemy_random), TRUE);

    gtk_widget_set_sensitive(ui.chk_boss_loadzones, FALSE);
    gtk_widget_set_sensitive(ui.chk_doppelganger, FALSE);
    gtk_widget_set_sensitive(ui.chk_enemy_random, FALSE);
	
	set_visible_sensitive(ui.chk_key_items_not_on_drops, TRUE, FALSE);
	set_visible_sensitive(ui.chk_boss_loadzones, TRUE, FALSE);
	set_visible_sensitive(ui.chk_doppelganger, TRUE, FALSE);
	set_visible_sensitive(ui.chk_enemy_random, TRUE, FALSE);

    // Enemy percent visible & editable (because Enemy Randomization is forced ON)
    set_visible_sensitive(ui.entry_enemy_percent, TRUE, TRUE);
    set_visible_sensitive(ui.label_enemy_percent_pct, TRUE, TRUE);

    // enemy hp/tolerance still user-toggleable
    set_visible_sensitive(ui.chk_enemy_hp, TRUE, TRUE);
    set_visible_sensitive(ui.chk_enemy_tolerance, TRUE, TRUE);

    // Hints & check_seed hidden
    set_visible_sensitive(ui.frame_hints, FALSE, FALSE);
    set_visible_sensitive(ui.chk_check_seed, FALSE, FALSE);
	
	for (guint i = 0; i < ui.skill_checkboxes->len; i++) {
		GtkWidget *chk = g_ptr_array_index(ui.skill_checkboxes, i);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chk), FALSE);
		gtk_widget_set_sensitive(chk, FALSE);
	}

    // Item shuffle always forced
    enforce_item_shuffle_forced();
}

static void set_mode_pumpkin() {
    // Mode 3: mostly default, but force torch-sanity ON & locked
    set_mode_default(); // start from default layout
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui.chk_torch_sanity), TRUE);
    gtk_widget_set_sensitive(ui.chk_torch_sanity, FALSE);

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui.chk_enemy_random)))
	{
		set_visible_sensitive(ui.chk_key_items_not_on_drops, TRUE, FALSE);
	}
	else
	{
		 set_visible_sensitive(ui.chk_key_items_not_on_drops, TRUE, TRUE);
	}
	//Bosses & Enemies
	set_visible_sensitive(ui.frame_be, TRUE, TRUE);
	
    // Hide "start with random sub-weapon"
    set_visible_sensitive(ui.chk_start_with_random_subweapon, FALSE, FALSE);
	set_visible_sensitive(ui.chk_pumpkin_subweapons, FALSE, FALSE); 
	
    // Starting skills section remains visible (user said Pumpkin mode shouldn't be able to deselect show starting skills)
    // Force starting skills to be all checked and insensitive? You previously wanted "all Starting Skills" auto selects; here user said Pumpkin shouldn't be able to deselect show starting skills.
    // We'll leave show control visible but ensure skill checkboxes remain enabled (unless you want them forced on).
    set_visible_sensitive(ui.frame_starting_skills, TRUE, TRUE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui.chk_show_starting_skills), TRUE);
    set_visible_sensitive(ui.skill_grid, TRUE, TRUE);
	
	for (guint i = 0; i < ui.skill_checkboxes->len; i++) {
		GtkWidget *chk = g_ptr_array_index(ui.skill_checkboxes, i);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chk), TRUE);
		gtk_widget_set_sensitive(chk, FALSE);
	}

	
    enforce_item_shuffle_forced();
}

static void set_mode_random_settings() {
    // Mode 4: only Item Shuffle Mode, Item Shuffle checkbox, and Check Seed visible.
    // Hide everything else except top entries and mode selection and the item shuffle controls and check_seed.
    // Top entries remain visible (iso, seed etc.)
    // Hide large groups
    set_visible_sensitive(ui.chk_orbs_anywhere, FALSE, FALSE);
    set_visible_sensitive(ui.chk_key_items_not_on_drops, FALSE, FALSE);
    set_visible_sensitive(ui.chk_key_doors_not_require_default, FALSE, FALSE);
    set_visible_sensitive(ui.chk_warp_room, FALSE, FALSE);
    set_visible_sensitive(ui.chk_area_locking, FALSE, FALSE);
    set_visible_sensitive(ui.chk_torch_sanity, FALSE, FALSE);
	set_visible_sensitive(ui.chk_switch_shuffle, FALSE, FALSE);
	set_visible_sensitive(ui.chk_sphere_depth, FALSE, FALSE);
	set_visible_sensitive(ui.entry_sphere_depth, FALSE, FALSE);
	set_visible_sensitive(ui.label_sphere_depth, FALSE, FALSE);
    set_visible_sensitive(ui.chk_boss_loadzones, FALSE, FALSE);
    set_visible_sensitive(ui.chk_doppelganger, FALSE, FALSE);
    set_visible_sensitive(ui.chk_enemy_random, FALSE, FALSE);
    set_visible_sensitive(ui.entry_enemy_percent, FALSE, FALSE);
    set_visible_sensitive(ui.label_enemy_percent_pct, FALSE, FALSE);
    set_visible_sensitive(ui.chk_enemy_hp, FALSE, FALSE);
    set_visible_sensitive(ui.chk_enemy_tolerance, FALSE, FALSE);

    set_visible_sensitive(ui.frame_changes, FALSE, FALSE);
    set_visible_sensitive(ui.frame_subweapons, FALSE, FALSE);
    set_visible_sensitive(ui.frame_def, FALSE, FALSE);
    set_visible_sensitive(ui.frame_starting_skills, FALSE, FALSE);
    set_visible_sensitive(ui.frame_qol, FALSE, FALSE);
	set_visible_sensitive(ui.frame_be, FALSE, FALSE);
	
    // Hints hidden except check_seed
    set_visible_sensitive(ui.frame_hints, TRUE, TRUE);
	set_visible_sensitive(ui.chk_hints, FALSE, FALSE);
    set_visible_sensitive(ui.chk_check_seed, TRUE, TRUE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui.chk_check_seed), TRUE);
    
	// Item controls visible / forced
    set_visible_sensitive(ui.chk_item_shuffle, TRUE, FALSE);
    //set_visible_sensitive(ui.cmb_item_shuffle_mode, TRUE, TRUE);

    enforce_item_shuffle_forced();
}

/* Updates the UI according to selected mode */
static void update_mode(GtkComboBoxText *combo, gpointer user_data) {
    const gchar *mode_text = gtk_combo_box_text_get_active_text(combo);

    if (!mode_text) return;

    if (g_strcmp0(mode_text, "Default") == 0) {
        set_mode_default();
    } else if (g_strcmp0(mode_text, "Joachim") == 0) {
        set_mode_joachim();
    } else if (g_strcmp0(mode_text, "Pumpkin") == 0) {
        set_mode_pumpkin();
    } else if (g_strcmp0(mode_text, "Random") == 0) { // Random / Mode 4 / Random Settings
        set_mode_random_settings();
    } else {
		load_preset(&ui);
	}

    g_free((gchar*)mode_text);
}

/* Enemy Randomization toggled - show/hide percent entry and "%" label */
static void on_enemy_random_toggled(GtkToggleButton *toggle, gpointer user_data) {
    gboolean active = gtk_toggle_button_get_active(toggle);
    set_visible_sensitive(ui.entry_enemy_percent, active, TRUE);
    set_visible_sensitive(ui.label_enemy_percent_pct, active, TRUE);

    // enforce relation: if Enemy Randomization is unchecked, also uncheck "key items not on drops" if desired
    if (!active) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui.chk_key_items_not_on_drops), FALSE);
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui.chk_enemy_random)))
		{
			set_visible_sensitive(ui.chk_key_items_not_on_drops, TRUE, FALSE);
		}
		else
		{
			 set_visible_sensitive(ui.chk_key_items_not_on_drops, TRUE, TRUE);
		}
    } else {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui.chk_key_items_not_on_drops), TRUE);
        gtk_widget_set_sensitive(ui.chk_key_items_not_on_drops, FALSE);
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui.chk_enemy_random)))
		{
			set_visible_sensitive(ui.chk_key_items_not_on_drops, TRUE, FALSE);
		}
		else
		{
			 set_visible_sensitive(ui.chk_key_items_not_on_drops, TRUE, TRUE);
		}
    }
}

static void on_sphere_depth_toggled(GtkToggleButton *toggle, gpointer user_data) {
	gboolean active = gtk_toggle_button_get_active(toggle);
	set_visible_sensitive(ui.entry_sphere_depth, active, TRUE);
	set_visible_sensitive(ui.label_sphere_depth, active, TRUE);
	
	if(active)
	{
		set_visible_sensitive(ui.chk_area_locking, TRUE, FALSE);
		set_visible_sensitive(ui.chk_key_doors_not_require_default, TRUE, FALSE);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui.chk_area_locking), FALSE);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui.chk_key_doors_not_require_default), FALSE);
	}
	else
	{
		set_visible_sensitive(ui.chk_area_locking, TRUE, TRUE);
		set_visible_sensitive(ui.chk_key_doors_not_require_default, TRUE, TRUE);
	}
}

static void on_boss_loadzones_toggled(GtkToggleButton *toggle, gpointer user_data) {
	gboolean active = gtk_toggle_button_get_active(toggle);
	if(active)
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui.chk_doppelganger), TRUE);
	}
}

static void on_item_limits_toggled(GtkToggleButton *toggle, gpointer user_data) {
	gboolean active = gtk_toggle_button_get_active(toggle);
	set_visible_sensitive(ui.entry_item_limit, active, active);
}

/* Helper to create checkboxes and return them easily */
static GtkWidget* mk_check(const char *label) {
    return gtk_check_button_new_with_label(label);
}

static void on_show_starting_skills_toggled(GtkToggleButton *toggle, gpointer user_data)
{
    UIWidgets *ui = (UIWidgets*)user_data; // your UI struct

    gboolean show = gtk_toggle_button_get_active(toggle);

    // Show/hide the skill grid and make its children sensitive
    set_visible_sensitive(ui->skill_grid, show, TRUE);
}

static void on_browse_iso_clicked(GtkButton *button, gpointer user_data) {
    UIWidgets *ui = (UIWidgets*)user_data;

    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Select ISO",
        GTK_WINDOW(ui->window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        NULL
    );

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        gtk_entry_set_text(GTK_ENTRY(ui->entry_iso), filename);
        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

static void on_browse_spl_clicked(GtkButton *button, gpointer user_data) {
    UIWidgets *ui = (UIWidgets*)user_data;

    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Select Spoiler Log",
        GTK_WINDOW(ui->window),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Save", GTK_RESPONSE_ACCEPT,
        NULL
    );

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        gtk_entry_set_text(GTK_ENTRY(ui->entry_spoiler), filename);
        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}
static void on_random_seed_clicked(GtkButton *button, gpointer user_data) {
    UIWidgets *ui = (UIWidgets*)user_data;

    guint seed = g_random_int();
    gchar seed_str[32];
    g_snprintf(seed_str, sizeof(seed_str), "%u", seed);
    gtk_entry_set_text(GTK_ENTRY(ui->entry_seed), seed_str);
}

static void debug_null(const char *name, GtkWidget *w) {
    if (!w) g_warning("WIDGET IS NULL: %s", name);
}

static void attach_tooltips(UIWidgets *ui) { 

	gtk_widget_set_tooltip_text(ui->entry_iso,
		"FilePath to Castlevania: Lament of Innocence .iso (NTSC-U version)");

	gtk_widget_set_tooltip_text(ui->entry_seed,
		"Choose your seed. Using the same seed will always give the same result.");

	gtk_widget_set_tooltip_text(ui->entry_spoiler,
		"Write a file that lists what each location contains.");

	gtk_widget_set_tooltip_text(ui->mode_combo,
		"If you intend to use a different game mode, select the appropriate version.");

	gtk_widget_set_tooltip_text(ui->chk_boss_loadzones,
		"Randomizes Bosses.");

	gtk_widget_set_tooltip_text(ui->chk_doppelganger,
		"Randomizes the Doppelganger fights (may swap them).");

	gtk_widget_set_tooltip_text(ui->chk_enemy_hp,
		"Randomizes enemy HP (including bosses).");

	gtk_widget_set_tooltip_text(ui->chk_enemy_tolerance,
		"Randomizes enemy elemental tolerances / weaknesses.");

	gtk_widget_set_tooltip_text(ui->chk_enemy_random,
		"Randomizes which enemies spawn in each location.");

	gtk_widget_set_tooltip_text(ui->entry_enemy_percent,
		"Percent chance an enemy will be replaced when enemy randomization is enabled.");

	gtk_widget_set_tooltip_text(ui->chk_modify_powerups,
		"Changes how much of an increase (or decrease) Power-Ups provide.");

	gtk_widget_set_tooltip_text(ui->chk_randomize_relic_mp,
		"Randomizes MP costs by assigning costs from random relics.");
		
	gtk_widget_set_tooltip_text(ui->chk_subweapon_attacks,
		"Each sub-weapon attack can be a different one.");

	gtk_widget_set_tooltip_text(ui->chk_pumpkin_subweapons,
		"Replaces certain sub-weapon attacks with their Pumpkin Mode versions.");

	gtk_widget_set_tooltip_text(ui->chk_subweapon_heart_costs,
		"Randomizes how many hearts each sub-weapon attack uses.");

	gtk_widget_set_tooltip_text(ui->chk_start_with_random_subweapon,
		"Start the game with one of the five base sub-weapons chosen at random.");

	gtk_widget_set_tooltip_text(ui->chk_armor_def_random,
		"Randomizes the DEF value granted by armors.");

	gtk_widget_set_tooltip_text(ui->chk_starting_def_random,
		"Randomizes how much DEF Leon starts the game with.");

	gtk_widget_set_tooltip_text(ui->frame_starting_skills,
		"Choose which skills you wish to start the game with.");

	gtk_widget_set_tooltip_text(ui->chk_qol_wolfsfoot,
		"Rinaldo sells the Wolf's Foot for $-1, and its MP drains very slowly.");

	gtk_widget_set_tooltip_text(ui->chk_qol_starting,
		"Start the game with money, MP, and hearts.");

	gtk_widget_set_tooltip_text(ui->chk_qol_shop,
		"The shop will sell additional items.");

	gtk_widget_set_tooltip_text(ui->chk_qol_foods,
		"Each food randomly heals HP, MP, or Hearts.");

	gtk_widget_set_tooltip_text(ui->chk_hints,
		"Write additional hints onto some item descriptions.");

	gtk_widget_set_tooltip_text(ui->chk_check_seed,
		"Validate that the final boss can be beaten with the randomized setup.");

}

unsigned int hash_string(const char* str)
{
	unsigned int hash = 0;
	while (*str) {
		hash = hash * 31 + (unsigned char)(*str);
		str++;
	}
	return hash;
}

static int safe_atoi(const char *t) {
    if (!t || !*t) return 0;
    return atoi(t);
}

#define MODE_DEFAULT 0
#define MODE_JOACHIM 1
#define MODE_PUMPKIN 2
#define MODE_RANDOM 3

static void enforce_mode_constraints(RandomizerSettings* s)
{
	printf("mode: %d \n", s->mode);
	switch(s->mode)
	{
		case MODE_DEFAULT:
			break;
		
		case MODE_JOACHIM:
			s->item_shuffle_enabled = 1;
			s->key_items_not_on_drops = 0;
			s->orbs_anywhere = 0;
			s->area_locking = 0;
			s->torch_sanity = 0;
			s->key_doors_not_require_default = 0;
			s->sphere_depth_enabled = 0;
			s->boss_loadzones = 1;
			s->doppelganger = 1;
			s->enemy_random = 1;
			s->modify_powerups = 0;
			s->randomize_relic_mp = 0;
			s->armor_def_random = 0;
			s->starting_def_random = 0;
			s->qol_wolfsfoot = 0;
			s->qol_starting = 0;
			s->qol_shop = 0;
			s->qol_foods = 0;
			s->qol_item_limits = 0;
			break;
			
		case MODE_PUMPKIN:
			s->start_with_random_subweapon = 0;
			s->pumpkin_subweapons = 0;
			break;
			
		case MODE_RANDOM: {
			srand(hash_string(s->seed_string));
			int Num_Settings = 42;
			int settings[Num_Settings];
			for(int i = 0; i < Num_Settings; i++)
			{
				settings[i] = rand() % 2;
			}
			s->item_shuffle_enabled = 1;
			s->orbs_anywhere = settings[0];
			s->key_items_not_on_drops = 1;
			s->key_doors_not_require_default = settings[1];
			s->warp_room_random = settings[2];
			s->area_locking = settings[3];
			s->switch_shuffle = settings[4];
			s->sphere_depth_enabled = 0;
			s->boss_loadzones = settings[5];
			s->doppelganger = settings[6];
			s->enemy_random = settings[7];
			s->enemy_hp = settings[8];
			s->enemy_tolerance = settings[9];
			s->modify_powerups = settings[10];
			s->randomize_relic_mp = settings[11];
			s->subweapon_attacks = settings[12];
			s->pumpkin_subweapons = settings[13];
			s->subweapon_heart_costs = settings[14];
			s->start_with_random_subweapon = settings[15];
			s->qol_wolfsfoot = settings[31];
			s->qol_starting = settings[32];
			s->qol_shop = settings[33];
			s->qol_foods = settings[34];
			s->item_limit = 0;
			s->hints = settings[35];
			s->check_seed = 1;
			break;
		}
	}
}

static void collect_settings(RandomizerSettings *s, UIWidgets *ui)
{
    memset(s, 0, sizeof(*s));

    /* Top entries */
    strncpy(s->iso_path, gtk_entry_get_text(GTK_ENTRY(ui->entry_iso)), MAX_PATH-1);
	s->iso_path[MAX_PATH-1] = '\0';  // ensure null termination

	strncpy(s->seed_string, gtk_entry_get_text(GTK_ENTRY(ui->entry_seed)), MAX_PATH-1);
	s->seed_string[MAX_PATH-1] = '\0';

	strncpy(s->spoiler_path, gtk_entry_get_text(GTK_ENTRY(ui->entry_spoiler)), MAX_PATH-1);
	s->spoiler_path[MAX_PATH-1] = '\0';


    /* Mode */
    s->mode = gtk_combo_box_get_active(GTK_COMBO_BOX(ui->mode_combo));

    /* Item shuffle */
    s->item_shuffle_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->chk_item_shuffle));
    s->item_shuffle_mode    = gtk_combo_box_get_active(GTK_COMBO_BOX(ui->cmb_item_shuffle_mode));

    /* Main progression logic */
    s->orbs_anywhere                 = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->chk_orbs_anywhere));
    s->key_items_not_on_drops        = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->chk_key_items_not_on_drops));
    s->key_doors_not_require_default = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->chk_key_doors_not_require_default));
    s->warp_room_random              = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->chk_warp_room));
    s->area_locking                  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->chk_area_locking));
    s->torch_sanity                  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->chk_torch_sanity));
    s->switch_shuffle                = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->chk_switch_shuffle));

    /* Sphere depth */
    s->sphere_depth_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->chk_sphere_depth));
    s->sphere_depth         = safe_atoi(gtk_entry_get_text(GTK_ENTRY(ui->entry_sphere_depth)));

    /* Boss & enemy randomization */
    s->boss_loadzones     = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->chk_boss_loadzones));
    s->doppelganger       = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->chk_doppelganger));
    s->enemy_random       = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->chk_enemy_random));
    s->enemy_percent      = safe_atoi(gtk_entry_get_text(GTK_ENTRY(ui->entry_enemy_percent)));
    s->enemy_hp           = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->chk_enemy_hp));
    s->enemy_tolerance    = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->chk_enemy_tolerance));

    /* Changes */
    s->modify_powerups       = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->chk_modify_powerups));
    s->randomize_relic_mp    = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->chk_randomize_relic_mp));

    /* Subweapons */
    s->subweapon_attacks          = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->chk_subweapon_attacks));
    s->pumpkin_subweapons         = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->chk_pumpkin_subweapons));
    s->subweapon_heart_costs      = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->chk_subweapon_heart_costs));
    s->start_with_random_subweapon= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->chk_start_with_random_subweapon));

    /* DEF / stats */
    s->armor_def_random     = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->chk_armor_def_random));
    s->starting_def_random  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->chk_starting_def_random));

    /* Starting skills */
    s->starting_skills_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->chk_show_starting_skills));

    /* Gather all skill-checkbox states into a bitfield or array */
    int skill_count = ui->skill_checkboxes->len;
    s->num_skills = skill_count;

    for (int i = 0; i < skill_count; i++) {
        GtkWidget *w = g_ptr_array_index(ui->skill_checkboxes, i);
        s->skills[i] = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
    }

    /* QoL */
    s->qol_wolfsfoot  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->chk_qol_wolfsfoot));
    s->qol_starting   = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->chk_qol_starting));
    s->qol_shop       = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->chk_qol_shop));
    s->qol_foods      = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->chk_qol_foods));
	s->qol_item_limits = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->chk_qol_item_limits));
	s->item_limit = safe_atoi(gtk_entry_get_text(GTK_ENTRY(ui->entry_item_limit)));
    
	/* Hints + seed validity */
    s->hints      = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->chk_hints));
    s->check_seed = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->chk_check_seed));
}

static void on_submit(GtkButton *btn, gpointer data)
{
    UIWidgets *ui = data;

    RandomizerSettings s;
    collect_settings(&s, ui);

	enforce_mode_constraints(&s);

    run_randomizer(&s); //CvLoIRandomizerV5.c (main randomizer code)

    show_message("Randomization complete!");
}



int main(int argc, char *argv[]) {
	g_setenv("GTK_USE_PORTAL", "0", TRUE);
	g_setenv("GSETTINGS_BACKEND", "memory", TRUE);
	
    gtk_init(&argc, &argv);

    /* Window */
    ui.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(ui.window), "Castlevania: Lament of Innocence Randomizer");
    gtk_container_set_border_width(GTK_CONTAINER(ui.window), 8);
    gtk_window_set_default_size(GTK_WINDOW(ui.window), 900, 720);

    g_signal_connect(ui.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    /* Vertical main box */
    //GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    //gtk_container_add(GTK_CONTAINER(ui.window), vbox);

	/* Vertical main box */
	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

	/* --- Wrap vbox in a scrolled window --- */
	GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_vexpand(scrolled, TRUE);
	gtk_widget_set_hexpand(scrolled, TRUE);

	/* Add scrolled window to the main window */
	gtk_container_add(GTK_CONTAINER(ui.window), scrolled);

	/* Put the vbox inside the scrolled window */
	gtk_container_add(GTK_CONTAINER(scrolled), vbox);

	
    /* --- Top rows: file entries --- */
    GtkWidget *grid_top = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid_top), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid_top), 6);
    gtk_box_pack_start(GTK_BOX(vbox), grid_top, FALSE, FALSE, 0);

    GtkWidget *lbl_iso = gtk_label_new("FilePath to Castlevania: Lament of Innocence NTSC-U .iso\n    (This will overwrite this file)");
    gtk_label_set_xalign(GTK_LABEL(lbl_iso), 0.0);
    ui.entry_iso = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(ui.entry_iso), "./CvLoI.iso");
    GtkWidget *btn_browse_iso = gtk_button_new_with_label("Browse");

    GtkWidget *lbl_seed = gtk_label_new("Seed (Leave blank for random)");
    gtk_label_set_xalign(GTK_LABEL(lbl_seed), 0.0);
    ui.entry_seed = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(ui.entry_seed), ""); // blank by default

    GtkWidget *lbl_spl = gtk_label_new("SpoilerLog");
    gtk_label_set_xalign(GTK_LABEL(lbl_spl), 0.0);
    ui.entry_spoiler = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(ui.entry_spoiler), "./Spllog.txt");
    GtkWidget *btn_browse_spl = gtk_button_new_with_label("Browse");

    gtk_grid_attach(GTK_GRID(grid_top), lbl_iso, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_top), ui.entry_iso, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_top), btn_browse_iso, 2, 0, 1, 1);

    gtk_grid_attach(GTK_GRID(grid_top), lbl_seed, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_top), ui.entry_seed, 1, 1, 1, 1);

    gtk_grid_attach(GTK_GRID(grid_top), lbl_spl, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_top), ui.entry_spoiler, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_top), btn_browse_spl, 2, 2, 1, 1);

	g_signal_connect(btn_browse_iso, "clicked", G_CALLBACK(on_browse_iso_clicked), &ui);
	g_signal_connect(btn_browse_spl, "clicked", G_CALLBACK(on_browse_spl_clicked), &ui);


    /* Mode combo */
    GtkWidget *lbl_mode = gtk_label_new("Item Shuffle Mode");
    gtk_label_set_xalign(GTK_LABEL(lbl_mode), 0.0);
    ui.mode_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ui.mode_combo), "Default");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ui.mode_combo), "Joachim");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ui.mode_combo), "Pumpkin");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ui.mode_combo), "Random");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ui.mode_combo), "Preset");
	gtk_combo_box_set_active(GTK_COMBO_BOX(ui.mode_combo), 0);

    gtk_grid_attach(GTK_GRID(grid_top), lbl_mode, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_top), ui.mode_combo, 1, 3, 1, 1);

    /* Item shuffle forced checkbox + mode dropdown */
    ui.chk_item_shuffle = mk_check("Item Shuffle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui.chk_item_shuffle), TRUE);
    gtk_widget_set_sensitive(ui.chk_item_shuffle, FALSE);
    //gtk_combo_box_set_active(GTK_COMBO_BOX(ui.cmb_item_shuffle_mode), 0);

    gtk_box_pack_start(GTK_BOX(vbox), ui.chk_item_shuffle, FALSE, FALSE, 0);
    //gtk_box_pack_start(GTK_BOX(vbox), ui.cmb_item_shuffle_mode, FALSE, FALSE, 0);

    /* Main options grid */
    GtkWidget *main_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(main_grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(main_grid), 12);
    gtk_box_pack_start(GTK_BOX(vbox), main_grid, FALSE, FALSE, 0);

    ui.chk_orbs_anywhere = mk_check("orbs/whips anywhere");
    ui.chk_key_items_not_on_drops = mk_check("key items not on drops");
    ui.chk_key_doors_not_require_default = mk_check("key doors could not require default key");
    ui.chk_warp_room = mk_check("warp room randomized");
    ui.chk_area_locking = mk_check("areaLocking");
    ui.chk_torch_sanity = mk_check("torch-Sanity");
	ui.chk_switch_shuffle = mk_check("switch room shuffle");
	
	ui.chk_sphere_depth = mk_check("sphere depth");
	ui.entry_sphere_depth = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(ui.entry_sphere_depth), "2");
	ui.label_sphere_depth = gtk_label_new("(min 2, max 11)");

    gtk_grid_attach(GTK_GRID(main_grid), ui.chk_orbs_anywhere, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(main_grid), ui.chk_key_items_not_on_drops, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(main_grid), ui.chk_key_doors_not_require_default, 2, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(main_grid), ui.chk_warp_room, 3, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(main_grid), ui.chk_area_locking, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(main_grid), ui.chk_torch_sanity, 1, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(main_grid), ui.chk_switch_shuffle, 2, 1, 1, 1);
	
	GtkWidget *h_sphere = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_box_pack_start(GTK_BOX(h_sphere), ui.entry_sphere_depth, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(h_sphere), ui.label_sphere_depth, FALSE, FALSE, 0);
	gtk_grid_attach(GTK_GRID(main_grid), ui.chk_sphere_depth, 3, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(main_grid), h_sphere,            3, 2, 1, 1);


    /* Boss / Enemy group */
	ui.frame_be = gtk_frame_new("Bosses and Enemies");
	gtk_box_pack_start(GTK_BOX(vbox), ui.frame_be, FALSE, FALSE, 0);

	GtkWidget *grid_be = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(grid_be), 4);
	gtk_grid_set_column_spacing(GTK_GRID(grid_be), 8);
	gtk_container_add(GTK_CONTAINER(ui.frame_be), grid_be);

	ui.chk_boss_loadzones = mk_check("Boss Loadzones random");
	ui.chk_doppelganger   = mk_check("Doppelganger fights random");
	ui.chk_enemy_hp       = mk_check("Enemy HP randomization");
	ui.chk_enemy_tolerance= mk_check("Enemy Tolerances/Weaknesses Random");
	ui.chk_enemy_random   = mk_check("Enemy Randomization");

	ui.entry_enemy_percent = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(ui.entry_enemy_percent), "85");
	ui.label_enemy_percent_pct = gtk_label_new("%");

	/* Horizontal box for percent input */
	GtkWidget *h_enemy = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_box_pack_start(GTK_BOX(h_enemy), ui.entry_enemy_percent, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(h_enemy), ui.label_enemy_percent_pct, FALSE, FALSE, 0);

	/* Attach to grid */
	gtk_grid_attach(GTK_GRID(grid_be), ui.chk_boss_loadzones,   0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_be), ui.chk_doppelganger,     0, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_be), ui.chk_enemy_hp,         1, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_be), ui.chk_enemy_tolerance,  1, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid_be), ui.chk_enemy_random,     0, 4, 1, 1);

	gtk_grid_attach(GTK_GRID(grid_be), h_enemy,                 1, 4, 1, 1);


    /* Connect enemy random toggled */
    g_signal_connect(ui.chk_enemy_random, "toggled", G_CALLBACK(on_enemy_random_toggled), NULL);

	g_signal_connect(ui.chk_sphere_depth, "toggled", G_CALLBACK(on_sphere_depth_toggled), NULL); 
	
	g_signal_connect(ui.chk_boss_loadzones, "toggled", G_CALLBACK(on_boss_loadzones_toggled), NULL);

    /* Changes frame */
    ui.frame_changes = gtk_frame_new("Changes");
    gtk_box_pack_start(GTK_BOX(vbox), ui.frame_changes, FALSE, FALSE, 0);
    GtkWidget *grid_changes = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(grid_changes), 4);
	gtk_grid_set_column_spacing(GTK_GRID(grid_changes), 12);
	gtk_container_add(GTK_CONTAINER(ui.frame_changes), grid_changes);

    ui.chk_modify_powerups = mk_check("Modify Power_ups");
    ui.chk_randomize_relic_mp = mk_check("Randomize Relic MP costs");
    
	GtkWidget *change_checks[] = {
		ui.chk_modify_powerups,
		ui.chk_randomize_relic_mp
	};

	for (int i = 0; i < 2; i++) {
		gtk_grid_attach(GTK_GRID(grid_changes), change_checks[i], i, 0, 1, 1);
	}


    /* Subweapons frame */
    ui.frame_subweapons = gtk_frame_new("Subweapons");
	gtk_box_pack_start(GTK_BOX(vbox), ui.frame_subweapons, FALSE, FALSE, 0);

	GtkWidget *grid_sub = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(grid_sub), 4);
	gtk_grid_set_column_spacing(GTK_GRID(grid_sub), 12);
	gtk_container_add(GTK_CONTAINER(ui.frame_subweapons), grid_sub);

    ui.chk_subweapon_attacks = mk_check("sub-weapon attacks random");
    ui.chk_pumpkin_subweapons = mk_check("pumpkin sub-weapons");
    ui.chk_subweapon_heart_costs = mk_check("sub-weapon heart costs random");
    ui.chk_start_with_random_subweapon = mk_check("start with random sub-weapon");

	GtkWidget *subs[] = {
		ui.chk_subweapon_attacks,
		ui.chk_pumpkin_subweapons,
		ui.chk_subweapon_heart_costs,
		ui.chk_start_with_random_subweapon
	};

	for (int i = 0; i < 4; i++) {
		int r = i / 4; // 4 columns
		int c = i % 4;
		gtk_grid_attach(GTK_GRID(grid_sub), subs[i], c, r, 1, 1);
	}

    /* DEF frame */
    ui.frame_def = gtk_frame_new("DEF");
    gtk_box_pack_start(GTK_BOX(vbox), ui.frame_def, FALSE, FALSE, 0);
    GtkWidget *grid_def = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(grid_def), 4);
	gtk_grid_set_column_spacing(GTK_GRID(grid_def), 12);
	gtk_container_add(GTK_CONTAINER(ui.frame_def), grid_def);

    ui.chk_armor_def_random = mk_check("Armor DEF random");
    ui.chk_starting_def_random = mk_check("starting DEF random");

	GtkWidget *def_checks[] = {
		ui.chk_armor_def_random,
		ui.chk_starting_def_random
	};

	for (int i = 0; i < 2; i++) {
		gtk_grid_attach(GTK_GRID(grid_def), def_checks[i], i, 0, 1, 1);
	}

	
    /* Starting skills frame (with show toggle) */
    ui.frame_starting_skills = gtk_frame_new("Starting Skills");
    gtk_box_pack_start(GTK_BOX(vbox), ui.frame_starting_skills, FALSE, FALSE, 0);
    GtkWidget *box_sk = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_add(GTK_CONTAINER(ui.frame_starting_skills), box_sk);

    ui.chk_show_starting_skills = mk_check("Show starting skills (toggle to hide)");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui.chk_show_starting_skills), TRUE);
    gtk_box_pack_start(GTK_BOX(box_sk), ui.chk_show_starting_skills, FALSE, FALSE, 0);

    // Skill grid arranged across then down 
    ui.skill_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(ui.skill_grid), 4);
    gtk_grid_set_column_spacing(GTK_GRID(ui.skill_grid), 12);
    gtk_box_pack_start(GTK_BOX(box_sk), ui.skill_grid, FALSE, FALSE, 0);

    const char *skill_names[] = {
        "Extension", "Draw Up", "Vertical High", "Rising Shot",
        "Fast Rising", "Spinning Blast", "Energy Blast", "Sonic Edge",
        "A Extension 1", "A Extension 2", "Step Attack", "Falcon Claw",
        "Quick Step", "Quick Step 2", "Perfect Guard"
    };
    const int skills_count = sizeof(skill_names)/sizeof(skill_names[0]);
    ui.skill_checkboxes = g_ptr_array_new_with_free_func(g_free);

    int cols = 8;
    for (int i = 0; i < skills_count; ++i) {
        GtkWidget *cb = mk_check(skill_names[i]);
        int r = i / cols;
        int c = i % cols;
        gtk_grid_attach(GTK_GRID(ui.skill_grid), cb, c, r, 1, 1);
        g_ptr_array_add(ui.skill_checkboxes, cb);
    }
    // remove the extra "unused" checkbox: not applicable because we created exactly skill_count checkboxes

    /* QoL frame */
    ui.frame_qol = gtk_frame_new("QoL");
    gtk_box_pack_start(GTK_BOX(vbox), ui.frame_qol, FALSE, FALSE, 0);
    GtkWidget *grid_qol = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(grid_qol), 4);
	gtk_grid_set_column_spacing(GTK_GRID(grid_qol), 12);
	gtk_container_add(GTK_CONTAINER(ui.frame_qol), grid_qol);

    ui.chk_qol_wolfsfoot = mk_check("QoL Wolf's Foot");
    ui.chk_qol_starting = mk_check("QoL start with $, MP, and <3s");
    ui.chk_qol_shop = mk_check("QoL Modified Shop");
    ui.chk_qol_foods = mk_check("foods can heal HP/MP/<3s randomly");
	ui.chk_qol_item_limits = mk_check("item_limits");
	ui.entry_item_limit = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(ui.entry_item_limit), "9");
	GtkWidget *h_item = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_box_pack_start(GTK_BOX(h_item), ui.entry_item_limit, FALSE, FALSE, 0);
	gtk_grid_attach(GTK_GRID(grid_qol), h_item,                 1, 1, 1, 1);
	
	GtkWidget *qol_checks[] = {
		ui.chk_qol_wolfsfoot,
		ui.chk_qol_starting,
		ui.chk_qol_shop,
		ui.chk_qol_foods,
		ui.chk_qol_item_limits,
	};

	int cols_qol = 4;
	int count_qol = 5;

	for (int i = 0; i < count_qol; i++) {
		int r = i / cols_qol;
		int c = i % cols_qol;
		gtk_grid_attach(GTK_GRID(grid_qol), qol_checks[i], c, r, 1, 1);
	}
	
	g_signal_connect(ui.chk_qol_item_limits, "toggled", G_CALLBACK(on_item_limits_toggled), NULL);


    /* Hints frame */
    ui.frame_hints = gtk_frame_new("Hints");
    gtk_box_pack_start(GTK_BOX(vbox), ui.frame_hints, FALSE, FALSE, 0);
    GtkWidget *box_h = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_container_add(GTK_CONTAINER(ui.frame_hints), box_h);
    ui.chk_hints = mk_check("Hints");
    ui.chk_check_seed = mk_check("check_seed");
    gtk_box_pack_start(GTK_BOX(box_h), ui.chk_hints, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box_h), ui.chk_check_seed, FALSE, FALSE, 0);

    /* Buttons */
    GtkWidget *h_buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_pack_end(GTK_BOX(vbox), h_buttons, FALSE, FALSE, 0);
    ui.btn_random_seed = gtk_button_new_with_label("Random Seed");
    ui.btn_save_preset = gtk_button_new_with_label("Save Preset");
    ui.btn_submit = gtk_button_new_with_label("Submit");

    gtk_box_pack_end(GTK_BOX(h_buttons), ui.btn_submit, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(h_buttons), ui.btn_save_preset, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(h_buttons), ui.btn_random_seed, FALSE, FALSE, 0);

	g_signal_connect(ui.btn_random_seed, "clicked", G_CALLBACK(on_random_seed_clicked), &ui);
	
    /* Connect mode change */
    g_signal_connect(ui.mode_combo, "changed", G_CALLBACK(update_mode), NULL);

    /* Connect show starting skills toggler */
    g_signal_connect(ui.chk_show_starting_skills, "toggled",
                 G_CALLBACK(on_show_starting_skills_toggled),
                 &ui);

	g_signal_connect(ui.btn_save_preset, "clicked",
			 G_CALLBACK(on_save_preset), &ui);

	
    gtk_widget_show_all(ui.window);
    // But ensure behavior: item shuffle insensitive & seed blank preserved
    
	/* Initialize UI to Default */
    set_mode_default(); 
	
	load_preset(&ui);
	
	enforce_item_shuffle_forced();
    gtk_entry_set_text(GTK_ENTRY(ui.entry_seed), "");
	
	attach_tooltips(&ui);
	
	g_signal_connect(ui.btn_submit, "clicked", G_CALLBACK(on_submit), &ui);
	
    gtk_main();
    return 0;
}



/* current issues:

*/

/* TODO:
TODO: Tool-tips:
//mising main randomizer checkboxes //later

(Current)
TODO: Submit passes the 'chooses' to the actual randomizer...

TODO: Make sure works on Windows, Linux, Android
TODO: PlayStation3 (if possible)

//PCSX2, ???, AetherSX2, PS3
*/

/* GUI 

*/