//Castlevania: Lament of Innocence
//Randomizer
#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <gdk/gdk.h>
#include <json-c/json.h> // Requires json-c librar
#include <pango/pango.h>
#include <unistd.h>

#define ROWS 6
#define COLS 8
#define TOTAL_CHECKBOXES 47
#define TOTAL_USED_CHECKBOXES 47

#define PRESET_FILE "00preset.json"

#define true 1
#define false 0

_Bool DEBUG = false;


typedef struct {
    GtkEntry *entries[3];   // Store GtkEntry pointers
    GtkWidget *checkboxes[TOTAL_CHECKBOXES];
	GtkWidget *random_seed_button;
    GtkWidget *save_button;  // Fix: Add missing member
    GtkWidget *submit_button; // Fix: Add missing member
} CheckBoxData;

void scale_text(GtkWidget *widget, int size) {
    GtkStyleContext *context = gtk_widget_get_style_context(widget);
    char css[128];

    // Apply font-size only to labels inside check buttons and other widgets
    snprintf(css, sizeof(css),
        "GtkButton, GtkEntry, GtkLabel, GtkCheckButton label { font-size: %dpx; }", 
        size
    );

    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, css, -1, NULL);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);

    g_object_unref(provider);
}

void on_window_resize(GtkWidget *widget, GdkRectangle *allocation, gpointer data) {
    CheckBoxData *cb_data = (CheckBoxData *)data;

    int new_size = allocation->width / 50; // Adjust based on window width
    if (new_size < 6) new_size = 6; // Prevent too small text
    if (new_size > 18) new_size = 18; // Prevent too large text

    // Scale entry fields
    scale_text(GTK_WIDGET(cb_data->entries[0]), new_size);
    scale_text(GTK_WIDGET(cb_data->entries[1]), new_size);
    scale_text(GTK_WIDGET(cb_data->entries[2]), new_size);

    // Scale buttons
    scale_text(cb_data->save_button, new_size);
    scale_text(cb_data->submit_button, new_size);

    // Scale checkboxes
	for (int i = 0; i < TOTAL_CHECKBOXES; i++) {
		GtkWidget *label = gtk_bin_get_child(GTK_BIN(cb_data->checkboxes[i])); // Get label inside checkbox
		if (label) {
			scale_text(label, new_size); // Apply scaling to the label inside the checkbox
		}
	}
}

// Function to handle dropped file paths
static void on_drag_data_received(GtkWidget *widget, GdkDragContext *context,
                                  gint x, gint y, GtkSelectionData *selection_data,
                                  guint info, guint time, gpointer user_data) {
    if (gtk_selection_data_get_length(selection_data) >= 0) {
        gchar **uris = gtk_selection_data_get_uris(selection_data);
        if (uris != NULL && uris[0] != NULL) {
            gchar *filename = g_filename_from_uri(uris[0], NULL, NULL); // Convert URI to filepath
            if (filename) {
                gtk_entry_set_text(GTK_ENTRY(widget), filename); // Set text entry
                g_free(filename);
            }
        }
        g_strfreev(uris);
    }
    gtk_drag_finish(context, TRUE, FALSE, time);
}

// Function to enable drag-and-drop for a text entry
void enable_drag_drop(GtkWidget *entry) {
    gtk_drag_dest_set(entry, GTK_DEST_DEFAULT_ALL, NULL, 0, GDK_ACTION_COPY);
    GtkTargetEntry targets[] = {{"text/uri-list", 0, 0}};
    gtk_drag_dest_set_target_list(entry, gtk_target_list_new(targets, 1));
    g_signal_connect(entry, "drag-data-received", G_CALLBACK(on_drag_data_received), NULL);
}

// Function to load presets from a JSON file
void load_preset(CheckBoxData *cb_data) {
    FILE *file = fopen(PRESET_FILE, "r");
    if (!file) {
        // Show an error dialog if the file doesn't exist or can't be opened
        GtkWidget *error_dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Error: Could not load preset file.");
        gtk_dialog_run(GTK_DIALOG(error_dialog));
        gtk_widget_destroy(error_dialog);
        return; // No preset file, return early
    }

    struct json_object *root, *json_value;
    char buffer[1024];
    fread(buffer, sizeof(buffer), 1, file);
    fclose(file);

    root = json_tokener_parse(buffer);
    if (!root) {
        // Show an error dialog if JSON parsing fails
        GtkWidget *error_dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Error: Failed to parse preset file.");
        gtk_dialog_run(GTK_DIALOG(error_dialog));
        gtk_widget_destroy(error_dialog);
        return; // Failed to parse JSON, return early
    }

    // Load entry fields
    json_value = json_object_object_get(root, "iso_path");
    if (json_value) {
        gtk_entry_set_text(cb_data->entries[0], json_object_get_string(json_value));
    }

    // Seed handling is excluded from preset loading
    // json_value = json_object_object_get(root, "seed");
    // if (json_value) {
    //     gtk_entry_set_text(cb_data->entries[1], json_object_get_string(json_value));
    // }

    json_value = json_object_object_get(root, "spoiler_log");
    if (json_value) {
        gtk_entry_set_text(cb_data->entries[2], json_object_get_string(json_value));
    }

    // Load checkbox states
    json_value = json_object_object_get(root, "checkboxes");
    if (json_value) {
        for (int i = 0; i < 50; i++) {
            struct json_object *val = json_object_array_get_idx(json_value, i);
            if (val && GTK_IS_TOGGLE_BUTTON(cb_data->checkboxes[i])) {
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb_data->checkboxes[i]), json_object_get_boolean(val));
            }
        }
    }

    json_object_put(root); // Free memory

    // Show a success dialog informing the user that the preset was loaded
    GtkWidget *info_dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "Preset loaded successfully!");
    gtk_dialog_run(GTK_DIALOG(info_dialog));
    gtk_widget_destroy(info_dialog);
}

// Function to save the current settings to a JSON file
void save_preset(GtkWidget *widget, CheckBoxData *cb_data) {
    struct json_object *root = json_object_new_object();
    struct json_object *checkbox_array = json_object_new_array();

    // Save entry fields
    json_object_object_add(root, "iso_path", json_object_new_string(gtk_entry_get_text(cb_data->entries[0])));
    json_object_object_add(root, "seed", json_object_new_string(gtk_entry_get_text(cb_data->entries[1])));
    json_object_object_add(root, "spoiler_log", json_object_new_string(gtk_entry_get_text(cb_data->entries[2])));

    // Save checkbox states
    for (int i = 0; i < 50; i++) {
        if (GTK_IS_TOGGLE_BUTTON(cb_data->checkboxes[i])) {
            json_object_array_add(checkbox_array, json_object_new_boolean(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb_data->checkboxes[i]))));
        }
    }
    json_object_object_add(root, "checkboxes", checkbox_array);

    // Write to file
    FILE *file = fopen(PRESET_FILE, "w");
    if (!file) {
        // Show an error message if the file can't be opened
        GtkWidget *error_dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Error: Could not save preset file.");
        gtk_dialog_run(GTK_DIALOG(error_dialog));
        gtk_widget_destroy(error_dialog);
        json_object_put(root); // Free memory
        return;
    }

    // Save the data into the file
    fprintf(file, "%s", json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY));
    fclose(file);

    json_object_put(root); // Free memory

    // Inform the user that the preset was saved
    GtkWidget *info_dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "Preset saved successfully!");
    gtk_dialog_run(GTK_DIALOG(info_dialog));
    gtk_widget_destroy(info_dialog);
}

void random_seed(GtkWidget *widget, CheckBoxData *cb_data) {
     // Initialize random number generator with more entropy
      // Combine time and process ID for more randomness
    int random_number = rand() ^ (getpid() << 16);  // Generate a random number

    // If your platform supports better random functions, you can use them here
    // For example: random() or /dev/urandom could be better on some systems

    // Convert the random number to a string
    char seed_str[20];  // Assuming the number won't exceed 20 digits
    snprintf(seed_str, sizeof(seed_str), "%d", random_number);  // Convert to string

    // Set the seed entry field with the generated random seed
    gtk_entry_set_text(cb_data->entries[1], seed_str);  // Assuming cb_data->entries[1] is the seed entry
}

void give_information_about_everything(GtkWidget *parent)
{
    GtkWidget *dialog;

    // Display warning message
    dialog = gtk_message_dialog_new(GTK_WINDOW(parent),
                                    GTK_DIALOG_MODAL,
                                    GTK_MESSAGE_WARNING,
                                    GTK_BUTTONS_OK,
                                    "WARNING: This will overwrite the provided .iso file.\n"
                                    "The .iso FilePath cannot have any spaces in it. \n"
									"A log file does not need to be provided.\n"
                                    "(It is HIGHLY RECOMMENDED to have one)\n"
                                    "The log FilePath provided needs to be a .txt file AND the FilePath can't have spaces.\n"
									"Everything is Case Sensitive.\n"
									"NOTE: Many of the locked doors in the Prelude to the Dark Abyss have been unlocked.\n"
                                    "(This is despite them still looking locked.)\n"
									"NOTE: You must watch the Cutscene of the Seal on the Pagoda of the Misty Moon door breaking\n"
                                    "to be able to get to the final bosses.\n"
                                    "(That does NOT mean you need to get all the orbs to enter the Pagoda.)\n"
									"WARNING: If you leave the Golem boss fight after placing the E tablet,\n"
                                    "you will NOT be able to fight him again.");
	gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

}

static void on_show_more_toggled(GtkToggleButton *toggle_button, gpointer data) {
    CheckBoxData *cb_data = (CheckBoxData *)data;

    // Show or hide checkboxes 25 to 40 based on the toggle state
    gboolean active = gtk_toggle_button_get_active(toggle_button);
    for (int i = 24; i < 40; i++) {
        if (active) {
            gtk_widget_show(cb_data->checkboxes[i]);
        } else {
            gtk_widget_hide(cb_data->checkboxes[i]);
        }
    }
}

char *itemNames[] = {"NONE","Whip of Alchemy","Whip of Flames","Whip of Ice","Whip of Lightning","Vampire Killer","Earth Plate","Meteor Plate","Moonlight Plate",
"Solar Plate","Draupnir","Aroma Earring","Qigong Belt","Coin of Happiness","Raccoon Charm","Bloody Cape","Perseus Ring","Anti-Poison Ring","Cleric's Ring",
"Ring of Fire","Artic Ring","Ring of Thunder","Talisman","Heart Brooch","Mobius's Brooch","Piyo-piyo Shoes","Sacrificial Doll","Magnetic Necklace","Assassin Necklace",
"Jewel Crush","Megingjord","MemberPlate","Brisingamen","Piko-piko Hammer","Jade Mask","UNUSED","UNUSED","UNUSED","UNUSED","UNUSED","UNUSED","UNUSED","Potion",
"High Potion","Super Potion","Heart Repair","Mana Prism","Serum","Uncurse Potion","Magical Ticket","Memorial Ticket","(UNUSED) Marker Stone 1","(UNUSED) Marker Stone 2","(UNUSED) Marker Stone 3","(UNUSED) Marker Stone 4","(UNUSED) Marker Stone 5","(UNUSED) Marker Stone 6"
,"(UNUSED) Marker Stone 7","(UNUSED) Marker Stone 8","(UNUSED) Rosario","Diamond","Ruby","Sapphire","Opal","Turquoise","Zircon","Curtain Time Bell","Small Meat","Big Meat","Ramen","Wine","Hamburger","Shortcake",
"Sushi","Curry","Tomato Juice","Neapolitan","UNUSED","UNUSED","UNUSED","UNUSED","UNUSED","UNUSED","UNUSED","UNUSED","e Tablet", "VI Tablet", "IV Tablet","Unlock Jewel",
"White Tiger Key","Blue Dragon Key","Red Phoenix Key","Black Turtle Key","Yellow Dragon Key","Dragon Crest","Tool Bag","Music Box","Ancient Text 1","Ancient Text 2",
"Map 1","Map 2","Map 3","Map 4","Map 5","Map 6","Map 7","Marker Stone 1","Marker Stone 2","Marker Stone 3","Marker Stone 4","Marker Stone 5","Marker Stone 6","Marker Stone 7",
"Marker Stone 8","Ancient Text 3","Ancient Text 4","UNUSED","UNUSED","UNUSED","UNUSED","UNUSED","UNUSED","UNUSED","UNUSED","UNUSED","UNUSED","UNUSED","Red Orb","Blue Orb",
"Yellow Orb","Green Orb","Purple Orb","White Orb","Black Orb","Wolf's Foot","Saisei Incense","Meditative Incense","Invincible Jar","Crystal Skull","Black Bishop","White Bishop",
"Lucifer's Sword","Svarog Statue","Little Hammer","HP Max Up","Heart Max Up","MP Max Up","small heart","big heart","$100","$25","$250","$400","$1000","((UNUSED)) Marker Stone 1","((UNUSED)) Marker Stone 2","((UNUSED)) Marker Stone 3"
,"((UNUSED)) Marker Stone 4","((UNUSED)) Marker Stone 5","((UNUSED)) Marker Stone 6","((UNUSED)) Marker Stone 7","((UNUSED)) Marker Stone 8","$1","$5","$10","Rosario","Bloody Skull","UNUSED","UNUSED","UNUSED","Knife","Axe","Holy Water","Crystal","Cross","Pumpkin"
};

int TODO(FILE* fp)
{
/*TODO:
	
	Custom_item_list rando (REDO)
	
	QoL_no_repeat_locking_rooms
	
	More torches on AreaLocking.
	
	fix ordering of checkboxes
	
	flip left vs right paths per areas...
	
	more hints?
	
	*/
	return 1;
}

void setup_trap_items_or_fake_items(FILE* fp)
{
	
	int unused_acc_addresses[] = { // 7
		0x3DA86C, 0x3DA8F0, 0x3DA974, 0x3DA9F8, 0x3DAA7C, 0x3DAB00, 0x3DAB84
	};
	int unused_item_addresses[] = { // 8
		0x3DB6F8, 0x3DB748, 0x3DB798, 0x3DB7E8, 0x3DB838, 0x3DB888, 0x3DB8D8, 0x3DB928
	};
	int unused_pickup_addresses[] = { // 3
		0x3DD088, 0x3DD0D0, 0x3DD118
	};
	int unused_event_item_addresses[] = { // 11
			
	};
	
	char* FAKE_TEXT = "FAKE";
	
	//ACC
	unsigned char array_of_bytes_HEART_BROOCH[] = {
		0xA1, 0x02, 0x00, 0x00, /* Name */
		0x4D, 0x02, 0x00, 0x00, 
		0xB8, 0x01, 0x00, 0x00, 
		0x16, 0x01, 0x00, 0x00, 
		0xFD, 0x01, 0x00, 0x00, 
		0xFD, 0x01, 0x00, 0x00, 
		0xB8, 0x01, 0x00, 0x00, /* RTW Desc. */
		0x16, 0x01, 0x00, 0x00, /* Menu Desc. */
		0x16, 0x00, 0x00, 0x00, /* 2D sprite */
		0x15, 0x00, 0x00, 0x00, /* 3D model */
		0x00, 0x00, 0x00, 0x00, /* 3D model color */
		0x7D, 0x4A, 0x00, 0x00, 
		0x00, 0x00, 0x8C, 0x42, /* 3D model menu offset */
		0x09, 0x00, 0x00, 0x00, /* How many Rinaldo would sell */
		0xFF, 0xFF, 0xFF, 0xFF, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, /* Buy Price */
		0x00, 0x00, 0x00, 0x00, /* Sell Price */
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00  
	};
	int sizeofX = *(&array_of_bytes_HEART_BROOCH + 1) - array_of_bytes_HEART_BROOCH;
	unsigned char array_of_bytes_MOBIUS_BROOCH[] = {
		0xA1, 0x02, 0x00, 0x00, /* Name */
		0x4E, 0x02, 0x00, 0x00, 
		0xB9, 0x01, 0x00, 0x00, 
		0x17, 0x01, 0x00, 0x00, 
		0xFD, 0x01, 0x00, 0x00, 
		0xFD, 0x01, 0x00, 0x00, 
		0xB9, 0x01, 0x00, 0x00, 
		0x17, 0x01, 0x00, 0x00, 
		0x17, 0x00, 0x00, 0x00, 
		0x15, 0x00, 0x00, 0x00, 
		0x01, 0x00, 0x00, 0x00, 
		0x7D, 0x4A, 0x00, 0x00, 
		0x00, 0x00, 0x8C, 0x42, 
		0x01, 0x00, 0x00, 0x00, 
		0xFF, 0xFF, 0xFF, 0xFF, 
		0x00, 0x00, 0x00, 0x00, 
		0x40, 0x0D, 0x03, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00  
	};
	unsigned char array_of_bytes_TALISMAN[] = {
		0xA1, 0x02, 0x00, 0x00, /* Name */
		0x4C, 0x02, 0x00, 0x00, 
		0xB7, 0x01, 0x00, 0x00, 
		0x15, 0x01, 0x00, 0x00, 
		0xFD, 0x01, 0x00, 0x00, 
		0xFD, 0x01, 0x00, 0x00, 
		0xB7, 0x01, 0x00, 0x00, 
		0x15, 0x01, 0x00, 0x00, 
		0x15, 0x00, 0x00, 0x00, 
		0x18, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x6A, 0x4A, 0x00, 0x00, 
		0x00, 0x00, 0x8C, 0x42, 
		0x09, 0x00, 0x00, 0x00, 
		0xFF, 0xFF, 0xFF, 0xFF, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x20, 0x4E, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00  
	};
	unsigned char array_of_bytes_AROMA_EARRING[] = {
		0xA1, 0x02, 0x00, 0x00, /* Name */
		0x41, 0x02, 0x00, 0x00, 
		0xAC, 0x01, 0x00, 0x00, 
		0x0A, 0x01, 0x00, 0x00, 
		0xFD, 0x01, 0x00, 0x00, 
		0xFD, 0x01, 0x00, 0x00, 
		0xAC, 0x01, 0x00, 0x00, 
		0x0A, 0x01, 0x00, 0x00, 
		0x0A, 0x00, 0x00, 0x00, 
		0x48, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x7D, 0x4A, 0x00, 0x00, 
		0x00, 0x00, 0x8C, 0x42, 
		0x09, 0x00, 0x00, 0x00, 
		0xFF, 0xFF, 0xFF, 0xFF, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x88, 0x13, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0xA0, 0x40, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00  
	};
	unsigned char array_of_bytes_SACRIFICIAL_DOLL[] = {
		0xA1, 0x02, 0x00, 0x00, /* Name */
		0x4F, 0x02, 0x00, 0x00, 
		0xBA, 0x01, 0x00, 0x00, 
		0x18, 0x01, 0x00, 0x00, 
		0xFD, 0x01, 0x00, 0x00, 
		0xFD, 0x01, 0x00, 0x00, 
		0xBA, 0x01, 0x00, 0x00, 
		0x18, 0x01, 0x00, 0x00, 
		0x19, 0x00, 0x00, 0x00, 
		0x24, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x7D, 0x4A, 0x00, 0x00, 
		0x00, 0x00, 0x8C, 0x42, 
		0x01, 0x00, 0x00, 0x00, 
		0xFF, 0xFF, 0xFF, 0xFF, 
		0x00, 0x00, 0x00, 0x00, 
		0xD0, 0x07, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00  
	};
	unsigned char array_of_bytes_DRAUPNIR[] = {
		0xA1, 0x02, 0x00, 0x00, /* Name */
		0x40, 0x02, 0x00, 0x00, 
		0xAB, 0x01, 0x00, 0x00, 
		0x09, 0x01, 0x00, 0x00, 
		0xFD, 0x01, 0x00, 0x00, 
		0xFD, 0x01, 0x00, 0x00, 
		0xAB, 0x01, 0x00, 0x00, 
		0x09, 0x01, 0x00, 0x00, 
		0x09, 0x00, 0x00, 0x00, 
		0x13, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x7D, 0x4A, 0x00, 0x00, 
		0x00, 0x00, 0x8C, 0x42, 
		0x09, 0x00, 0x00, 0x00, 
		0xFF, 0xFF, 0xFF, 0xFF, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0xA0, 0x40, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00
	};
	unsigned char array_of_bytes_ASSASSIN_NECKLACE[] = {
		0xA1, 0x02, 0x00, 0x00, /* Name */
		0x51, 0x02, 0x00, 0x00, 
		0xBC, 0x01, 0x00, 0x00, 
		0x1A, 0x01, 0x00, 0x00, 
		0xFD, 0x01, 0x00, 0x00, 
		0xFD, 0x01, 0x00, 0x00, 
		0xBC, 0x01, 0x00, 0x00, 
		0x1A, 0x01, 0x00, 0x00, 
		0x1B, 0x00, 0x00, 0x00, 
		0x1E, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x6A, 0x4A, 0x00, 0x00, 
		0x00, 0x00, 0x8C, 0x42, 
		0x09, 0x00, 0x00, 0x00, 
		0xFF, 0xFF, 0xFF, 0xFF, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x4C, 0x1D, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00
	};
	unsigned char array_of_bytes_JADE_MASK[] = {
		0xA1, 0x02, 0x00, 0x00, /* Name */
		0x58, 0x02, 0x00, 0x00, 
		0xC3, 0x01, 0x00, 0x00, 
		0x21, 0x01, 0x00, 0x00, 
		0xFD, 0x01, 0x00, 0x00, 
		0xFD, 0x01, 0x00, 0x00, 
		0xC3, 0x01, 0x00, 0x00, 
		0x21, 0x01, 0x00, 0x00, 
		0x67, 0x00, 0x00, 0x00, 
		0x09, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x7D, 0x4A, 0x00, 0x00, 
		0x00, 0x00, 0x8C, 0x42, 
		0x01, 0x00, 0x00, 0x00, 
		0xFF, 0xFF, 0xFF, 0xFF, 
		0x00, 0x00, 0x00, 0x00, 
		0xE7, 0x03, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00
	};

	
	//ITEMS
	unsigned char array_of_bytes_POTION[] = {
		0xA1, 0x02, 0x00, 0x00, /* Name */
		0xAA, 0x02, 0x00, 0x00, 
		0x13, 0x02, 0x00, 0x00, 
		0x70, 0x01, 0x00, 0x00, 
		0xFD, 0x01, 0x00, 0x00, 
		0xFD, 0x01, 0x00, 0x00, 
		0x13, 0x02, 0x00, 0x00, /* RTW Desc. */
		0x70, 0x01, 0x00, 0x00, /* Menu Desc. */
		0x21, 0x00, 0x00, 0x00, /* 2D sprite */
		0x3E, 0x00, 0x00, 0x00, /* 3D model */
		0x00, 0x00, 0x00, 0x00, /* 3D model color */
		0x6A, 0x4A, 0x00, 0x00, 
		0x00, 0x00, 0x8C, 0x42, /* 3D model menu offset */
		0x09, 0x00, 0x00, 0x00, /* How many Rinaldo would sell */
		0xFF, 0xFF, 0xFF, 0xFF, 
		0xFF, 0x00, 0x00, 0x00, /* Item Type */
		0xC8, 0x00, 0x00, 0x00, 
		0x00, 0x00, /* Buy Price */ 0x00, 0x00, /* Sell Price */
		0x00, 0x00, 0x00, 0x00, /* Healing Amount */
		0x00, 0x00, 0x00, 0x3F  /* Time frame */
	};
	int sizeofZ = *(&array_of_bytes_POTION + 1) - array_of_bytes_POTION;
	unsigned char array_of_bytes_MAGICAL_TICKET[] = {
		0xA1, 0x02, 0x00, 0x00, 
		0xB1, 0x02, 0x00, 0x00, 
		0x1A, 0x02, 0x00, 0x00, 
		0x77, 0x01, 0x00, 0x00, 
		0xFD, 0x01, 0x00, 0x00, 
		0xFD, 0x01, 0x00, 0x00, 
		0x1A, 0x02, 0x00, 0x00, 
		0x77, 0x01, 0x00, 0x00, 
		0x28, 0x00, 0x00, 0x00, 
		0x30, 0x00, 0x00, 0x00, 
		0x01, 0x00, 0x00, 0x00, 
		0x6A, 0x4A, 0x00, 0x00, 
		0x00, 0x00, 0x8C, 0x42, 
		0x09, 0x00, 0x00, 0x00, 
		0xFF, 0xFF, 0xFF, 0xFF, 
		0x01, 0x00, 0x00, 0x00, 
		0xC8, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x3F
	};
	unsigned char array_of_bytes_MEMORIAL_TICKET[] = {
		0xA2, 0x02, 0x00, 0x00, 
		0xB2, 0x02, 0x00, 0x00, 
		0x1B, 0x02, 0x00, 0x00, 
		0x78, 0x01, 0x00, 0x00, 
		0xFD, 0x01, 0x00, 0x00, 
		0xFD, 0x01, 0x00, 0x00, 
		0x1B, 0x02, 0x00, 0x00, 
		0x78, 0x01, 0x00, 0x00, 
		0x29, 0x00, 0x00, 0x00, 
		0x30, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x6A, 0x4A, 0x00, 0x00, 
		0x00, 0x00, 0x8C, 0x42, 
		0x09, 0x00, 0x00, 0x00, 
		0xFF, 0xFF, 0xFF, 0xFF, 
		0x01, 0x00, 0x00, 0x00, 
		0x20, 0x03, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x3F
	};
	unsigned char array_of_bytes_ITEM_ROSARIO[] = {
		0xA1, 0x02, 0x00, 0x00, 
		0xBB, 0x02, 0x00, 0x00, 
		0x24, 0x02, 0x00, 0x00, 
		0x81, 0x01, 0x00, 0x00, 
		0xFD, 0x01, 0x00, 0x00, 
		0xFD, 0x01, 0x00, 0x00, 
		0x24, 0x02, 0x00, 0x00, 
		0x81, 0x01, 0x00, 0x00, 
		0x32, 0x00, 0x00, 0x00, 
		0x02, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x6A, 0x4A, 0x00, 0x00, 
		0x00, 0x00, 0x8C, 0x42, 
		0x09, 0x00, 0x00, 0x00, 
		0xFF, 0xFF, 0xFF, 0xFF, 
		0x01, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x3F
	};
	unsigned char array_of_bytes_DIAMOND[] = {
		0xA1, 0x02, 0x00, 0x00, 
		0xBC, 0x02, 0x00, 0x00, 
		0x25, 0x02, 0x00, 0x00, 
		0x82, 0x01, 0x00, 0x00, 
		0xFD, 0x01, 0x00, 0x00, 
		0xFD, 0x01, 0x00, 0x00, 
		0x9B, 0x01, 0x00, 0x00, 
		0xA5, 0x01, 0x00, 0x00, 
		0x33, 0x00, 0x00, 0x00, 
		0x31, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x6A, 0x4A, 0x00, 0x00, 
		0x00, 0x00, 0x8C, 0x42, 
		0x01, 0x00, 0x00, 0x00, 
		0xFF, 0xFF, 0xFF, 0xFF, 
		0x01, 0x00, 0x00, 0x00, 
		0x10, 0x27, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x3F
	};
	unsigned char array_of_bytes_CURTIAN_TIME_BELL[] = {
		0xA1, 0x02, 0x00, 0x00, 
		0x75, 0x02, 0x00, 0x00, 
		0xE0, 0x01, 0x00, 0x00, 
		0x3E, 0x01, 0x00, 0x00, 
		0xFD, 0x01, 0x00, 0x00, 
		0xFD, 0x01, 0x00, 0x00, 
		0xE0, 0x01, 0x00, 0x00, 
		0x3E, 0x01, 0x00, 0x00, 
		0x47, 0x00, 0x00, 0x00, 
		0x14, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x7D, 0x4A, 0x00, 0x00, 
		0x00, 0x00, 0x8C, 0x42, 
		0x09, 0x00, 0x00, 0x00, 
		0xFF, 0xFF, 0xFF, 0xFF, 
		0x01, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x3F
	};
	unsigned char array_of_bytes_SERUM[] = {
		0xA1, 0x02, 0x00, 0x00, 
		0xAF, 0x02, 0x00, 0x00, 
		0x18, 0x02, 0x00, 0x00, 
		0x75, 0x01, 0x00, 0x00, 
		0xFD, 0x01, 0x00, 0x00, 
		0xFD, 0x01, 0x00, 0x00, 
		0x18, 0x02, 0x00, 0x00, 
		0x75, 0x01, 0x00, 0x00, 
		0x26, 0x00, 0x00, 0x00, 
		0x37, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x6A, 0x4A, 0x00, 0x00, 
		0x00, 0x00, 0x8C, 0x42, 
		0x09, 0x00, 0x00, 0x00, 
		0xFF, 0xFF, 0xFF, 0xFF, 
		0x01, 0x00, 0x00, 0x00, 
		0x64, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x3F
	};
	unsigned char array_of_bytes_NEOPOLITAN[] = {
		0xA1, 0x02, 0x00, 0x00, 
		0xCB, 0x02, 0x00, 0x00, 
		0x34, 0x02, 0x00, 0x00, 
		0x91, 0x01, 0x00, 0x00, 
		0xFD, 0x01, 0x00, 0x00, 
		0xFD, 0x01, 0x00, 0x00, 
		0x34, 0x02, 0x00, 0x00, 
		0x91, 0x01, 0x00, 0x00, 
		0x42, 0x00, 0x00, 0x00, 
		0x3D, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x6A, 0x4A, 0x00, 0x00, 
		0x00, 0x00, 0x8C, 0x42, 
		0x09, 0x00, 0x00, 0x00, 
		0xFF, 0xFF, 0xFF, 0xFF, 
		0x01, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x4B, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x3F
	};

	//pick-ups
	unsigned char array_of_bytes_HP_MAX_UP[] = {
		0x64, 0x02, 0x00, 0x00, 
		0x64, 0x02, 0x00, 0x00, 
		0xCF, 0x01, 0x00, 0x00, 
		0x2D, 0x01, 0x00, 0x00, 
		0xFD, 0x01, 0x00, 0x00, 
		0x2D, 0x01, 0x00, 0x00, 
		0xCF, 0x01, 0x00, 0x00, 
		0x2D, 0x01, 0x00, 0x00, 
		0xFF, 0x00, 0x00, 0x00, 
		0x4A, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x7D, 0x4A, 0x00, 0x00, 
		0x00, 0x00, 0xF0, 0x42, 
		0x00, 0x00, 0x00, 0x00, 
		0xFF, 0xFF, 0xFF, 0xFF, 
		0x00, 0x00, 0x20, 0x41, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00
	};
	unsigned char array_of_bytes_ROSARIO[] = {
		0xA1, 0x02, 0x00, 0x00, 
		0xBB, 0x02, 0x00, 0x00, 
		0x24, 0x02, 0x00, 0x00, 
		0x81, 0x01, 0x00, 0x00, 
		0xFD, 0x01, 0x00, 0x00, 
		0x81, 0x01, 0x00, 0x00, 
		0x24, 0x02, 0x00, 0x00, 
		0x81, 0x01, 0x00, 0x00, 
		0x32, 0x00, 0x00, 0x00, 
		0x02, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x6A, 0x4A, 0x00, 0x00, 
		0x00, 0x00, 0x8C, 0x42, 
		0x00, 0x00, 0x00, 0x00, 
		0xFF, 0xFF, 0xFF, 0xFF, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00
	};
	unsigned char array_of_bytes_BLOODY_SKULL[] = {
		0xA1, 0x02, 0x00, 0x00, 
		0x71, 0x02, 0x00, 0x00, 
		0x24, 0x02, 0x00, 0x00, 
		0x81, 0x01, 0x00, 0x00, 
		0xFD, 0x01, 0x00, 0x00, 
		0x81, 0x01, 0x00, 0x00, 
		0x24, 0x02, 0x00, 0x00, 
		0x81, 0x01, 0x00, 0x00, 
		0x5F, 0x00, 0x00, 0x00, 
		0x53, 0x00, 0x00, 0x00, 
		0x01, 0x00, 0x00, 0x00, 
		0x6A, 0x4A, 0x00, 0x00, 
		0x00, 0x00, 0x8C, 0x42, 
		0x00, 0x00, 0x00, 0x00, 
		0xFF, 0xFF, 0xFF, 0xFF, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00
	};
	int sizeofY = *(&array_of_bytes_BLOODY_SKULL + 1) - array_of_bytes_BLOODY_SKULL;
	
	//ACC
	for(int i = 0; i < sizeofX; i++)
	{
		fseek(fp,unused_acc_addresses[0]+i,SEEK_SET);
		fwrite(&array_of_bytes_AROMA_EARRING[i],sizeof(array_of_bytes_AROMA_EARRING[i]),1,fp);
	}
	for(int i = 0; i < sizeofX; i++)
	{
		fseek(fp,unused_acc_addresses[1]+i,SEEK_SET);
		fwrite(&array_of_bytes_HEART_BROOCH[i],sizeof(array_of_bytes_HEART_BROOCH[i]),1,fp);
	}
	for(int i = 0; i < sizeofX; i++)
	{
		fseek(fp,unused_acc_addresses[2]+i,SEEK_SET);
		fwrite(&array_of_bytes_MOBIUS_BROOCH[i],sizeof(array_of_bytes_MOBIUS_BROOCH[i]),1,fp);
	}
	for(int i = 0; i < sizeofX; i++)
	{
		fseek(fp,unused_acc_addresses[3]+i,SEEK_SET);
		fwrite(&array_of_bytes_DRAUPNIR[i],sizeof(array_of_bytes_DRAUPNIR[i]),1,fp);
	}
	for(int i = 0; i < sizeofX; i++)
	{
		fseek(fp,unused_acc_addresses[4]+i,SEEK_SET);
		fwrite(&array_of_bytes_TALISMAN[i],sizeof(array_of_bytes_TALISMAN[i]),1,fp);
	}
	for(int i = 0; i < sizeofX; i++)
	{
		fseek(fp,unused_acc_addresses[5]+i,SEEK_SET);
		fwrite(&array_of_bytes_SACRIFICIAL_DOLL[i],sizeof(array_of_bytes_SACRIFICIAL_DOLL[i]),1,fp);
	}
	for(int i = 0; i < sizeofX; i++)
	{
		fseek(fp,unused_acc_addresses[6]+i,SEEK_SET);
		fwrite(&array_of_bytes_JADE_MASK[i],sizeof(array_of_bytes_JADE_MASK[i]),1,fp);
	}
	
	//ITEMS
	for(int i = 0; i < sizeofZ; i++)
	{
		fseek(fp,unused_item_addresses[0]+i,SEEK_SET);
		fwrite(&array_of_bytes_POTION[i],sizeof(array_of_bytes_POTION[i]),1,fp);
	}
	for(int i = 0; i < sizeofZ; i++)
	{
		fseek(fp,unused_item_addresses[1]+i,SEEK_SET);
		fwrite(&array_of_bytes_CURTIAN_TIME_BELL[i],sizeof(array_of_bytes_CURTIAN_TIME_BELL[i]),1,fp);
	}
	for(int i = 0; i < sizeofZ; i++)
	{
		fseek(fp,unused_item_addresses[2]+i,SEEK_SET);
		fwrite(&array_of_bytes_ITEM_ROSARIO[i],sizeof(array_of_bytes_ITEM_ROSARIO[i]),1,fp);
	}
	for(int i = 0; i < sizeofZ; i++)
	{
		fseek(fp,unused_item_addresses[3]+i,SEEK_SET);
		fwrite(&array_of_bytes_DIAMOND[i],sizeof(array_of_bytes_DIAMOND[i]),1,fp);
	}
	for(int i = 0; i < sizeofZ; i++)
	{
		fseek(fp,unused_item_addresses[4]+i,SEEK_SET);
		fwrite(&array_of_bytes_MAGICAL_TICKET[i],sizeof(array_of_bytes_MAGICAL_TICKET[i]),1,fp);
	}
	for(int i = 0; i < sizeofZ; i++)
	{
		fseek(fp,unused_item_addresses[5]+i,SEEK_SET);
		fwrite(&array_of_bytes_MEMORIAL_TICKET[i],sizeof(array_of_bytes_MEMORIAL_TICKET[i]),1,fp);
	}
	for(int i = 0; i < sizeofZ; i++)
	{
		fseek(fp,unused_item_addresses[6]+i,SEEK_SET);
		fwrite(&array_of_bytes_SERUM[i],sizeof(array_of_bytes_SERUM[i]),1,fp);
	}
	for(int i = 0; i < sizeofZ; i++)
	{
		fseek(fp,unused_item_addresses[7]+i,SEEK_SET);
		fwrite(&array_of_bytes_NEOPOLITAN[i],sizeof(array_of_bytes_NEOPOLITAN[i]),1,fp);
	}
	
	//pick-ups
	for(int i = 0; i < sizeofY; i++)
	{
		fseek(fp,unused_pickup_addresses[0]+i,SEEK_SET);
		fwrite(&array_of_bytes_BLOODY_SKULL[i],sizeof(array_of_bytes_BLOODY_SKULL[i]),1,fp);
	}
	for(int i = 0; i < sizeofY; i++)
	{
		fseek(fp,unused_pickup_addresses[1]+i,SEEK_SET);
		fwrite(&array_of_bytes_ROSARIO[i],sizeof(array_of_bytes_ROSARIO[i]),1,fp);
	}
	for(int i = 0; i < sizeofY; i++)
	{
		fseek(fp,unused_pickup_addresses[2]+i,SEEK_SET);
		fwrite(&array_of_bytes_BLOODY_SKULL[i],sizeof(array_of_bytes_BLOODY_SKULL[i]),1,fp);
	}
}

//struct
struct enemy
{
	int number;
	char* name;
	int name_id_address;
	int name_id;
	float HP;
	int HP_address;
	int common_item;
	int common_item_address;
	float common_item_rate;
	int common_item_rate_address;
	int rare_item;
	int rare_item_address;
	float rare_item_rate;
	int rare_item_rate_address;
	float rosario_rate;
	int rosario_address;
	int fire_tolerance_weakness;
	int fire_tolerance_weakness_address;
	int ice_tolerance_weakness;
	int ice_tolerance_weakness_address;
	int thunder_tolerance_weakness;
	int thunder_tolerance_weakness_address;
	int knife_tolerance_weakness;
	int knife_tolerance_weakness_address;
	int axe_tolerance_weakness;
	int axe_tolerance_weakness_address;
	int water_tolerance_weakness;
	int water_tolerance_weakness_address;
	int crystal_tolerance_weakness;
	int crystal_tolerance_weakness_address;
	int cross_tolerance_weakness;
	int cross_tolerance_weakness_address;
};

unsigned char newByte;
unsigned char new_byte; 
int randVal;
int randValue;


/* SETUP */
void setup_loadzone_changes(FILE* fp)
{
	// change right door in Prelude to the Dark Abyss to go to warp room (one-way)
	int prelude_warp = 0x1F9EF1F2;
	fseek(fp,prelude_warp,SEEK_SET);
	new_byte = 0x03;
	fwrite(&new_byte,sizeof(new_byte),1,fp);
	fseek(fp,prelude_warp+4,SEEK_SET);
	new_byte = 0x00;
	fwrite(&new_byte,sizeof(new_byte),1,fp);
	// change door to warp room to not have a combat lock on it.
	int enter_warp_door = 0x1FD08B00;
	fseek(fp,enter_warp_door,SEEK_SET);
	new_byte = 0x03;
	fwrite(&new_byte,sizeof(new_byte),1,fp);
	fseek(fp,enter_warp_door+5,SEEK_SET);
	new_byte = 0x80;
	fwrite(&new_byte,sizeof(new_byte),1,fp);
	// change door exiting Golden Knight fight to not have a combat lock. (Both Directions)
	int golden_knight_door = 0x208A7500;
	new_byte = 0x03;
	fseek(fp,golden_knight_door,SEEK_SET);
	fwrite(&new_byte,sizeof(new_byte),1,fp);
	fseek(fp,golden_knight_door+5,SEEK_SET);
	new_byte = 0x80;
	fwrite(&new_byte,sizeof(new_byte),1,fp);
	int golden_knight_door_exit = 0x1FB82080;
	new_byte = 0x03;
	fseek(fp,golden_knight_door_exit,SEEK_SET);
	fwrite(&new_byte,sizeof(new_byte),1,fp);
	fseek(fp,golden_knight_door_exit+5,SEEK_SET);
	new_byte = 0x80;
	fwrite(&new_byte,sizeof(new_byte),1,fp);
	// change door to Pagoda of the Misty Moon to not use seal check.
	int seal_byte = 0x1F9EF285;
	fseek(fp,seal_byte,SEEK_SET);
	new_byte = 0x80;
	fwrite(&new_byte,sizeof(new_byte),1,fp);
	new_byte = 0x03;
	fseek(fp,seal_byte-5,SEEK_SET);
	fwrite(&new_byte,sizeof(new_byte),1,fp);
	// change door to Pagoda of the Misty Moon to go to sub-weapon hallway (two-way)
	int enter_pagoda_byte = 0x1F9EF272;
	fseek(fp,enter_pagoda_byte,SEEK_SET);
	new_byte = 0x02;
	fwrite(&new_byte,sizeof(new_byte),1,fp);
	int exit_pagoda = 0x1A584770;
	fseek(fp,exit_pagoda,SEEK_SET);
	new_byte = 0x07;
	fwrite(&new_byte,sizeof(new_byte),1,fp);
	fseek(fp,exit_pagoda+2,SEEK_SET);
	new_byte = 0x00;
	fwrite(&new_byte,sizeof(new_byte),1,fp);
	fseek(fp,exit_pagoda+6,SEEK_SET);
	new_byte = 0x01;
	fwrite(&new_byte,sizeof(new_byte),1,fp);
	
	// change door to stairs before Walter's Thrown room to use 5-monster seal check.
	int new_seal = 0x1DB8CD05;
	fseek(fp,new_seal,SEEK_SET);
	new_byte = 0x08;
	fwrite(&new_byte,sizeof(new_byte),1,fp);
		//write text to explain: This means you have to watch the seal being broken in the Prelude to the Dark Abyss
		//printf("ATTENTION: \nyou will not be able to open the door where the seal for the 5 orbs has been moved to unless you watch the cutscene of it breaking in the Prelude to the Dark Abyss! \n");
	
}
void setup_destroy_joachim_mode(FILE* fp)
{
	int joachim_mode_trigger = 0x449AD0;
	fseek(fp,joachim_mode_trigger,SEEK_SET);
	new_byte = 0x00;
	fwrite(&new_byte,sizeof(new_byte),1,fp);
	
	int joachim_text_mode1 = 0x1184D78;
	fseek(fp,joachim_text_mode1,SEEK_SET);
	new_byte = 0x00;
	fwrite(&new_byte,sizeof(new_byte),1,fp);
	int joachim_text_mode2 = 0x118FD78;
	fseek(fp,joachim_text_mode2,SEEK_SET);
	new_byte = 0x00;
	fwrite(&new_byte,sizeof(new_byte),1,fp);
	int joachim_text_mode3 = 0x11B9F40;
	fseek(fp,joachim_text_mode3,SEEK_SET);
	new_byte = 0x00;
	fwrite(&new_byte,sizeof(new_byte),1,fp);
}
void setup_destroy_pumpkin_mode(FILE* fp)
{
	int pumpkin_mode_trigger = 0x449AC0;
	fseek(fp,pumpkin_mode_trigger,SEEK_SET);
	new_byte = 0x00;
	fwrite(&new_byte,sizeof(new_byte),1,fp);
	
	int pumpkin_text_mode1[] = {0x117A694, 0x1184D53, 0x118FD53, 0x119AE94, 0x11B9F1B};
	for(int i = 0; i < 5; i++)
	{
		fseek(fp,pumpkin_text_mode1[i],SEEK_SET);
		new_byte = 0x00;
		fwrite(&new_byte,sizeof(new_byte),1,fp);
	}
	
	
}
void setup_destroy_shop(FILE* fp)
{
	//make every item have a gold_buy_cost of 0
	//make every item have a gold_sell_cost of 0
	static int shopBuyPriceAddress[] = 
	{
		//Armors
		0x003D99B8,
		0x003D9A3C,
		0x003D9AC0,
		0x003D9B44,
		//Relics
		0x003DC780, //Wolf's Foot [0]0
		
		0x003DC7C8, //Saisei Incense
		0x003DC810, //Meditative Incense
		0x003DC858, //Invincible Jar
		0x003DC8A0, //Crystal Skull
		0x003DC8E8, //Black Bishop
		0x003DC930, //White Bishop
		0x003DC978, //Lucifer's Sword
		0x003DC9C0, //Svarog Statue [8]
		0x003DCA08, //Little Hammer [9]
		//Accessories
		0x003D9BCC, //Draupnir [10]
		0x003D9C50, //Aroma Earring
		0x003D9CD4, //Qigong Belt
		0x003D9D58, //Coin of Happiness
		0x003D9DDC, //Raccoon Charm
		0x003D9E60, //Bloody Cape
		0x003D9EE4, //Perseus's Ring
		0x003D9F68, //Anti-poison Ring
		0x003D9FEC, //Cleric's Ring
		0x003DA070, //Ring of Fire
		0x003DA0F4, //Artic Ring
		0x003DA178, //Ring of Thunder
		0x003DA1FC, //Talisman
		0x003DA280, //Heart brooch
		0x003DA304, //Mobius's brooch [24]
		0x003DA388, //Piyo-piyo Shoes
		0x003DA40C, //Sacrificial Doll
		0x003DA490, //Magnetic Necklace [27] //???
		0x003DA514, //Assassin Necklace
		0x003DA598, //Jewel Crush
		0x003DA61C, //Megingjord
		0x003DA6A0, //Member Plate
		0x003DA724, //Brisingamen
		0x003DA7A8, //Piko-piko Hammer
		0x003DA82C, //Jade Mask [34]
		//Usable Items
		0x003DAC4C, //Potion [35]
		0x003DAC9C, //High Potion
		0x003DACEC, //Super Potion
		0x003DAD3C, //Heart Repair
		0x003DAD8C, //Mana Prism
		0x003DADDC, //Serum
		0x003DAE2C, //Uncurse Potion
		0x003DAE7C, //Magical Ticket
		0x003DAECC, //Memorial Ticket [43]
		//0x003DAF1C, //Marker Stone 1
		//0x003DAF6C, //Marker Stone 2
		//0x003DAFBC, //Marker Stone 3
		//0x003DB00C, //Marker Stone 4
		//0x003DB05C, //Marker Stone 5
		//0x003DB0AC, //Marker Stone 6
		//0x003DB0FC, //Marker Stone 7
		//0x003DB14C, //Marker Stone 8
		//0x003DB19C, //Rosario
		0x003DB1EC, //Diamond [44]
		0x003DB23C, //Ruby
		0x003DB28C, //Sapphire
		0x003DB2DC, //Opal
		0x003DB32C, //Turquoise
		0x003DB37C, //Zircon [49]
		0x003DB3CC, //Curtain Time Bell [50]
		0x003DB41C, //Small Meat [51]
		0x003DB46C, //Big Meat
		0x003DB4BC, //Ramen
		0x003DB50C, //Wine
		0x003DB55C, //Hamburger
		0x003DB5AC, //Shortcake
		0x003DB5FC, //Sushi
		0x003DB64C, //Curry
		0x003DB69C, //Tomato Juice
		0x003DB6EC, //Neapolitan [60]
		//Event Items
		0x003DB9B8, //"e" Tablet [61]
		0x003DBA00, //"VI" Tablet
		0x003DBA48, //"IV" Tablet
		0x003DBA90, //Unlock Jewel [64]
		//0x003DBAD8, //White Tiger Key
		//0x003DBB20, //Blue Dragon Key
		//0x003DBB68, //Red Phoenix Key
		//0x003DBBB0, //Black Turtle Key
		//0x003DBBF8, //Yellow Dragon Key
		//0x003DBC40, //Dragon Crest
		0x003DBC88, //Tool Bag [65]
		0x003DBCD0, //Music Box
		0x003DBD18, //Ancient Text 1 [66]
		0x003DBD60, //Ancient Text 2
		0x003DBDA8, //Map 1
		0x003DBDF0, //Map 2
		0x003DBE38, //Map 3
		0x003DBE80, //Map 4
		0x003DBEC8, //Map 5 [72]
		//0x003DBF10, //Map 6
		//0x003DBF58, //Map 7
		0x003DBFA0, //Marker Stone 1 [73]
		0x003DBFE8, //Marker Stone 2
		0x003DC030, //Marker Stone 3
		0x003DC078, //Marker Stone 4
		0x003DC0C0, //Marker Stone 5
		0x003DC108, //Marker Stone 6
		0x003DC150, //Marker Stone 7
		0x003DC198, //Marker Stone 8
		0x003DC1E0, //Ancient Text 3
		0x003DC228 //Ancient Text 4 [82]
	};
	int shopBuyPriceAddressLength = *(&shopBuyPriceAddress + 1) - shopBuyPriceAddress;
	new_byte = 0x00;
	
	for(int i = 0; i < shopBuyPriceAddressLength; i++)
	{
		for(int j = 0; j < 4; j++)
		{
			fseek(fp,shopBuyPriceAddress[i]+j,SEEK_SET);
			fwrite(&new_byte,sizeof(new_byte),1,fp);
			if(i < 38 || i > 64)
			{
				fseek(fp,shopBuyPriceAddress[i]+4+j,SEEK_SET);
				fwrite(&new_byte,sizeof(new_byte),1,fp);
			}
		}
	}
	/*
	int potion_heals = 0x3DAC50;
	fseek(fp,potion_heals,SEEK_SET);
	new_byte = 0x32;
	fwrite(&new_byte,sizeof(new_byte),1,fp);
	*/
	
}
void setup_change_sprites(FILE* fp)
{
	//for HP_Max_Up.sprite_id, MP_Max_Up.sprite_id, Heart_Max_Up.sprite_id, and money_item.sprite_ids change them to high_potion.sprite_id, mana_prism.sprite_id, Heart_Repair.sprite_id, Coin_of_Happyness.sprite_id
	int moneySprites[] = 
	{
		0x003DCB98,
		0x003DCBE0,
		0x003DCC28,
		0x003DCC70,
		0x003DCCB8,
		0x003DCD00,
		0x003DCD48,
		0x003DCD90,
		0x003DCDD8,
		0x003DCE20,
		0x003DCE68,
		0x003DCEB0,
		0x003DCEF8,
		0x003DCF40,
		0x003DCF88,
		0x003DCFD0
	};
	int moneySpritesLength = *(&moneySprites + 1) - moneySprites;
	const int HPMaxSprite = 0x003DCA10 + 0x20; //HP Max Up sprite
	const int HeartMaxSprite = 0x003DCA58 + 0x20; //Heart Max Up sprite
	const int MPMaxSprite = 0x003DCAA0 + 0x20; //MP Max Up sprite
	newByte = 0x22;
	fseek(fp,HPMaxSprite,SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	newByte = 0x24;
	fseek(fp,HeartMaxSprite,SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	newByte = 0x25;
	fseek(fp,MPMaxSprite,SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	
	newByte = 0x0C;
	for(int i = 0; i < moneySpritesLength; i++)
	{
		fseek(fp,moneySprites[i],SEEK_SET);
		fwrite(&newByte,sizeof(newByte),1,fp);
	}
}
struct enemy enemies[] = 
{
	//{#,name,name_id_address,name_id,HP,HP_address,common_item,common_item_address,common_item_rate,common_item_rate_address,rare_item,rare_item_address,rare_item_rate,rare_item_rate_address,rosario_rate,rosario_address,fire tolerance/weakness,fire tolerance/weakness_address ,ice tolerance/weakness,ice tolerance/weakness_address,thunder tolerance/weakness,thunder tolerance/weakness_address,knife tolerance/weakness,knife tolerance/weakness_address,axe tolerance/weakness,axe tolerance/weakness_address,water tolerance/weakness,water tolerance/weakness_address,crystal tolerance/weakness,crystal tolerance/weakness_address,cross tolerance/weakness,cross tolerance/weakness_address},
	{1,"Golden Knight",0x6FCA98,0xA4,60,0x6FCA94,0x00,0x6FCAE8,0,0x6FCAEC,0x00,0x6FCAEA,0,0x6FCAF0,0,0x6FCB14,0,0x6FCAB2,0,0x6FCAB6,0,0x6FCABA,0,0x6FCABE,0,0x6FCAC2,0,0x6FCAC6,0,0x6FCACA,0,0x6FCACE},
	{2,"Bat",0x6E7C98,0x54,1,0x6E7C94,0x00,0x6E7CE8,0,0x6E7CEC,0x00,0x6E7CEA,0,0x6E7CF0,0,0x6E7D14,-1,0x6E7CB2,0,0x6E7CB6,0,0x6E7CBA,-1,0x6E7CBE,-1,0x6E7CC2,0,0x6E7CC6,0,0x6E7CCA,0,0x6E7CCE},
	{3,"Zombie",0x776F98,0x108,50,0x776F94,0x00,0x776FE8,0,0x776FEC,0x00,0x776FEA,0,0x776FF0,0,0x777014,-1,0x776FB2,0,0x776FB6,0,0x776FBA,0,0x776FBE,-1,0x776FC2,-1,0x776FC6,0,0x776FCA,0,0x776FCE},
	{4,"Skeleton",0x762698,0xEC,56,0x762694,0x2A,0x7626E8,0,0x7626EC,0x00,0x7626EA,0,0x7626F0,0,0x762714,-1,0x7626B2,0,0x7626B6,0,0x7626BA,1,0x7626BE,-1,0x7626C2,-1,0x7626C6,0,0x7626CA,0,0x7626CE},
	{5,"Axe Armor",0x6E7A98,0x50,160,0x6E7A94,0x00,0x6E7AE8,0,0x6E7AEC,0x41,0x6E7AEA,0,0x6E7AF0,0,0x6E7B14,0,0x6E7AB2,0,0x6E7AB6,-1,0x6E7ABA,1,0x6E7ABE,-1,0x6E7AC2,0,0x6E7AC6,0,0x6E7ACA,0,0x6E7ACE},
	{6,"Evil Sword",0x6EF818,0x7A,34,0x6EF814,0xFC18,0x6EF868,0,0x6EF86C,0xF830,0x6EF86A,0,0x6EF870,0,0x6EF894,0,0x6EF832,0,0x6EF836,0,0x6EF83A,1,0x6EF83E,0,0x6EF842,0,0x6EF846,-1,0x6EF84A,0,0x6EF84E},
	{7,"Vassago",0x776B98,0x104,66,0x776B94,0x00,0x776BE8,0,0x776BEC,0x00,0x776BEA,0,0x776BF0,0,0x776C14,0,0x776BB2,0,0x776BB6,0,0x776BBA,1,0x776BBE,1,0x776BC2,-1,0x776BC6,0,0x776BCA,-1,0x776BCE},
	{8,"Peeping Eye",0x760D98,0xC2,50,0x760D94,0x00,0x760DE8,0,0x760DEC,0x00,0x760DEA,0,0x760DF0,0,0x760E14,0,0x760DB2,-1,0x760DB6,0,0x760DBA,0,0x760DBE,0,0x760DC2,0,0x760DC6,0,0x760DCA,0,0x760DCE},
	{9,"Mad Diver",0x6E7898,0x4C,120,0x6E7894,0x2D,0x6E78E8,0,0x6E78EC,0x00,0x6E78EA,0,0x6E78F0,0,0x6E7914,-1,0x6E78B2,0,0x6E78B6,0,0x6E78BA,0,0x6E78BE,0,0x6E78C2,-1,0x6E78C6,0,0x6E78CA,-1,0x6E78CE},
	{10,"Spirit",0x6FC898,0xA0,30,0x6FC894,0x00,0x6FC8E8,0,0x6FC8EC,0x00,0x6FC8EA,0,0x6FC8F0,0,0x6FC914,0,0x6FC8B2,0,0x6FC8B6,0,0x6FC8BA,1,0x6FC8BE,1,0x6FC8C2,-1,0x6FC8C6,0,0x6FC8CA,-1,0x6FC8CE},
	{11,"Skeleton Swordman",0x761898,0xD0,80,0x761894,0x2A,0x7618E8,0,0x7618EC,0x00,0x7618EA,0,0x7618F0,0,0x761914,-1,0x7618B2,0,0x7618B6,0,0x7618BA,1,0x7618BE,0,0x7618C2,-1,0x7618C6,0,0x7618CA,-1,0x7618CE},
	{12,"Hellhound",0x6EC798,0x5A,80,0x6EC794,0x00,0x6EC7E8,0,0x6EC7EC,0x00,0x6EC7EA,0,0x6EC7F0,0,0x6EC814,0,0x6EC7B2,-1,0x6EC7B6,0,0x6EC7BA,-1,0x6EC7BE,-1,0x6EC7C2,0,0x6EC7C6,0,0x6EC7CA,0,0x6EC7CE},
	{13,"Ghost Soldier",0x6F0298,0x8C,80,0x6F0294,0x00,0x6F02E8,0,0x6F02EC,0x00,0x6F02EA,0,0x6F02F0,0,0x6F0314,0,0x6F02B2,0,0x6F02B6,0,0x6F02BA,1,0x6F02BE,0,0x6F02C2,-1,0x6F02C6,0,0x6F02CA,-1,0x6F02CE},
	{14,"Flame Zombie",0x6EFD98,0x82,50,0x6EFD94,0x00,0x6EFDE8,0,0x6EFDEC,0x00,0x6EFDEA,0,0x6EFDF0,0,0x6EFE14,1,0x6EFDB2,-1,0x6EFDB6,0,0x6EFDBA,0,0x6EFDBE,0,0x6EFDC2,-1,0x6EFDC6,0,0x6EFDCA,-1,0x6EFDCE},
	{15,"Flame Sword",0x776398,0xF6,80,0x776394,0x00,0x7763E8,0,0x7763EC,0x00,0x7763EA,0,0x7763F0,0,0x776414,1,0x7763B2,-1,0x7763B6,0,0x7763BA,1,0x7763BE,0,0x7763C2,0,0x7763C6,-1,0x7763CA,0,0x7763CE},
	{16,"Flame Demon",0x6EFC98,0x80,200,0x6EFC94,0x2E,0x6EFCE8,0,0x6EFCEC,0x00,0x6EFCEA,0,0x6EFCF0,0,0x6EFD14,1,0x6EFCB2,-1,0x6EFCB6,0,0x6EFCBA,0,0x6EFCBE,0,0x6EFCC2,-1,0x6EFCC6,0,0x6EFCCA,-1,0x6EFCCE},
	{17,"Red Skeleton",0x761F98,0xDE,50,0x761F94,0x00,0x761FE8,0,0x761FEC,0x00,0x761FEA,0,0x761FF0,0,0x762014,-1,0x761FB2,0,0x761FB6,0,0x761FBA,1,0x761FBE,0,0x761FC2,-1,0x761FC6,0,0x761FCA,-1,0x761FCE},
	{18,"Shadow Wolf",0x6EC598,0x56,100,0x6EC594,0x00,0x6EC5E8,0,0x6EC5EC,0x00,0x6EC5EA,0,0x6EC5F0,0,0x6EC614,-1,0x6EC5B2,0,0x6EC5B6,0,0x6EC5BA,1,0x6EC5BE,0,0x6EC5C2,-1,0x6EC5C6,0,0x6EC5CA,-1,0x6EC5CE},
	{19,"Astral Fighter",0x761998,0xD2,80,0x761994,0x2A,0x7619E8,0,0x7619EC,0x00,0x7619EA,0,0x7619F0,0,0x761A14,-1,0x7619B2,0,0x7619B6,0,0x7619BA,1,0x7619BE,0,0x7619C2,-1,0x7619C6,0,0x7619CA,-1,0x7619CE},
	{20,"Ghost Knight",0x6F0498,0x90,80,0x6F0494,0x00,0x6F04E8,0,0x6F04EC,0x00,0x6F04EA,0,0x6F04F0,0,0x6F0514,-1,0x6F04B2,0,0x6F04B6,0,0x6F04BA,1,0x6F04BE,0,0x6F04C2,-1,0x6F04C6,0,0x6F04CA,-1,0x6F04CE},
	{21,"Skeleton Archer",0x761798,0xCE,30,0x761794,0x2A,0x7617E8,0,0x7617EC,0xFC18,0x7617EA,0,0x7617F0,0,0x761814,-1,0x7617B2,0,0x7617B6,0,0x7617BA,1,0x7617BE,0,0x7617C2,-1,0x7617C6,0,0x7617CA,-1,0x7617CE},
	{22,"Flea Man",0x6F0198,0x8A,10,0x6F0194,0x2D,0x6F01E8,0,0x6F01EC,0x4B,0x6F01EA,0,0x6F01F0,0,0x6F0214,0,0x6F01B2,0,0x6F01B6,0,0x6F01BA,-1,0x6F01BE,-1,0x6F01C2,0,0x6F01C6,0,0x6F01CA,0,0x6F01CE},
	{23,"Ghost Warrior",0x6F0398,0x8E,50,0x6F0394,0x00,0x6F03E8,0,0x6F03EC,0x00,0x6F03EA,0,0x6F03F0,0,0x6F0414,-1,0x6F03B2,0,0x6F03B6,0,0x6F03BA,1,0x6F03BE,0,0x6F03C2,-1,0x6F03C6,0,0x6F03CA,-1,0x6F03CE},
	{24,"Gargoyle",0x6F0D18,0x9C,84,0x6F0D14,0x2D,0x6F0D68,0,0x6F0D6C,0x00,0x6F0D6A,0,0x6F0D70,0,0x6F0D94,0,0x6F0D32,0,0x6F0D36,1,0x6F0D3A,1,0x6F0D3E,0,0x6F0D42,-1,0x6F0D46,-1,0x6F0D4A,0,0x6F0D4E},
	{25,"Lizard Man",0x6FD318,0xB2,160,0x6FD314,0x00,0x6FD368,0,0x6FD36C,0x44,0x6FD36A,0,0x6FD370,0,0x6FD394,0,0x6FD332,0,0x6FD336,-1,0x6FD33A,-1,0x6FD33E,-1,0x6FD342,0,0x6FD346,0,0x6FD34A,1,0x6FD34E},
	{26,"Rune Spirit",0x6FC798,0x9E,20,0x6FC794,0x00,0x6FC7E8,0,0x6FC7EC,0x00,0x6FC7EA,0,0x6FC7F0,0,0x6FC814,0,0x6FC7B2,0,0x6FC7B6,0,0x6FC7BA,1,0x6FC7BE,1,0x6FC7C2,-1,0x6FC7C6,0,0x6FC7CA,-1,0x6FC7CE},
	{27,"Astral Knight",0x761B98,0xD6,80,0x761B94,0x2A,0x761BE8,0,0x761BEC,0x00,0x761BEA,0,0x761BF0,0,0x761C14,-1,0x761BB2,0,0x761BB6,0,0x761BBA,1,0x761BBE,0,0x761BC2,-1,0x761BC6,0,0x761BCA,-1,0x761BCE},
	{28,"Armor Knight",0x6FCB98,0xA6,50,0x6FCB94,0x3D,0x6FCBE8,0,0x6FCBEC,0xF448,0x6FCBEA,0,0x6FCBF0,0,0x6FCC14,0,0x6FCBB2,0,0x6FCBB6,-1,0x6FCBBA,1,0x6FCBBE,-1,0x6FCBC2,0,0x6FCBC6,0,0x6FCBCA,0,0x6FCBCE},
	{29,"Skeleton Knight",0x762298,0xE4,100,0x762294,0x2A,0x7622E8,0,0x7622EC,0x00,0x7622EA,0,0x7622F0,0,0x762314,-1,0x7622B2,0,0x7622B6,0,0x7622BA,1,0x7622BE,0,0x7622C2,-1,0x7622C6,0,0x7622CA,-1,0x7622CE},
	{30,"Poison Zombie",0x760F98,0xC6,50,0x760F94,0x2F,0x760FE8,0,0x760FEC,0x00,0x760FEA,0,0x760FF0,0,0x761014,-1,0x760FB2,0,0x760FB6,0,0x760FBA,0,0x760FBE,0,0x760FC2,-1,0x760FC6,0,0x760FCA,-1,0x760FCE},
	{31,"Mist",0x6FD918,0xBC,60,0x6FD914,0x00,0x6FD968,0,0x6FD96C,0x00,0x6FD96A,0,0x6FD970,0,0x6FD994,-1,0x6FD932,0,0x6FD936,0,0x6FD93A,1,0x6FD93E,1,0x6FD942,-1,0x6FD946,-1,0x6FD94A,-1,0x6FD94E},
	{32,"Man-Eating Plant",0x760898,0xBE,200,0x760894,0x47,0x7608E8,0,0x7608EC,0x00,0x7608EA,0,0x7608F0,0,0x760914,-1,0x7608B2,0,0x7608B6,0,0x7608BA,-1,0x7608BE,-1,0x7608C2,0,0x7608C6,0,0x7608CA,0,0x7608CE},
	{33,"Buckbaird",0x760C98,0xC0,66,0x760C94,0x00,0x760CE8,0,0x760CEC,0x00,0x760CEA,0,0x760CF0,0,0x760D14,0,0x760CB2,0,0x760CB6,1,0x760CBA,0,0x760CBE,0,0x760CC2,0,0x760CC6,0,0x760CCA,0,0x760CCE},
	{34,"Skeleton Soldier",0x762098,0xE0,56,0x762094,0x2A,0x7620E8,0,0x7620EC,0x00,0x7620EA,0,0x7620F0,0,0x762114,-1,0x7620B2,0,0x7620B6,0,0x7620BA,1,0x7620BE,0,0x7620C2,-1,0x7620C6,0,0x7620CA,-1,0x7620CE},
	{35,"Poison Lizard",0x6FD418,0xB4,140,0x6FD414,0x2F,0x6FD468,0,0x6FD46C,0x11,0x6FD46A,0,0x6FD470,0,0x6FD494,0,0x6FD432,0,0x6FD436,-1,0x6FD43A,-1,0x6FD43E,-1,0x6FD442,0,0x6FD446,0,0x6FD44A,1,0x6FD44E},
	{36,"Wolf Skeleton",0x6EC698,0x58,80,0x6EC694,0x00,0x6EC6E8,0,0x6EC6EC,0x00,0x6EC6EA,0,0x6EC6F0,0,0x6EC714,-1,0x6EC6B2,0,0x6EC6B6,0,0x6EC6BA,1,0x6EC6BE,0,0x6EC6C2,-1,0x6EC6C6,0,0x6EC6CA,-1,0x6EC6CE},
	{37,"Astral Warrior",0x761A98,0xD4,80,0x761A94,0x2A,0x761AE8,0,0x761AEC,0x00,0x761AEA,0,0x761AF0,0,0x761B14,-1,0x761AB2,0,0x761AB6,0,0x761ABA,1,0x761ABE,0,0x761AC2,-1,0x761AC6,0,0x761ACA,-1,0x761ACE},
	{38,"Fish Man",0x6EFF98,0x86,84,0x6EFF94,0x43,0x6EFFE8,0,0x6EFFEC,0x49,0x6EFFEA,0,0x6EFFF0,0,0x6F0014,1,0x6EFFB2,1,0x6EFFB6,-1,0x6EFFBA,0,0x6EFFBE,-1,0x6EFFC2,-1,0x6EFFC6,0,0x6EFFCA,0,0x6EFFCE},
	{39,"Frost Sword",0x776498,0xF8,80,0x776494,0x00,0x7764E8,0,0x7764EC,0x00,0x7764EA,0,0x7764F0,0,0x776514,-1,0x7764B2,1,0x7764B6,0,0x7764BA,1,0x7764BE,0,0x7764C2,0,0x7764C6,-1,0x7764CA,0,0x7764CE},
	{40,"Frost Demon",0x6F0918,0x94,200,0x6F0914,0x2E,0x6F0968,0,0x6F096C,0x00,0x6F096A,0,0x6F0970,0,0x6F0994,-1,0x6F0932,1,0x6F0936,0,0x6F093A,0,0x6F093E,0,0x6F0942,-1,0x6F0946,0,0x6F094A,-1,0x6F094E},
	{41,"Frost Zombie",0x6F0A18,0x96,50,0x6F0A14,0x00,0x6F0A68,0,0x6F0A6C,0x00,0x6F0A6A,0,0x6F0A70,0,0x6F0A94,-1,0x6F0A32,1,0x6F0A36,0,0x6F0A3A,0,0x6F0A3E,0,0x6F0A42,-1,0x6F0A46,0,0x6F0A4A,-1,0x6F0A4E},
	{42,"Merman",0x6EFE98,0x84,120,0x6EFE94,0x43,0x6EFEE8,0,0x6EFEEC,0x49,0x6EFEEA,0,0x6EFEF0,0,0x6EFF14,1,0x6EFEB2,1,0x6EFEB6,-1,0x6EFEBA,-1,0x6EFEBE,-1,0x6EFEC2,0,0x6EFEC6,0,0x6EFECA,0,0x6EFECE},
	{43,"Ghost",0x776A98,0x102,66,0x776A94,0x00,0x776AE8,0,0x776AEC,0x00,0x776AEA,0,0x776AF0,0,0x776B14,0,0x776AB2,0,0x776AB6,0,0x776ABA,1,0x776ABE,1,0x776AC2,-1,0x776AC6,0,0x776ACA,-1,0x776ACE},
	{44,"Skeleton Flower",0x761C98,0xD8,150,0x761C94,0x2F,0x761CE8,0,0x761CEC,0x00,0x761CEA,0,0x761CF0,0,0x761D14,-1,0x761CB2,0,0x761CB6,0,0x761CBA,0,0x761CBE,0,0x761CC2,-1,0x761CC6,0,0x761CCA,-1,0x761CCE},
	{45,"Heavy Armor",0x6FCC98,0xA8,200,0x6FCC94,0x3E,0x6FCCE8,0,0x6FCCEC,0x00,0x6FCCEA,0,0x6FCCF0,0,0x6FCD14,0,0x6FCCB2,0,0x6FCCB6,-1,0x6FCCBA,1,0x6FCCBE,-1,0x6FCCC2,0,0x6FCCC6,0,0x6FCCCA,0,0x6FCCCE},
	{46,"Skeleton Warrior",0x762198,0xE2,90,0x762194,0x2A,0x7621E8,0,0x7621EC,0x1B,0x7621EA,0,0x7621F0,0,0x762214,-1,0x7621B2,0,0x7621B6,0,0x7621BA,1,0x7621BE,0,0x7621C2,-1,0x7621C6,0,0x7621CA,-1,0x7621CE},
	{47,"Thunder Sword",0x776598,0xFA,80,0x776594,0x00,0x7765E8,0,0x7765EC,0x00,0x7765EA,0,0x7765F0,0,0x776614,0,0x7765B2,0,0x7765B6,1,0x7765BA,-1,0x7765BE,-1,0x7765C2,0,0x7765C6,-1,0x7765CA,0,0x7765CE},
	{48,"Thunder Demon",0x776998,0x100,200,0x776994,0x2E,0x7769E8,0,0x7769EC,0x00,0x7769EA,0,0x7769F0,0,0x776A14,0,0x7769B2,0,0x7769B6,1,0x7769BA,-1,0x7769BE,-1,0x7769C2,-1,0x7769C6,0,0x7769CA,-1,0x7769CE},
	{49,"Storm Skeleton",0x761D98,0xDA,60,0x761D94,0x00,0x761DE8,0,0x761DEC,0x00,0x761DEA,0,0x761DF0,0,0x761E14,-1,0x761DB2,0,0x761DB6,0,0x761DBA,1,0x761DBE,0,0x761DC2,-1,0x761DC6,0,0x761DCA,-1,0x761DCE},
	{50,"Cyclops",0x6ECC98,0x62,400,0x6ECC94,0x40,0x6ECCE8,0,0x6ECCEC,0x21,0x6ECCEA,0,0x6ECCF0,0,0x6ECD14,0,0x6ECCB2,0,0x6ECCB6,0,0x6ECCBA,-1,0x6ECCBE,-1,0x6ECCC2,0,0x6ECCC6,0,0x6ECCCA,0,0x6ECCCE},
	{51,"Chaos Sword",0x776298,0xF4,150,0x776294,0x00,0x7762E8,0,0x7762EC,0x00,0x7762EA,0,0x7762F0,0,0x776314,0,0x7762B2,0,0x7762B6,0,0x7762BA,1,0x7762BE,0,0x7762C2,0,0x7762C6,-1,0x7762CA,0,0x7762CE},
	{52,"Skeleton Hunter",0x761698,0xCC,150,0x761694,0x2A,0x7616E8,0,0x7616EC,0xF448,0x7616EA,0,0x7616F0,0,0x761714,-1,0x7616B2,0,0x7616B6,0,0x7616BA,1,0x7616BE,0,0x7616C2,-1,0x7616C6,0,0x7616CA,-1,0x7616CE},
	{53,"Executioner",0x6ECB98,0x60,350,0x6ECB94,0x30,0x6ECBE8,0,0x6ECBEC,0x12,0x6ECBEA,0,0x6ECBF0,0,0x6ECC14,-1,0x6ECBB2,0,0x6ECBB6,0,0x6ECBBA,0,0x6ECBBE,0,0x6ECBC2,-1,0x6ECBC6,0,0x6ECBCA,-1,0x6ECBCE},
	{54,"Hanged Man",0x6FCD98,0xAA,350,0x6FCD94,0x00,0x6FCDE8,0,0x6FCDEC,0x0D,0x6FCDEA,0,0x6FCDF0,0,0x6FCE14,-1,0x6FCDB2,0,0x6FCDB6,0,0x6FCDBA,0,0x6FCDBE,0,0x6FCDC2,-1,0x6FCDC6,0,0x6FCDCA,-1,0x6FCDCE},
	{55,"Red Ogre",0x6ECA98,0x5E,400,0x6ECA94,0x00,0x6ECAE8,0,0x6ECAEC,0x4A,0x6ECAEA,0,0x6ECAF0,0,0x6ECB14,-1,0x6ECAB2,0,0x6ECAB6,0,0x6ECABA,-1,0x6ECABE,-1,0x6ECAC2,0,0x6ECAC6,0,0x6ECACA,0,0x6ECACE},
	{56,"Spartacus",0x762398,0xE6,250,0x762394,0x2B,0x7623E8,0,0x7623EC,0x1F,0x7623EA,0,0x7623F0,0,0x762414,-1,0x7623B2,0,0x7623B6,0,0x7623BA,1,0x7623BE,0,0x7623C2,-1,0x7623C6,0,0x7623CA,-1,0x7623CE},
	{57,"Lesser Demon",0x6E7B98,0x52,200,0x6E7B94,0x2D,0x6E7BE8,0,0x6E7BEC,0x0B,0x6E7BEA,0,0x6E7BF0,0,0x6E7C14,0,0x6E7BB2,0,0x6E7BB6,0,0x6E7BBA,0,0x6E7BBE,0,0x6E7BC2,-1,0x6E7BC6,0,0x6E7BCA,-1,0x6E7BCE},
	{58,"Evil Stabber",0x6EF718,0x76,200,0x6EF714,0x00,0x6EF768,0,0x6EF76C,0x1C,0x6EF76A,0,0x6EF770,0,0x6EF794,0,0x6EF732,0,0x6EF736,0,0x6EF73A,0,0x6EF73E,0,0x6EF742,-1,0x6EF746,0,0x6EF74A,-1,0x6EF74E},
	{59,"Death Reaper",0x6F0098,0x88,100,0x6F0094,0x2D,0x6F00E8,0,0x6F00EC,0x19,0x6F00EA,0,0x6F00F0,0,0x6F0114,0,0x6F00B2,0,0x6F00B6,0,0x6F00BA,0,0x6F00BE,0,0x6F00C2,0,0x6F00C6,-1,0x6F00CA,0,0x6F00CE},
	{60,"Mirage Skeleton",0x761E98,0xDC,500,0x761E94,0x00,0x761EE8,0,0x761EEC,0x89,0x761EEA,0,0x761EF0,0,0x761F14,-1,0x761EB2,0,0x761EB6,0,0x761EBA,1,0x761EBE,0,0x761EC2,-1,0x761EC6,0,0x761ECA,-1,0x761ECE},
	{61,"Gaap",0x6F0B18,0x98,180,0x6F0B14,0x32,0x6F0B68,0,0x6F0B6C,0x00,0x6F0B6A,0,0x6F0B70,0,0x6F0B94,-1,0x6F0B32,0,0x6F0B36,0,0x6F0B3A,0,0x6F0B3E,0,0x6F0B42,-1,0x6F0B46,1,0x6F0B4A,-1,0x6F0B4E},
	{62,"Lizard Knight",0x6FD518,0xB6,300,0x6FD514,0x00,0x6FD568,0,0x6FD56C,0x16,0x6FD56A,0,0x6FD570,0,0x6FD594,0,0x6FD532,0,0x6FD536,-1,0x6FD53A,-1,0x6FD53E,-1,0x6FD542,0,0x6FD546,0,0x6FD54A,1,0x6FD54E},
	{63,"Axe Knight",0x6E7998,0x4E,250,0x6E7994,0x3F,0x6E79E8,0,0x6E79EC,0x46,0x6E79EA,0,0x6E79F0,0,0x6E7A14,1,0x6E79B2,1,0x6E79B6,-1,0x6E79BA,1,0x6E79BE,-1,0x6E79C2,0,0x6E79C6,0,0x6E79CA,0,0x6E79CE},
	{64,"Phantom",0x760E98,0xC4,400,0x760E94,0x2B,0x760EE8,0,0x760EEC,0x8A,0x760EEA,0,0x760EF0,0,0x760F14,-1,0x760EB2,0,0x760EB6,0,0x760EBA,1,0x760EBE,0,0x760EC2,-1,0x760EC6,0,0x760ECA,-1,0x760ECE},
	{65,"Dullahan",0x6ED598,0x72,250,0x6ED594,0x31,0x6ED5E8,0,0x6ED5EC,0x46,0x6ED5EA,0,0x6ED5F0,0,0x6ED614,1,0x6ED5B2,1,0x6ED5B6,-1,0x6ED5BA,1,0x6ED5BE,-1,0x6ED5C2,0,0x6ED5C6,0,0x6ED5CA,0,0x6ED5CE},
	{66,"Soulless",0x6ECE98,0x68,30,0x6ECE94,0x00,0x6ECEE8,0,0x6ECEEC,0x00,0x6ECEEA,0,0x6ECEF0,0,0x6ECF14,-1,0x6ECEB2,0,0x6ECEB6,0,0x6ECEBA,0,0x6ECEBE,0,0x6ECEC2,-1,0x6ECEC6,0,0x6ECECA,-1,0x6ECECE},
	{67,"Soulless",0x6ECF98,0x66,30,0x6ECF94,0x00,0x6ECFE8,0,0x6ECFEC,0x00,0x6ECFEA,0,0x6ECFF0,0,0x6ED014,-1,0x6ECFB2,0,0x6ECFB6,0,0x6ECFBA,0,0x6ECFBE,0,0x6ECFC2,-1,0x6ECFC6,0,0x6ECFCA,-1,0x6ECFCE},
	{68,"Maggot",0x6FD618,0xB8,45,0x6FD614,0x00,0x6FD668,0,0x6FD66C,0x00,0x6FD66A,0,0x6FD670,0,0x6FD694,-1,0x6FD632,0,0x6FD636,0,0x6FD63A,0,0x6FD63E,0,0x6FD642,-1,0x6FD646,1,0x6FD64A,0,0x6FD64E},
	{69,"Flame Elemental",0x6EFA98,0x7C,1500,0x6EFA94,0x00,0x6EFAE8,0,0x6EFAEC,0x00,0x6EFAEA,0,0x6EFAF0,0,0x6EFB14,1,0x6EFAB2,-1,0x6EFAB6,0,0x6EFABA,0,0x6EFABE,0,0x6EFAC2,0,0x6EFAC6,0,0x6EFACA,0,0x6EFACE},
	{70,"Frost Elemental",0x6F0818,0x92,1500,0x6F0814,0x00,0x6F0868,0,0x6F086C,0x00,0x6F086A,0,0x6F0870,0,0x6F0894,-1,0x6F0832,1,0x6F0836,0,0x6F083A,0,0x6F083E,0,0x6F0842,1,0x6F0846,1,0x6F084A,1,0x6F084E},
	{71,"Thunder Elemental",0x776798,0xFC,1500,0x776794,0x00,0x7767E8,0,0x7767EC,0x00,0x7767EA,0,0x7767F0,0,0x776814,0,0x7767B2,0,0x7767B6,1,0x7767BA,-1,0x7767BE,-1,0x7767C2,0,0x7767C6,0,0x7767CA,0,0x7767CE},
	{72,"Doppelganger",0x6ED498,0x70,1000,0x6ED494,0x00,0x6ED4E8,0,0x6ED4EC,0x00,0x6ED4EA,0,0x6ED4F0,0,0x6ED514,0,0x6ED4B2,0,0x6ED4B6,0,0x6ED4BA,0,0x6ED4BE,0,0x6ED4C2,0,0x6ED4C6,0,0x6ED4CA,0,0x6ED4CE},
	{73,"Doppelganger",0x6ED398,0x6E,1500,0x6ED394,0x00,0x6ED3E8,0,0x6ED3EC,0x00,0x6ED3EA,0,0x6ED3F0,0,0x6ED414,0,0x6ED3B2,0,0x6ED3B6,0,0x6ED3BA,0,0x6ED3BE,0,0x6ED3C2,0,0x6ED3C6,0,0x6ED3CA,0,0x6ED3CE},
	{74,"Undead Parasite",0x6EC898,0x5C,1200,0x6EC894,0x00,0x6EC8E8,0,0x6EC8EC,0x00,0x6EC8EA,0,0x6EC8F0,0,0x6EC914,0,0x6EC8B2,0,0x6EC8B6,0,0x6EC8BA,0,0x6EC8BE,0,0x6EC8C2,-1,0x6EC8C6,0,0x6EC8CA,-1,0x6EC8CE},
	{75,"Golem",0x6FC998,0xA2,1500,0x6FC994,0x00,0x6FC9E8,0,0x6FC9EC,0x00,0x6FC9EA,0,0x6FC9F0,0,0x6FCA14,0,0x6FC9B2,0,0x6FC9B6,1,0x6FC9BA,1,0x6FC9BE,0,0x6FC9C2,0,0x6FC9C6,0,0x6FC9CA,0,0x6FC9CE},
	{76,"Succubus",0x776198,0xF2,1000,0x776194,0x00,0x7761E8,0,0x7761EC,0x00,0x7761EA,0,0x7761F0,0,0x776214,-1,0x7761B2,0,0x7761B6,0,0x7761BA,-1,0x7761BE,0,0x7761C2,0,0x7761C6,0,0x7761CA,0,0x7761CE},
	{77,"Medusa",0x6FD818,0xBA,2000,0x6FD814,0x00,0x6FD868,0,0x6FD86C,0x00,0x6FD86A,0,0x6FD870,0,0x6FD894,0,0x6FD832,0,0x6FD836,0,0x6FD83A,0,0x6FD83E,-1,0x6FD842,0,0x6FD846,0,0x6FD84A,0,0x6FD84E},
	{78,"Joachim",0x6FCE98,0xAC,1500,0x6FCE94,0x00,0x6FCEE8,0,0x6FCEEC,0x00,0x6FCEEA,0,0x6FCEF0,0,0x6FCF14,0,0x6FCEB2,-1,0x6FCEB6,0,0x6FCEBA,1,0x6FCEBE,1,0x6FCEC2,-1,0x6FCEC6,1,0x6FCECA,-1,0x6FCECE},
	{79,"Forgotten One",0x6ED298,0x6C,4800,0x6ED294,0x00,0x6ED2E8,0,0x6ED2EC,0x00,0x6ED2EA,0,0x6ED2F0,0,0x6ED314,1,0x6ED2B2,-1,0x6ED2B6,1,0x6ED2BA,0,0x6ED2BE,0,0x6ED2C2,0,0x6ED2C6,0,0x6ED2CA,1,0x6ED2CE},
	{80,"Walter",0x776E98,0x106,1500,0x776E94,0x00,0x776EE8,0,0x776EEC,0x00,0x776EEA,0,0x776EF0,0,0x776F14,0,0x776EB2,0,0x776EB6,0,0x776EBA,1,0x776EBE,1,0x776EC2,-1,0x776EC6,0,0x776ECA,-1,0x776ECE},
	{81,"Death",0x6ED098,0x6A,4444,0x6ED094,0x00,0x6ED0E8,0,0x6ED0EC,0x00,0x6ED0EA,0,0x6ED0F0,0,0x6ED114,0,0x6ED0B2,0,0x6ED0B6,0,0x6ED0BA,0,0x6ED0BE,0,0x6ED0C2,0,0x6ED0C6,0,0x6ED0CA,0,0x6ED0CE},
};
static int enemies_Length = *(&enemies + 1) - enemies;

//HINTS
void hints_write(FILE* fp)
{
	int locations[] =
	{
		//non-Repeatable
			0x20259A60, //Entrance
			0x2044CAE0, //Entrance
			0x2002CF80, //Entrance
			0x09A6A980, //ASML
			0x1745D4E0, //GFbT
			0x05323DE0, //HoSR
			0x23ED6C10, //Theatre
			0x117ECB60, //DPoW
			0x0A0C6B60, //ASML
			//EntrancePotion = 
			0x20259AE0,
			// GFbTPotion = 
			0x14AB1380,
			// DPoWPotion = 
			0x10BD2300,
			// ASMLFlameElementalPotion = 
			0x0B240470,
			// ASMLMegingjordPotion = 
			0x0C556070,
			// HoSR1stPotion = 
			0x0484E010,
			// HoSR2ndPotion = 
			0x049E4D80,
			// Theatre1stPotion = 
			0x23CE4480,
			// Theatre2ndPotion = 
			0x244AE000,
			// HoSRHighPotion = 
			0x02FC9690,
			// PoETHighPotion = 
			0x1F776C60,
			// ASMLHangedManHighPotion = 
			0x0B09D570,
			// ASMLMegingjordHighPotion = 
			0x0C555FF0,
			// TheatreHighPotion = 
			0x21EE6D10,
			// DPoWSuperPotion = 
			0x134CE560,
			// ASMLSuperPotion = 
			0x0C05D210,
			// PotMMSuperPotion = 
			0x1DE6CB60,
			// DPoWHeartRepair = 
			0x12B56990,
			// TheatreHeartRepair = 
			0x21D03C90,
			0x250F8570, //Theatre
			// EntranceSerum = 
			0x2044CB60,
			// HoSRSerum = 
			0x02C297B0,
			// EntranceUncursePotion = 
			0x20804560,
			// HoSRUncursePotion = 
			0x02C29830,
		//Magical Ticket
			0x208045E0,
		//Curtain Time Bell  
			0x233A5CE0,
			// HoSRNeapolitan = 
			0x0319A080,
			// ASMLShortcake = 
			0x0BBD3080,
			// HoSRRamen = 
			0x08017400, //
			// WhiteTigerKey = 
			0x089FB1E0, //HoSR
			// BlueDragonKey = 
			0x124C2060, //DPoW
			// RedPhoenixKey = 
			0x18D2EA60, //GFbT
			// BlackTurtleKey = 
			0x24F26C60, //Theatre
			// YellowDragonKey = 
			0x0A77A3E0, //ASML
			// AncientText2 = 
			0x2245EEE0, //Theatre
			// AncientText1 = 
			0x0BEFB3F0,
			// AncientText3 = 
			0x0C1BC8F0,
			// AncientText4 = 
			0x09D38F00,
			// Map1 = 
			0x2002D000, //Entrance
			// Map2 = 
			0x0D05C770, //ASML
			// Map3 = 
			0x0ECFB510, //DPoW
			// Map4 = 
			0x1599D480, //GFbT
			// Map5 = 
			0x238EC470, //Theatre
		//Event Items
			// ToolBag = 
			0x14467A60,
			// ETablet = 
			0x0B9E9C60,
			// VITablet = 
			0x1EDC0BE0,
			// DragonCrest = 
			0x1EF44160,
			// UnlockJewel = 
			0x1EAE6B60,
			// Svarog Statue = 
			0x209E5880,
			// WolfsFoot = 
			0x0AB0E960,
			// SaiseiIncense = 
			0x198E0B60,
			// BlackBishop = 
			0x08CFE360,
			// LucifersSword = 
			0x231FF460,
			// LittleHammer = 
			0x1233E0E0,
			// MeditativeIncence = 
			0x13337450, //
			// Draupnir = 
			0x13A638E0,
			// AromaEarring = 
			0x22B66E80,
			// RacoonCharm = 
			0x11C532E0,
			// BloodyCape = 
			0x08EA35E0,
			// RingofFire = 
			0x1491E9E0,
			// ArticRing = 
			0x0C34EAE0,
			// RingofThunder = 
			0x18A1DF60,
			// HeartBrooch = 
			0x22EB31E0,
			// JewelCrush = 
			0x172BE1E0,
			// Megingjord = 
			0x0C6C00E0,
			// Brisingamen = 
			0x23539760,
		//Heart Max Up
			// ASMLHPHeartUp = 
			0x0B240370,
			// ASMLFlameElementalHeartUp = 
			0x0B2403F0,
			// DPoWHeartUp1 = 
			0x11C53360, //1st?
			// DPoWHeartUp2 = 
			0x12197CE0, //2nd?
			// GFbTHeartUp = 
			0x19092400,
			// PotMMHeartUp = 
			0x1E1DDA60,
			// TheatreHeartUp = 
			0x22D0CF60,
		//MP Max Up
			// HoSRMPMaxUp = 
			0x088769E0,
			// ASMLMPMaxUp = 
			0x0BD97A80,
			// DPoWMPMaxUp = 
			0x12FD6ED0,
			// TheatreMPMaxUp = 
			0x22646A10,
			// PotMMMPMaxUp = 
			0x1E371160,
		//HP Max Up
			// HoSRHPMaxUp1 = 
			0x02C29730,
			// HoSRHPMaxUp2 = 
			0x086CF560,
			// ASMLHPMaxUp = 
			0x0B3E7EE0,
			// DPoWHPMaxUpBF1 = 
			0x0F9B0880,
			// HoSRHPMaxUpBF2 = 
			0x12004460,
			// TheatreHPMaxUp1 = 
			0x2305A7E0,
			// TheatreHPMaxUp2 = 
			0x23761F00,
			// GFbTHPMaxUp = 
			0x156654E0,
			//// PotMMDoppelHPMaxUp = 0x1C6B9510, //??? could cause problems
			// PotMMHPMaxUp = 
			0x1EC79C60, //
		//$1000
			// HoSR1000 = 
			0x026F22D0, //replace with whip of lightning?
			// ASML1000 = 
			0x0AC7A7E0, //replace with whip of flames?
			// DPoW1000 = 
			0x134CE5E0, //replace with whip of ice?
		//$400
			// GFbT4001 = 
			0x15665560, //replace with red orb?
			// GFbT4002 = 
			0x16343290, //replace with blue orb?
			// GFbT4003 = 
			0x1653B400, //replace with yellow orb?
			// GFbT4004 = 
			0x17120200, //replace with green orb?
			// HoSR400 = 
			0x04B91A10, //replace with purple orb?   

			// ASML4001 = 
			0x09908580, //replace with white bishop?

			// ASML4002 = 
			0x0ADDC6F0, //replace with Sacrificial doll?
			// DPoW4001 = 
			0x12B56890, //replace with Jade Mask?

			// DPoW4002 = 
			0x12B56910, //replace with Diamond?
			
			// Theatre4001 = 
			0x23ED6C90, //replace with earth plate?
			// Theatre4002 = 
			0x240C9D80, //replace with meteor plate?
			// Theatre4003 = 
			0x242B7880, //replace with moonlight plate?
			// Theatre4004 = 
			0x24A24E80, //replace with solar plate?
		//repeatable
		//Torches
		//Knives
			// HoSRKnife = 
			0x0708D580,
			// ASMLKnife = 
			0x0DA607F0,
			// GFbTKnife = 
			0x15FBCB00,
			// PotMMKnife = 
			0x1A6882F0,
			// TheatreKnife = 
			0x26237B70,
		//Axes
			// HoSRAxe = 
			0x0335EA80,
			// ASMLAxe = 
			0x0CED53F0,
			// DPoWFrostElementalAxe = 
			0x10171D80,
			// DPoWBridgeLeverAxe = 
			0x11034C70,
			// GFbTAxe = 
			0x15827A70,
			// PotMMAxe = 
			0x1A688370,
			// TheatreAxe = 
			0x242B5610,
		//Holy Water
			// HoSRHolyWater = 
			0x063C3EF0,
			// GFbTHolyWater =
			0x17778C00,
			// PotMMHolyWater = 
			0x1A6883F0,
			// TheatreHolyWater = 
			0x240C7A90,
		//Crystal
			// HoSRCrystal = 
			0x02DF7310,
			// ASMLCrystal = 
			0x0D05AA70,
			// DPoWCrystal = 
			0x114C3AF0,
			// GFbTCrystal = 
			0x17EC3EF0,
			// PotMMCrystal = 
			0x1A688470,
			// TheatreCrystal = 
			0x265E8270,
		//Cross
			// HoSRCross = 
			0x06703A00,
			// ASMLWhiteOrbCross = 
			0x0D1A24F0,
			// ASML3FCross = 
			0x0DE96970,
			// DPoWCross = 
			0x11636670,
			// PotMMCross = 
			0x1A6884F0,
		//$250
			0x14E46170, //replace with black orb?
		//common drops
			0x6FCAE8,
			0x6E7CE8,
			0x776FE8,
			0x7626E8,
			0x6E7AE8,
			0x6EF868,
			0x776BE8,
			0x760DE8,
			0x6E78E8,
			0x6FC8E8,
			0x7618E8,
			0x6EC7E8,
			0x6F02E8,
			0x6EFDE8,
			0x7763E8,
			0x6EFCE8,
			0x761FE8,
			0x6EC5E8,
			0x7619E8,
			0x6F04E8,
			0x7617E8,
			0x6F01E8,
			0x6F03E8,
			0x6F0D68,
			0x6FD368,
			0x6FC7E8,
			0x761BE8,
			0x6FCBE8,
			0x7622E8,
			0x760FE8,
			0x6FD968,
			0x7608E8,
			0x760CE8,
			0x7620E8,
			0x6FD468,
			0x6EC6E8,
			0x761AE8,
			0x6EFFE8,
			0x7764E8,
			0x6F0968,
			0x6F0A68,
			0x6EFEE8,
			0x776AE8,
			0x761CE8,
			0x6FCCE8,
			0x7621E8,
			0x7765E8,
			0x7769E8,
			0x761DE8,
			0x6ECCE8,
			0x7762E8,
			0x7616E8,
			0x6ECBE8,
			0x6FCDE8,
			0x6ECAE8,
			0x7623E8,
			0x6E7BE8,
			0x6EF768,
			0x6F00E8,
			0x761EE8,
			0x6F0B68,
			0x6FD568,
			0x6E79E8,
			0x760EE8,
			0x6ED5E8,
		//rare drops
			0x6FCAEA,
			0x6E7CEA,
			0x776FEA,
			0x7626EA,
			0x6E7AEA,
			0x6EF86A,
			0x776BEA,
			0x760DEA,
			0x6E78EA,
			0x6FC8EA,
			0x7618EA,
			0x6EC7EA,
			0x6F02EA,
			0x6EFDEA,
			0x7763EA,
			0x6EFCEA,
			0x761FEA,
			0x6EC5EA,
			0x7619EA,
			0x6F04EA,
			0x7617EA,
			0x6F01EA,
			0x6F03EA,
			0x6F0D6A,
			0x6FD36A,
			0x6FC7EA,
			0x761BEA,
			0x6FCBEA,
			0x7622EA,
			0x760FEA,
			0x6FD96A,
			0x7608EA,
			0x760CEA,
			0x7620EA,
			0x6FD46A,
			0x6EC6EA,
			0x761AEA,
			0x6EFFEA,
			0x7764EA,
			0x6F096A,
			0x6F0A6A,
			0x6EFEEA,
			0x776AEA,
			0x761CEA,
			0x6FCCEA,
			0x7621EA,
			0x7765EA,
			0x7769EA,
			0x761DEA,
			0x6ECCEA,
			0x7762EA,
			0x7616EA,
			0x6ECBEA,
			0x6FCDEA,
			0x6ECAEA,
			0x7623EA,
			0x6E7BEA,
			0x6EF76A,
			0x6F00EA,
			0x761EEA,
			0x6F0B6A,
			0x6FD56A,
			0x6E79EA,
			0x760EEA,
			0x6ED5EA,
	};
	int locations_Length = *(&locations + 1) - locations;
	
	char* locations_Names[] =
	{
			//non-Repeatable
			"MarkerStone1", //0x20259A60, //Entrance
			"MarkerStone2", //0x2044CAE0, //Entrance
			"MarkerStone3", //0x2002CF80, //Entrance
			"MarkerStone4", //0x09A6A980, //ASML
			"MarkerStone5", //0x1745D4E0, //GFbT
			"MarkerStone6", //0x05323DE0, //HoSR
			"MarkerStone7", //0x23ED6C10, //Theatre
			"MarkerStone8", //0x117ECB60, //DPoW
			"WhiteOrb", //0x0A0C6B60, //ASML
			"EntrancePotion", 
			//20259AE0,
			"GFbTPotion", 
			//14AB1380,
			"DPoWPotion", 
			//10BD2300,
			"ASMLElementalPotion", 
			//0B240470,
			"ASMLMegingjordPotion", 
			//0C556070,
			"HoSR1stPotion", 
			//0484E010,
			"HoSR2ndPotion", 
			//049E4D80,
			"Theatre1stPotion", 
			//23CE4480,
			"Theatre2ndPotion", 
			//244AE000,
			"HoSRHighPotion", 
			//02FC9690,
			"PoETHighPotion", 
			//1F776C60,
			"ASMLHangedManHighPotion", 
			//0B09D570,
			"MegingjordHighPotion", 
			//0C555FF0,
			"TheatreHighPotion", 
			//21EE6D10,
			"DPoWSuperPotion", 
			//134CE560,
			"ASMLSuperPotion", 
			//0C05D210,
			"PotMMSuperPotion", 
			//1DE6CB60,
			"DPoWHeartRepair", 
			//12B56990,
			"TheatreHeartRepair", 
			//21D03C90,
			"ManaPrism",
			//250F8570, //Theatre
			"EntranceSerum", 
			//2044CB60,
			"HoSRSerum", 
			//02C297B0,
			"EntranceUncursePotion", 
			//20804560,
			"HoSRUncursePotion", 
			//02C29830,
			"MagicalTicket", 
			//208045E0,
			"CurtainTimeBell",  
			//233A5CE0,
			"HoSRNeapolitan", 
			//0319A080,
			"ASMLShortcake", 
			//0BBD3080,
			"HoSRRamen", 
			//08017400, //
			"WhiteTigerKey", 
			//089FB1E0, //HoSR
			"BlueDragonKey", 
			//124C2060, //DPoW
			"RedPhoenixKey", 
			//18D2EA60, //GFbT
			"BlackTurtleKey", 
			//24F26C60, //Theatre
			"YellowDragonKey", 
			//0A77A3E0, //ASML
			"AncientText2", 
			//2245EEE0, //Theatre
			"AncientText1", 
			//0BEFB3F0,
			"AncientText3", 
			//0C1BC8F0,
			"AncientText4", 
			//09D38F00,
			"Map1", 
			//2002D000, //Entrance
			"Map2", 
			//0D05C770, //ASML
			"Map3", 
			//0ECFB510, //DPoW
			"Map4", 
			//1599D480, //GFbT
			"Map5", 
			//238EC470, //Theatre
		//Event Items
			"ToolBag", 
			//14467A60,
			"ETablet", 
			//0B9E9C60,
			"VITablet", 
			//1EDC0BE0,
			"DragonCrest", 
			//1EF44160,
			"UnlockJewel", 
			//1EAE6B60,
			"Svarog Statue", 
			//209E5880,
			"WolfsFoot", 
			//0AB0E960,
			"SaiseiIncense", 
			//198E0B60,
			"BlackBishop", 
			//08CFE360,
			"LucifersSword", 
			//231FF460,
			"LittleHammer", 
			//1233E0E0,
			"MeditativeIncence", 
			//13337450, //
			"Draupnir", 
			//13A638E0,
			"AromaEarring", 
			//22B66E80,
			"RacoonCharm", 
			//11C532E0,
			"BloodyCape", 
			//08EA35E0,
			"RingofFire", 
			//1491E9E0,
			"ArticRing", 
			//0C34EAE0,
			"RingofThunder", 
			//18A1DF60,
			"HeartBrooch", 
			//22EB31E0,
			"JewelCrush", 
			//172BE1E0,
			"Megingjord", 
			//0C6C00E0,
			"Brisingamen", 
			//23539760,
		//Heart Max Up
			"ASMLHPHeartUp", 
			//0B240370,
			"ASMLElementalHeartUp", 
			//0B2403F0,
			"DPoWHeartUp1", 
			//11C53360, //1st?
			"DPoWHeartUp2", 
			//12197CE0, //2nd?
			"GFbTHeartUp", 
			//19092400,
			"PotMMHeartUp", 
			//1E1DDA60,
			"TheatreHeartUp", 
			//22D0CF60,
		//MP Max Up
			"HoSRMPMaxUp", 
			//088769E0,
			"ASMLMPMaxUp", 
			//0BD97A80,
			"DPoWMPMaxUp", 
			//12FD6ED0,
			"TheatreMPMaxUp", 
			//22646A10,
			"PotMMMPMaxUp", 
			//1E371160,
		//HP Max Up
			"HoSRHPMaxUp1", 
			//02C29730,
			"HoSRHPMaxUp2", 
			//086CF560,
			"ASMLHPMaxUp", 
			//0B3E7EE0,
			"DPoWHPMaxUpBF1", 
			//0F9B0880,
			"HoSRHPMaxUpBF2", 
			//12004460,
			"TheatreHPMaxUp1", 
			//2305A7E0,
			"TheatreHPMaxUp2", 
			//23761F00,
			"GFbTHPMaxUp", 
			//156654E0,
			//"PotMMDoppelHPMaxUp", //1C6B9510, //??? could cause problems
			"PotMMHPMaxUp", 
			//1EC79C60, //
		//$1000
			"HoSR1000", 
			//026F22D0, //replace with whip of lightning?
			"ASML1000", 
			//0AC7A7E0, //replace with whip of flames?
			"DPoW1000", 
			//134CE5E0, //replace with whip of ice?
		//$400
			"GFbT4001", 
			//15665560, //replace with red orb?
			"GFbT4002", 
			//16343290, //replace with blue orb?
			"GFbT4003", 
			//1653B400, //replace with yellow orb?
			"GFbT4004", 
			//17120200, //replace with green orb?
			"HoSR400", 
			//04B91A10, //replace with purple orb?   

			"ASML4001", 
			//09908580, //replace with white bishop?

			"ASML4002", 
			//0ADDC6F0, //replace with Sacrificial doll?
			"DPoW4001", 
			//12B56890, //replace with Jade Mask?

			"DPoW4002", 
			//12B56910, //replace with Diamond?
			
			"Theatre4001", 
			//23ED6C90, //replace with earth plate?
			"Theatre4002", 
			//240C9D80, //replace with meteor plate?
			"Theatre4003", 
			//242B7880, //replace with moonlight plate?
			"Theatre4004", 
			//24A24E80, //replace with solar plate?
		//repeatable
		//Torches
		//Knives
			"HoSRKnife", 
			//0708D580,
			"ASMLKnife", 
			//0DA607F0,
			"GFbTKnife", 
			//15FBCB00,
			"PotMMKnife", 
			//1A6882F0,
			"TheatreKnife", 
			//26237B70,
		//Axes
			"HoSRAxe", 
			//0335EA80,
			"ASMLAxe", 
			//0CED53F0,
			"DPoWFrostElementalAxe", 
			//10171D80,
			"DPoWBridgeLeverAxe", 
			//11034C70,
			"GFbTAxe", 
			//15827A70,
			"PotMMAxe", 
			//1A688370,
			"TheatreAxe", 
			//242B5610,
		//Holy Water
			"HoSRHolyWater", 
			//063C3EF0,
			"GFbTHolyWater",
			//17778C00,
			"PotMMHolyWater", 
			//1A6883F0,
			"TheatreHolyWater", 
			//240C7A90,
		//Crystal
			"HoSRCrystal", 
			//02DF7310,
			"ASMLCrystal", 
			//0D05AA70,
			"DPoWCrystal", 
			//114C3AF0,
			"GFbTCrystal", 
			//17EC3EF0,
			"PotMMCrystal", 
			//1A688470,
			"TheatreCrystal", 
			//265E8270,
		//Cross
			"HoSRCross", 
			//06703A00,
			"ASMLWhiteOrbCross", 
			//0D1A24F0,
			"ASML3FCross", 
			//0DE96970,
			"DPoWCross", 
			//11636670,
			"PotMMCross", 
			//1A6884F0,
			"$250Torch",
			//14E46170, //replace with black orb?
		//common drops
	};
	int location_name_LEN = *(&locations_Names + 1) - locations_Names;
	int SIZE = 1;
	unsigned char buffer[SIZE];
	char newLine = 0x0A;
	int index = 0;
	int address;
	int enemy_index;
	for(int i = 0; i < locations_Length; i++)
	{
		fseek(fp,locations[i],SEEK_SET);
		fread(buffer,sizeof(buffer),1,fp);
		if(i < location_name_LEN)
		{
			switch(buffer[0])
			{
				case 0x02:
					index = 0;
					address = 0x1197208;
					fseek(fp,address,SEEK_SET);
					for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
					{
						fseek(fp,address+j,SEEK_SET);
						fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
					for(int j = 0; locations_Names[i][j] != '\0'; j++)
					{
						fseek(fp,address+index,SEEK_SET);
						fwrite(&locations_Names[i][j],sizeof(locations_Names[i][j]),1,fp);
						index++;
					}		
					fseek(fp,address+index,SEEK_SET);
					newByte = 0x00;
					fwrite(&newByte,sizeof(newByte),1,fp);
					break;
				case 0x03:
					index = 0;
					address = 0x1196F50;
					fseek(fp,address,SEEK_SET);
					for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
					{
						fseek(fp,address+j,SEEK_SET);
						fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
					for(int j = 0; locations_Names[i][j] != '\0'; j++)
					{
						fseek(fp,address+index,SEEK_SET);
						fwrite(&locations_Names[i][j],sizeof(locations_Names[i][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);
					newByte = 0x00;
					fwrite(&newByte,sizeof(newByte),1,fp);
					break;
				case 0x04:
					index = 0;
					address = 0x1196168;
					fseek(fp,address,SEEK_SET);
					for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
					{
						fseek(fp,address+j,SEEK_SET);
						fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
					for(int j = 0; locations_Names[i][j] != '\0'; j++)
					{
						fseek(fp,address+index,SEEK_SET);
						fwrite(&locations_Names[i][j],sizeof(locations_Names[i][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);
					newByte = 0x00;
					fwrite(&newByte,sizeof(newByte),1,fp);
					break;
				case 0x06:
					index = 0;
					address = 0x1194510;
					fseek(fp,address,SEEK_SET);
					for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
					{
						fseek(fp,address+j,SEEK_SET);
						fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
					for(int j = 0; locations_Names[i][j] != '\0'; j++)
					{
						fseek(fp,address+index,SEEK_SET);
						fwrite(&locations_Names[i][j],sizeof(locations_Names[i][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);
					newByte = 0x00;
					fwrite(&newByte,sizeof(newByte),1,fp);
					break;
				case 0x07:
					index = 0;
					address = 0x1195850;
					fseek(fp,address,SEEK_SET);
					for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
					{
						fseek(fp,address+j,SEEK_SET);
						fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
					for(int j = 0; locations_Names[i][j] != '\0'; j++)
					{
						fseek(fp,address+index,SEEK_SET);
						fwrite(&locations_Names[i][j],sizeof(locations_Names[i][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);
					newByte = 0x00;
					fwrite(&newByte,sizeof(newByte),1,fp);
					break;
				case 0x08:
					index = 0;
					address = 0x1195800;
					fseek(fp,address,SEEK_SET);
					for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
					{
						fseek(fp,address+j,SEEK_SET);
						fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
					for(int j = 0; locations_Names[i][j] != '\0'; j++)
					{
						fseek(fp,address+index,SEEK_SET);
						fwrite(&locations_Names[i][j],sizeof(locations_Names[i][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);
					newByte = 0x00;
					fwrite(&newByte,sizeof(newByte),1,fp);
					break;
				case 0x09:
					index = 0;
					address = 0x11957B0;
					fseek(fp,address,SEEK_SET);
					for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
					{
						fseek(fp,address+j,SEEK_SET);
						fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
					for(int j = 0; locations_Names[i][j] != '\0'; j++)
					{
						fseek(fp,address+index,SEEK_SET);
						fwrite(&locations_Names[i][j],sizeof(locations_Names[i][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);
					newByte = 0x00;
					fwrite(&newByte,sizeof(newByte),1,fp);
					break;
				case 0x0A:
					index = 0;
					address = 0x1195F28;
					fseek(fp,address,SEEK_SET);
					for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
					{
						fseek(fp,address+j,SEEK_SET);
						fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
					for(int j = 0; locations_Names[i][j] != '\0'; j++)
					{
						fseek(fp,address+index,SEEK_SET);
						fwrite(&locations_Names[i][j],sizeof(locations_Names[i][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);
					newByte = 0x00;
					fwrite(&newByte,sizeof(newByte),1,fp);
					break;
				case 0x0B:
					index = 0;
					address = 0x1195F68;
					fseek(fp,address,SEEK_SET);
					for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
					{
						fseek(fp,address+j,SEEK_SET);
						fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
					for(int j = 0; locations_Names[i][j] != '\0'; j++)
					{
						fseek(fp,address+index,SEEK_SET);
						fwrite(&locations_Names[i][j],sizeof(locations_Names[i][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);
					newByte = 0x00;
					fwrite(&newByte,sizeof(newByte),1,fp);
					break;
				case 0x16:
					index = 0;
					address = 0x1194D08;
					fseek(fp,address,SEEK_SET);
					for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
					{
						fseek(fp,address+j,SEEK_SET);
						fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
					for(int j = 0; locations_Names[i][j] != '\0'; j++)
					{
						fseek(fp,address+index,SEEK_SET);
						fwrite(&locations_Names[i][j],sizeof(locations_Names[i][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);
					newByte = 0x00;
					fwrite(&newByte,sizeof(newByte),1,fp);
					break;
				case 0x17:
					break;
				case 0x18:
					/*
					index = 0;
					address = 0x1195C30;
					fseek(fp,address,SEEK_SET);
					for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
					{
						fseek(fp,address+j,SEEK_SET);
						fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
					for(int j = 0; locations_Names[i][j] != '\0'; j++)
					{
						fseek(fp,address+index,SEEK_SET);
						fwrite(&locations_Names[i][j],sizeof(locations_Names[i][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);
					newByte = 0x00;
					fwrite(&newByte,sizeof(newByte),1,fp);
					*/
					break;
				case 0x19:
					index = 0;
					address = 0x11959C0;
					fseek(fp,address,SEEK_SET);
					for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
					{
						fseek(fp,address+j,SEEK_SET);
						fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
					for(int j = 0; locations_Names[i][j] != '\0'; j++)
					{
						fseek(fp,address+index,SEEK_SET);
						fwrite(&locations_Names[i][j],sizeof(locations_Names[i][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);
					newByte = 0x00;
					fwrite(&newByte,sizeof(newByte),1,fp);
					break;
				case 0x1A:
					index = 0;
					address = 0x1195BE8;
					fseek(fp,address,SEEK_SET);
					for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
					{
						fseek(fp,address+j,SEEK_SET);
						fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
					for(int j = 0; locations_Names[i][j] != '\0'; j++)
					{
						fseek(fp,address+index,SEEK_SET);
						fwrite(&locations_Names[i][j],sizeof(locations_Names[i][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);
					newByte = 0x00;
					fwrite(&newByte,sizeof(newByte),1,fp);
					break;
				case 0x1C:
					index = 0;
					address = 0x11972E8;
					fseek(fp,address,SEEK_SET);
					for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
					{
						fseek(fp,address+j,SEEK_SET);
						fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
					for(int j = 0; locations_Names[i][j] != '\0'; j++)
					{
						fseek(fp,address+index,SEEK_SET);
						fwrite(&locations_Names[i][j],sizeof(locations_Names[i][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);
					newByte = 0x00;
					fwrite(&newByte,sizeof(newByte),1,fp);
					break;
				case 0x1D:
					break;
				case 0x1E:
					break;
				case 0x20:
					break;
				case 0x21:
					index = 0;
					address = 0x11959E8;
					fseek(fp,address,SEEK_SET);
					for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
					{
						fseek(fp,address+j,SEEK_SET);
						fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
					for(int j = 0; locations_Names[i][j] != '\0'; j++)
					{
						fseek(fp,address+index,SEEK_SET);
						fwrite(&locations_Names[i][j],sizeof(locations_Names[i][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);
					newByte = 0x00;
					fwrite(&newByte,sizeof(newByte),1,fp);
					break;
				case 0x22:
					break;
				case 0x3B:
					index = 0;
					address = 0x11974D0;
					fseek(fp,address,SEEK_SET);
					for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
					{
						fseek(fp,address+j,SEEK_SET);
						fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
					for(int j = 0; locations_Names[i][j] != '\0'; j++)
					{
						fseek(fp,address+index,SEEK_SET);
						fwrite(&locations_Names[i][j],sizeof(locations_Names[i][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);
					newByte = 0x00;
					fwrite(&newByte,sizeof(newByte),1,fp);
					break;
				case 0x3C:
					break;
				case 0x3D:
					break;
				case 0x40:
					break;
				case 0x5F:
					break;
				case 0x7F:
					index = 0;
					address = 0x1196D60;
					fseek(fp,address,SEEK_SET);
					for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
					{
						fseek(fp,address+j,SEEK_SET);
						fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
					for(int j = 0; locations_Names[i][j] != '\0'; j++)
					{
						fseek(fp,address+index,SEEK_SET);
						fwrite(&locations_Names[i][j],sizeof(locations_Names[i][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);
					newByte = 0x00;
					fwrite(&newByte,sizeof(newByte),1,fp);
					break;
				case 0x80:
					index = 0;
					address = 0x1197698;
					fseek(fp,address,SEEK_SET);
					for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
					{
						fseek(fp,address+j,SEEK_SET);
						fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
					for(int j = 0; locations_Names[i][j] != '\0'; j++)
					{
						fseek(fp,address+index,SEEK_SET);
						fwrite(&locations_Names[i][j],sizeof(locations_Names[i][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);
					newByte = 0x00;
					fwrite(&newByte,sizeof(newByte),1,fp);
					break;
				case 0x81:
					index = 0;
					address = 0x1196318;
					fseek(fp,address,SEEK_SET);
					for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
					{
						fseek(fp,address+j,SEEK_SET);
						fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
					for(int j = 0; locations_Names[i][j] != '\0'; j++)
					{
						fseek(fp,address+index,SEEK_SET);
						fwrite(&locations_Names[i][j],sizeof(locations_Names[i][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);
					newByte = 0x00;
					fwrite(&newByte,sizeof(newByte),1,fp);
					break;
				case 0x82:
					index = 0;
					address = 0x1196BE8;
					fseek(fp,address,SEEK_SET);
					for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
					{
						fseek(fp,address+j,SEEK_SET);
						fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
					for(int j = 0; locations_Names[i][j] != '\0'; j++)
					{
						fseek(fp,address+index,SEEK_SET);
						fwrite(&locations_Names[i][j],sizeof(locations_Names[i][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);
					newByte = 0x00;
					fwrite(&newByte,sizeof(newByte),1,fp);
					break;
				case 0x83:
					index = 0;
					address = 0x1196A70;
					fseek(fp,address,SEEK_SET);
					for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
					{
						fseek(fp,address+j,SEEK_SET);
						fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
					for(int j = 0; locations_Names[i][j] != '\0'; j++)
					{
						fseek(fp,address+index,SEEK_SET);
						fwrite(&locations_Names[i][j],sizeof(locations_Names[i][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);
					newByte = 0x00;
					fwrite(&newByte,sizeof(newByte),1,fp);
					break;
				case 0x84:
					break;
				case 0x85:
					index = 0;
					address = 0x1197470;
					fseek(fp,address,SEEK_SET);
					for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
					{
						fseek(fp,address+j,SEEK_SET);
						fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
					for(int j = 0; locations_Names[i][j] != '\0'; j++)
					{
						fseek(fp,address+index,SEEK_SET);
						fwrite(&locations_Names[i][j],sizeof(locations_Names[i][j]),1,fp);
						index++;
					}
					fseek(fp,address+index,SEEK_SET);
					newByte = 0x00;
					fwrite(&newByte,sizeof(newByte),1,fp);
					break;
				case 0x86:
					break;
				case 0x8D:
					break;
				case 0x8E:
					break;
				default:
					break;
			}
		}
		else
		{
			if(i > location_name_LEN)
			{
				if(i >= location_name_LEN && i < locations_Length - 65)
				{	
					enemy_index = i - location_name_LEN;
				}
				else if(i >= location_name_LEN + 65)
				{
					enemy_index = i - (location_name_LEN+65);
				}
				switch(buffer[0])
				{
					case 0x02:
						index = 0;
						address = 0x1197208;
						fseek(fp,address,SEEK_SET);
						for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
						{
							fseek(fp,address+j,SEEK_SET);
							fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
						for(int j = 0; enemies[enemy_index].name[j] != '\0'; j++)
						{
							fseek(fp,address+index,SEEK_SET);
							fwrite(&enemies[enemy_index].name[j],sizeof(enemies[enemy_index].name[j]),1,fp);
							index++;
						}		
						fseek(fp,address+index,SEEK_SET);
						newByte = 0x00;
						fwrite(&newByte,sizeof(newByte),1,fp);
						break;
					case 0x03:
						index = 0;
						address = 0x1196F50;
						fseek(fp,address,SEEK_SET);
						for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
						{
							fseek(fp,address+j,SEEK_SET);
							fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
						for(int j = 0; enemies[enemy_index].name[j] != '\0'; j++)
						{
							fseek(fp,address+index,SEEK_SET);
							fwrite(&enemies[enemy_index].name[j],sizeof(enemies[enemy_index].name[j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);
						newByte = 0x00;
						fwrite(&newByte,sizeof(newByte),1,fp);
						break;
					case 0x04:
						index = 0;
						address = 0x1196168;
						fseek(fp,address,SEEK_SET);
						for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
						{
							fseek(fp,address+j,SEEK_SET);
							fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
						for(int j = 0; enemies[enemy_index].name[j] != '\0'; j++)
						{
							fseek(fp,address+index,SEEK_SET);
							fwrite(&enemies[enemy_index].name[j],sizeof(enemies[enemy_index].name[j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);
						newByte = 0x00;
						fwrite(&newByte,sizeof(newByte),1,fp);
						break;
					case 0x06:
						index = 0;
						address = 0x1194510;
						fseek(fp,address,SEEK_SET);
						for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
						{
							fseek(fp,address+j,SEEK_SET);
							fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
						for(int j = 0; enemies[enemy_index].name[j] != '\0'; j++)
						{
							fseek(fp,address+index,SEEK_SET);
							fwrite(&enemies[enemy_index].name[j],sizeof(enemies[enemy_index].name[j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);
						newByte = 0x00;
						fwrite(&newByte,sizeof(newByte),1,fp);
						break;
					case 0x07:
						index = 0;
						address = 0x1195850;
						fseek(fp,address,SEEK_SET);
						for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
						{
							fseek(fp,address+j,SEEK_SET);
							fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
						for(int j = 0; enemies[enemy_index].name[j] != '\0'; j++)
						{
							fseek(fp,address+index,SEEK_SET);
							fwrite(&enemies[enemy_index].name[j],sizeof(enemies[enemy_index].name[j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);
						newByte = 0x00;
						fwrite(&newByte,sizeof(newByte),1,fp);
						break;
					case 0x08:
						index = 0;
						address = 0x1195800;
						fseek(fp,address,SEEK_SET);
						for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
						{
							fseek(fp,address+j,SEEK_SET);
							fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
						for(int j = 0; enemies[enemy_index].name[j] != '\0'; j++)
						{
							fseek(fp,address+index,SEEK_SET);
							fwrite(&enemies[enemy_index].name[j],sizeof(enemies[enemy_index].name[j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);
						newByte = 0x00;
						fwrite(&newByte,sizeof(newByte),1,fp);
						break;
					case 0x09:
						index = 0;
						address = 0x11957B0;
						fseek(fp,address,SEEK_SET);
						for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
						{
							fseek(fp,address+j,SEEK_SET);
							fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
						for(int j = 0; enemies[enemy_index].name[j] != '\0'; j++)
						{
							fseek(fp,address+index,SEEK_SET);
							fwrite(&enemies[enemy_index].name[j],sizeof(enemies[enemy_index].name[j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);
						newByte = 0x00;
						fwrite(&newByte,sizeof(newByte),1,fp);
						break;
					case 0x0A:
						index = 0;
						address = 0x1195F28;
						fseek(fp,address,SEEK_SET);
						for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
						{
							fseek(fp,address+j,SEEK_SET);
							fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
						for(int j = 0; enemies[enemy_index].name[j] != '\0'; j++)
						{
							fseek(fp,address+index,SEEK_SET);
							fwrite(&enemies[enemy_index].name[j],sizeof(enemies[enemy_index].name[j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);
						newByte = 0x00;
						fwrite(&newByte,sizeof(newByte),1,fp);
						break;
					case 0x0B:
						index = 0;
						address = 0x1195F68;
						fseek(fp,address,SEEK_SET);
						for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
						{
							fseek(fp,address+j,SEEK_SET);
							fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
						for(int j = 0; enemies[enemy_index].name[j] != '\0'; j++)
						{
							fseek(fp,address+index,SEEK_SET);
							fwrite(&enemies[enemy_index].name[j],sizeof(enemies[enemy_index].name[j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);
						newByte = 0x00;
						fwrite(&newByte,sizeof(newByte),1,fp);
						break;
					case 0x16:
						index = 0;
						address = 0x1194D08;
						fseek(fp,address,SEEK_SET);
						for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
						{
							fseek(fp,address+j,SEEK_SET);
							fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
						for(int j = 0; enemies[enemy_index].name[j] != '\0'; j++)
						{
							fseek(fp,address+index,SEEK_SET);
							fwrite(&enemies[enemy_index].name[j],sizeof(enemies[enemy_index].name[j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);
						newByte = 0x00;
						fwrite(&newByte,sizeof(newByte),1,fp);
						break;
					case 0x17:
						break;
					case 0x18:
						/*
						index = 0;
						address = 0x1195C30;
						fseek(fp,address,SEEK_SET);
						for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
						{
							fseek(fp,address+j,SEEK_SET);
							fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
						for(int j = 0; enemies[enemy_index].name[j] != '\0'; j++)
						{
							fseek(fp,address+index,SEEK_SET);
							fwrite(&enemies[enemy_index].name[j],sizeof(enemies[enemy_index].name[j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);
						newByte = 0x00;
						fwrite(&newByte,sizeof(newByte),1,fp);
						*/
						break;
					case 0x19:
						index = 0;
						address = 0x11959C0;
						fseek(fp,address,SEEK_SET);
						for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
						{
							fseek(fp,address+j,SEEK_SET);
							fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
						for(int j = 0; enemies[enemy_index].name[j] != '\0'; j++)
						{
							fseek(fp,address+index,SEEK_SET);
							fwrite(&enemies[enemy_index].name[j],sizeof(enemies[enemy_index].name[j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);
						newByte = 0x00;
						fwrite(&newByte,sizeof(newByte),1,fp);
						break;
					case 0x1A:
						index = 0;
						address = 0x1195BE8;
						fseek(fp,address,SEEK_SET);
						for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
						{
							fseek(fp,address+j,SEEK_SET);
							fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
						for(int j = 0; enemies[enemy_index].name[j] != '\0'; j++)
						{
							fseek(fp,address+index,SEEK_SET);
							fwrite(&enemies[enemy_index].name[j],sizeof(enemies[enemy_index].name[j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);
						newByte = 0x00;
						fwrite(&newByte,sizeof(newByte),1,fp);
						break;
					case 0x1C:
						index = 0;
						address = 0x11972E8;
						fseek(fp,address,SEEK_SET);
						for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
						{
							fseek(fp,address+j,SEEK_SET);
							fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
						for(int j = 0; enemies[enemy_index].name[j] != '\0'; j++)
						{
							fseek(fp,address+index,SEEK_SET);
							fwrite(&enemies[enemy_index].name[j],sizeof(enemies[enemy_index].name[j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);
						newByte = 0x00;
						fwrite(&newByte,sizeof(newByte),1,fp);
						break;
					case 0x1D:
						break;
					case 0x1E:
						break;
					case 0x20:
						break;
					case 0x21:
						index = 0;
						address = 0x11959E8;
						fseek(fp,address,SEEK_SET);
						for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
						{
							fseek(fp,address+j,SEEK_SET);
							fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
						for(int j = 0; enemies[enemy_index].name[j] != '\0'; j++)
						{
							fseek(fp,address+index,SEEK_SET);
							fwrite(&enemies[enemy_index].name[j],sizeof(enemies[enemy_index].name[j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);
						newByte = 0x00;
						fwrite(&newByte,sizeof(newByte),1,fp);
						break;
					case 0x22:
						break;
					case 0x3B:
						index = 0;
						address = 0x11974D0;
						fseek(fp,address,SEEK_SET);
						for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
						{
							fseek(fp,address+j,SEEK_SET);
							fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
						for(int j = 0; enemies[enemy_index].name[j] != '\0'; j++)
						{
							fseek(fp,address+index,SEEK_SET);
							fwrite(&enemies[enemy_index].name[j],sizeof(enemies[enemy_index].name[j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);
						newByte = 0x00;
						fwrite(&newByte,sizeof(newByte),1,fp);
						break;
					case 0x3C:
						break;
					case 0x3D:
						break;
					case 0x40:
						break;
					case 0x5F:
						break;
					case 0x7F:
						index = 0;
						address = 0x1196D60;
						fseek(fp,address,SEEK_SET);
						for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
						{
							fseek(fp,address+j,SEEK_SET);
							fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
						for(int j = 0; enemies[enemy_index].name[j] != '\0'; j++)
						{
							fseek(fp,address+index,SEEK_SET);
							fwrite(&enemies[enemy_index].name[j],sizeof(enemies[enemy_index].name[j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);
						newByte = 0x00;
						fwrite(&newByte,sizeof(newByte),1,fp);
						break;
					case 0x80:
						index = 0;
						address = 0x1197698;
						fseek(fp,address,SEEK_SET);
						for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
						{
							fseek(fp,address+j,SEEK_SET);
							fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
						for(int j = 0; enemies[enemy_index].name[j] != '\0'; j++)
						{
							fseek(fp,address+index,SEEK_SET);
							fwrite(&enemies[enemy_index].name[j],sizeof(enemies[enemy_index].name[j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);
						newByte = 0x00;
						fwrite(&newByte,sizeof(newByte),1,fp);
						break;
					case 0x81:
						index = 0;
						address = 0x1196318;
						fseek(fp,address,SEEK_SET);
						for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
						{
							fseek(fp,address+j,SEEK_SET);
							fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
						for(int j = 0; enemies[enemy_index].name[j] != '\0'; j++)
						{
							fseek(fp,address+index,SEEK_SET);
							fwrite(&enemies[enemy_index].name[j],sizeof(enemies[enemy_index].name[j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);
						newByte = 0x00;
						fwrite(&newByte,sizeof(newByte),1,fp);
						break;
					case 0x82:
						index = 0;
						address = 0x1196BE8;
						fseek(fp,address,SEEK_SET);
						for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
						{
							fseek(fp,address+j,SEEK_SET);
							fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
						for(int j = 0; enemies[enemy_index].name[j] != '\0'; j++)
						{
							fseek(fp,address+index,SEEK_SET);
							fwrite(&enemies[enemy_index].name[j],sizeof(enemies[enemy_index].name[j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);
						newByte = 0x00;
						fwrite(&newByte,sizeof(newByte),1,fp);
						break;
					case 0x83:
						index = 0;
						address = 0x1196A70;
						fseek(fp,address,SEEK_SET);
						for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
						{
							fseek(fp,address+j,SEEK_SET);
							fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
						for(int j = 0; enemies[enemy_index].name[j] != '\0'; j++)
						{
							fseek(fp,address+index,SEEK_SET);
							fwrite(&enemies[enemy_index].name[j],sizeof(enemies[enemy_index].name[j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);
						newByte = 0x00;
						fwrite(&newByte,sizeof(newByte),1,fp);
						break;
					case 0x84:
						break;
					case 0x85:
						index = 0;
						address = 0x1197470;
						fseek(fp,address,SEEK_SET);
						for(int j = 0; itemNames[buffer[0]][j] != '\0'; j++)
						{
							fseek(fp,address+j,SEEK_SET);
							fwrite(&itemNames[buffer[0]][j],sizeof(itemNames[buffer[0]][j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);fwrite(&newLine,sizeof(newLine),1,fp);index++;
						for(int j = 0; enemies[enemy_index].name[j] != '\0'; j++)
						{
							fseek(fp,address+index,SEEK_SET);
							fwrite(&enemies[enemy_index].name[j],sizeof(enemies[enemy_index].name[j]),1,fp);
							index++;
						}
						fseek(fp,address+index,SEEK_SET);
						newByte = 0x00;
						fwrite(&newByte,sizeof(newByte),1,fp);
						break;
					case 0x86:
						break;
					case 0x8D:
						break;
					case 0x8E:
						break;
					default:
						break;
				}
			}
		}
	}
	int text_fix_addresses[] = {
		0x119722E,0x1196F75,0x11961A8,0x1194543,
		0x119588E,0x119584C,0x11957FD,0x1195F66,0x1195F97,0x1194D42,
		0x1195C63,0x11959E7,0x1195C29,0x1197335,0x1195A17,0x1197513,
		0x1196D81,0x11976EA,0x1196369,0x1196C34,0x1196AA8,0x11974C3,
	};
	int text_fix_addresses_LEN = *(&text_fix_addresses + 1) - text_fix_addresses;
	for(int i = 0; i < text_fix_addresses_LEN; i++)
	{
		fseek(fp,text_fix_addresses[i],SEEK_SET);
		newByte = 0x00;
		fwrite(&newByte,sizeof(newByte),1,fp);
	}
}
void hints_items(FILE* fp, int ItemID, int addressNumber, int locations_Length)
{
	//Ancient Texts should hint: E tablet, Curtain Time Bell, Unlock Jewel, Dragon Crest
	//Maps should hint keys?
	//Marker Stones should hint bosses
	//unlock jewel should hint boss 
	//dragon crest should hint Vampire Killer
	char* locations[] =
	{
			//non-Repeatable
			"MarkerStone1", //0x20259A60, //Entrance
			"MarkerStone2", //0x2044CAE0, //Entrance
			"MarkerStone3", //0x2002CF80, //Entrance
			"MarkerStone4", //0x09A6A980, //ASML
			"MarkerStone5", //0x1745D4E0, //GFbT
			"MarkerStone6", //0x05323DE0, //HoSR
			"MarkerStone7", //0x23ED6C10, //Theatre
			"MarkerStone8", //0x117ECB60, //DPoW
			"WhiteOrb", //0x0A0C6B60, //ASML
			"EntrancePotion", 
			//20259AE0,
			"GFbTPotion", 
			//14AB1380,
			"DPoWPotion", 
			//10BD2300,
			"ASMLElementalPotion", 
			//0B240470,
			"ASMLMegingjordPotion", 
			//0C556070,
			"HoSR1stPotion", 
			//0484E010,
			"HoSR2ndPotion", 
			//049E4D80,
			"Theatre1stPotion", 
			//23CE4480,
			"Theatre2ndPotion", 
			//244AE000,
			"HoSRHighPotion", 
			//02FC9690,
			"PoETHighPotion", 
			//1F776C60,
			"ASMLHangManHighPot", 
			//0B09D570,
			"MegingjordHPotion", 
			//0C555FF0,
			"TheatreHPotion", 
			//21EE6D10,
			"DPoWSuperPotion", 
			//134CE560,
			"ASMLSuperPotion", 
			//0C05D210,
			"PotMMSuperPotion", 
			//1DE6CB60,
			"DPoWHeartRepair", 
			//12B56990,
			"TheatreHeartRepair", 
			//21D03C90,
			"ManaPrism",
			//250F8570, //Theatre
			"EntranceSerum", 
			//2044CB60,
			"HoSRSerum", 
			//02C297B0,
			"EntranceUncurse", 
			//20804560,
			"HoSRUncurse", 
			//02C29830,
			"MagicalTicket", 
			//208045E0,
			"CurtainTimeBell",  
			//233A5CE0,
			"HoSRNeapolitan", 
			//0319A080,
			"ASMLShortcake", 
			//0BBD3080,
			"HoSRRamen", 
			//08017400, //
			"WhiteTigerKey", 
			//089FB1E0, //HoSR
			"BlueDragonKey", 
			//124C2060, //DPoW
			"RedPhoenixKey", 
			//18D2EA60, //GFbT
			"BlackTurtleKey", 
			//24F26C60, //Theatre
			"YellowDragonKey", 
			//0A77A3E0, //ASML
			"AncientText2", 
			//2245EEE0, //Theatre
			"AncientText1", 
			//0BEFB3F0,
			"AncientText3", 
			//0C1BC8F0,
			"AncientText4", 
			//09D38F00,
			"Map1", 
			//2002D000, //Entrance
			"Map2", 
			//0D05C770, //ASML
			"Map3", 
			//0ECFB510, //DPoW
			"Map4", 
			//1599D480, //GFbT
			"Map5", 
			//238EC470, //Theatre
		//Event Items
			"ToolBag", 
			//14467A60,
			"ETablet", 
			//0B9E9C60,
			"VITablet", 
			//1EDC0BE0,
			"DragonCrest", 
			//1EF44160,
			"UnlockJewel", 
			//1EAE6B60,
			"Svarog Statue", 
			//209E5880,
			"WolfsFoot", 
			//0AB0E960,
			"SaiseiIncense", 
			//198E0B60,
			"BlackBishop", 
			//08CFE360,
			"LucifersSword", 
			//231FF460,
			"LittleHammer", 
			//1233E0E0,
			"MeditativeIncence", 
			//13337450, //
			"Draupnir", 
			//13A638E0,
			"AromaEarring", 
			//22B66E80,
			"RacoonCharm", 
			//11C532E0,
			"BloodyCape", 
			//08EA35E0,
			"RingofFire", 
			//1491E9E0,
			"ArticRing", 
			//0C34EAE0,
			"RingofThunder", 
			//18A1DF60,
			"HeartBrooch", 
			//22EB31E0,
			"JewelCrush", 
			//172BE1E0,
			"Megingjord", 
			//0C6C00E0,
			"Brisingamen", 
			//23539760,
		//Heart Max Up
			"ASMLHPHeartUp", 
			//0B240370,
			"ASMLElementalHeartUp", 
			//0B2403F0,
			"DPoWHeartUp1", 
			//11C53360, //1st?
			"DPoWHeartUp2", 
			//12197CE0, //2nd?
			"GFbTHeartUp", 
			//19092400,
			"PotMMHeartUp", 
			//1E1DDA60,
			"TheatreHeartUp", 
			//22D0CF60,
		//MP Max Up
			"HoSRMPMaxUp", 
			//088769E0,
			"ASMLMPMaxUp", 
			//0BD97A80,
			"DPoWMPMaxUp", 
			//12FD6ED0,
			"TheatreMPMaxUp", 
			//22646A10,
			"PotMMMPMaxUp", 
			//1E371160,
		//HP Max Up
			"HoSRHPMaxUp1", 
			//02C29730,
			"HoSRHPMaxUp2", 
			//086CF560,
			"ASMLHPMaxUp", 
			//0B3E7EE0,
			"DPoWHPMaxUpBF1", 
			//0F9B0880,
			"HoSRHPMaxUpBF2", 
			//12004460,
			"TheatreHPMaxUp1", 
			//2305A7E0,
			"TheatreHPMaxUp2", 
			//23761F00,
			"GFbTHPMaxUp", 
			//156654E0,
			//"PotMMDoppelHPMaxUp", //1C6B9510, //??? could cause problems
			"PotMMHPMaxUp", 
			//1EC79C60, //
		//$1000
			"HoSR1000", 
			//026F22D0, //replace with whip of lightning?
			"ASML1000", 
			//0AC7A7E0, //replace with whip of flames?
			"DPoW1000", 
			//134CE5E0, //replace with whip of ice?
		//$400
			"GFbT4001", 
			//15665560, //replace with red orb?
			"GFbT4002", 
			//16343290, //replace with blue orb?
			"GFbT4003", 
			//1653B400, //replace with yellow orb?
			"GFbT4004", 
			//17120200, //replace with green orb?
			"HoSR400", 
			//04B91A10, //replace with purple orb?   

			"ASML4001", 
			//09908580, //replace with white bishop?

			"ASML4002", 
			//0ADDC6F0, //replace with Sacrificial doll?
			"DPoW4001", 
			//12B56890, //replace with Jade Mask?

			"DPoW4002", 
			//12B56910, //replace with Diamond?
			
			"Theatre4001", 
			//23ED6C90, //replace with earth plate?
			"Theatre4002", 
			//240C9D80, //replace with meteor plate?
			"Theatre4003", 
			//242B7880, //replace with moonlight plate?
			"Theatre4004", 
			//24A24E80, //replace with solar plate?
		//repeatable
		//Torches
		//Knives
			"HoSRKnife", 
			//0708D580,
			"ASMLKnife", 
			//0DA607F0,
			"GFbTKnife", 
			//15FBCB00,
			"PotMMKnife", 
			//1A6882F0,
			"TheatreKnife", 
			//26237B70,
		//Axes
			"HoSRAxe", 
			//0335EA80,
			"ASMLAxe", 
			//0CED53F0,
			"DPoWFrostElementalAxe", 
			//10171D80,
			"DPoWBridgeLeverAxe", 
			//11034C70,
			"GFbTAxe", 
			//15827A70,
			"PotMMAxe", 
			//1A688370,
			"TheatreAxe", 
			//242B5610,
		//Holy Water
			"HoSRHolyWater", 
			//063C3EF0,
			"GFbTHolyWater",
			//17778C00,
			"PotMMHolyWater", 
			//1A6883F0,
			"TheatreHolyWater", 
			//240C7A90,
		//Crystal
			"HoSRCrystal", 
			//02DF7310,
			"ASMLCrystal", 
			//0D05AA70,
			"DPoWCrystal", 
			//114C3AF0,
			"GFbTCrystal", 
			//17EC3EF0,
			"PotMMCrystal", 
			//1A688470,
			"TheatreCrystal", 
			//265E8270,
		//Cross
			"HoSRCross", 
			//06703A00,
			"ASMLWhiteOrbCross", 
			//0D1A24F0,
			"ASML3FCross", 
			//0DE96970,
			"DPoWCross", 
			//11636670,
			"PotMMCross", 
			//1A6884F0,
			"$250Torch",
			//14E46170, //replace with black orb?
		//common drops
	};
	const int HoSRMapDesc = 0x01195388; 
	//const int endHoSRMapDesc = 0x011953A9; //Blue Orb
	const int ASMLMapDesc = 0x01195360; 
	//const int endASMLMapDesc = 0x01195381; //Red Orb
	const int DPoWMapDesc = 0x01195338; 
	//const int endDPoWMapDesc = 0x0119535B; //Green Orb
	const int GFbTMapDesc = 0x01195310; 
	//const int endGFbTMapDesc = 0x01195332; //Purple Orb
	const int TheatreMapDesc = 0x011952F0; 
	//const int endTheatreMapDesc = 0x01195309; //Yellow Orb

	const int AncientText1Desc = 0x011953F8; 
	//const int endAncientText1Desc = 0x0119541D; //Wolf's Foot
	const int AncientText2Desc = 0x011953B0; 
	//const int endAncientText2Desc = 0x011953F1; //Curtain Time Bell
	const int AncientText3Desc = 0x01195260; 
	//const int endAncientText3Desc = 0x0119529D; //White Orb
	const int AncientText4Desc = 0x01195210; 
	//const int endAncientText4Desc = 0x0119525A; //Black Orb
	
	//const int MarkerStone1Desc = 0x01194B90; 
	//const int endMarkerStone1Desc = 0x01194BDC; //Red Key
	//const int MarkerStone2Desc = 0x01194B40; 
	//const int endMarkerStone2Desc = 0x01194B8D; //Blue Key
	//const int MarkerStone3Desc = 0x01194AE8; 
	//const int endMarkerStone3Desc = 0x01194B37; //Yellow Key
	//const int MarkerStone4Desc = 0x01194A90; 
	//const int endMarkerStone4Desc = 0x01194ADE; //White Key
	//const int MarkerStone5Desc = 0x01194A40; 
	//const int endMarkerStone5Desc = 0x01194A8D; //Black Key
	//const int MarkerStone6Desc = 0x011949E8; 
	//const int endMarkerStone6Desc = 0x01194A36; //vi tablet
	//const int MarkerStone7Desc = 0x01194990; 
	//const int endMarkerStone7Desc = 0x011949E3; //dragon crest
	//const int MarkerStone8Desc = 0x01194938; 
	//const int endMarkerStone8Desc = 0x01194987; //e tablet
	
	//const int UnlockJewelDesc = 0x01195620; 
	//const int endUnlockJewelDesc = 0x01195662; //Vampire Killer
	//const int ETabletDesc = 0x011956A0; 
	//const int endETabletDesc = 0x011956C1; //Whip of Flames
	//const int CurtainTimeBellDesc = 0x011955D0; 
	//const int endCurtainTimeBellDesc = 0x01195618; //Whip of Ice
	const int DragonCrestDesc = 0x11954D8;
	//const int endDragonCrestDesc = 0x01195505;
	
	switch(ItemID)
	{
		case 0x05:
			fseek(fp,DragonCrestDesc,SEEK_SET);
			break;
		case 0x59:
			fseek(fp,TheatreMapDesc,SEEK_SET);
			break;
		case 0x5A:
			fseek(fp,GFbTMapDesc,SEEK_SET);
			break;
		case 0x5B:
			fseek(fp,ASMLMapDesc,SEEK_SET);
			break;
		case 0x5C:
			fseek(fp,DPoWMapDesc,SEEK_SET);
			break;
		case 0x5D:
			fseek(fp,HoSRMapDesc,SEEK_SET);
			break;
		case 0x5E:
			fseek(fp,AncientText1Desc,SEEK_SET);
			break;
		case 0x55:
			fseek(fp,AncientText3Desc,SEEK_SET);
			break;
		case 0x42:
			fseek(fp,AncientText2Desc,SEEK_SET);
			break;
		case 0x58:
			fseek(fp,AncientText4Desc,SEEK_SET);
			break;
		default:
			break;
	}
	for(int i = 0; itemNames[ItemID][i] != '\0'; i++)
	{
		fwrite(&itemNames[ItemID][i],sizeof(itemNames[ItemID][i]),1,fp);
	}
	newByte = 0x0A;
	fwrite(&newByte,sizeof(newByte),1,fp);
	//printf("addressNumber %d\n",addressNumber);
	if(addressNumber > locations_Length)
	{
		addressNumber -= locations_Length;
		//printf("addressNumber %d\n",addressNumber);
		if(addressNumber >= 65)
		{
			addressNumber -= 65;
			//printf("addressNumber %d\n",addressNumber);
		}
		for(int i = 0; enemies[addressNumber].name[i] != '\0'; i++)
		{
			fwrite(&enemies[addressNumber].name[i],sizeof(enemies[addressNumber].name[i]),1,fp);
		}
	}
	else
	{
		for(int i = 0; locations[addressNumber][i] != '\0'; i++)
		{
			fwrite(&locations[addressNumber][i],sizeof(locations[addressNumber][i]),1,fp);
		}
		newByte = 0x00;
		fwrite(&newByte,sizeof(newByte),1,fp);
	}
}
void hints_bosses(FILE* fp, char* bossName, int roomNumber)
{
	//const int HoSRMapDesc = 0x01195388; 
	//const int endHoSRMapDesc = 0x011953A9; //Blue Orb
	//const int ASMLMapDesc = 0x01195360; 
	//const int endASMLMapDesc = 0x01195381; //Red Orb
	//const int DPoWMapDesc = 0x01195338; 
	//const int endDPoWMapDesc = 0x0119535B; //Green Orb
	//const int GFbTMapDesc = 0x01195310; 
	//const int endGFbTMapDesc = 0x01195332; //Purple Orb
	//const int TheatreMapDesc = 0x011952F0; 
	//const int endTheatreMapDesc = 0x01195309; //Yellow Orb

	//const int AncientText1Desc = 0x011953F8; 
	//const int endAncientText1Desc = 0x0119541D; //Wolf's Foot
	//const int AncientText2Desc = 0x011953B0; 
	//const int endAncientText2Desc = 0x011953F1; //Curtain Time Bell
	//const int AncientText3Desc = 0x01195260; 
	//const int endAncientText3Desc = 0x0119529D; //White Orb
	//const int AncientText4Desc = 0x01195210; 
	//const int endAncientText4Desc = 0x0119525A; //Black Orb
	
	const int MarkerStone1Desc = 0x01194B90; 
	//const int endMarkerStone1Desc = 0x01194BDC; //Red Key
	const int MarkerStone2Desc = 0x01194B40; 
	//const int endMarkerStone2Desc = 0x01194B8D; //Blue Key
	const int MarkerStone3Desc = 0x01194AE8; 
	//const int endMarkerStone3Desc = 0x01194B37; //Yellow Key
	const int MarkerStone4Desc = 0x01194A90; 
	//const int endMarkerStone4Desc = 0x01194ADE; //White Key
	const int MarkerStone5Desc = 0x01194A40; 
	//const int endMarkerStone5Desc = 0x01194A8D; //Black Key
	const int MarkerStone6Desc = 0x011949E8; 
	//const int endMarkerStone6Desc = 0x01194A36; //vi tablet
	const int MarkerStone7Desc = 0x01194990; 
	//const int endMarkerStone7Desc = 0x011949E3; //dragon crest
	const int MarkerStone8Desc = 0x01194938; 
	//const int endMarkerStone8Desc = 0x01194987; //e tablet
	
	const int UnlockJewelDesc = 0x01195620; 
	//const int endUnlockJewelDesc = 0x01195662; //Vampire Killer
	//const int ETabletDesc = 0x011956A0; 
	//const int endETabletDesc = 0x011956C1; //Whip of Flames
	//const int CurtainTimeBellDesc = 0x011955D0; 
	//const int endCurtainTimeBellDesc = 0x01195618; //Whip of Ice
	//const int DragonCrestDesc = 0x11954D8;
	//const int endDragonCrestDesc = 0x01195505;
	
	char* location;
	switch(roomNumber)
	{
		case 0x00:
			fseek(fp,MarkerStone1Desc,SEEK_SET);
			for(int i = 0; bossName[i] != '\0'; i++)
			{
				fwrite(&bossName[i],sizeof(bossName[i]),1,fp);
			}
			newByte = '@';
			fwrite(&newByte,sizeof(newByte),1,fp);
			location = "UndeadParasite";
			for(int i = 0; location[i] != '\0'; i++)
			{
				fwrite(&location[i],sizeof(location[i]),1,fp);
			}
			newByte = 0x00;
			fwrite(&newByte,sizeof(newByte),1,fp);
			break;
		case 0x16:
			fseek(fp,MarkerStone2Desc,SEEK_SET);
			for(int i = 0; bossName[i] != '\0'; i++)
			{
				fwrite(&bossName[i],sizeof(bossName[i]),1,fp);
			}
			newByte = '@';
			fwrite(&newByte,sizeof(newByte),1,fp);
			location = "FlameElemental";
			for(int i = 0; location[i] != '\0'; i++)
			{
				fwrite(&location[i],sizeof(location[i]),1,fp);
			}
			newByte = 0x00;
			fwrite(&newByte,sizeof(newByte),1,fp);
			break;
		case 0x0E:
			fseek(fp,MarkerStone3Desc,SEEK_SET);
			for(int i = 0; bossName[i] != '\0'; i++)
			{
				fwrite(&bossName[i],sizeof(bossName[i]),1,fp);
			}
			newByte = '@';
			fwrite(&newByte,sizeof(newByte),1,fp);
			location = "Golem";
			for(int i = 0; location[i] != '\0'; i++)
			{
				fwrite(&location[i],sizeof(location[i]),1,fp);
			}
			newByte = 0x00;
			fwrite(&newByte,sizeof(newByte),1,fp);
			break;
		case 0x33:
			fseek(fp,MarkerStone4Desc,SEEK_SET);
			for(int i = 0; bossName[i] != '\0'; i++)
			{
				fwrite(&bossName[i],sizeof(bossName[i]),1,fp);
			}
			newByte = '@';
			fwrite(&newByte,sizeof(newByte),1,fp);
			location = "Joachim";
			for(int i = 0; location[i] != '\0'; i++)
			{
				fwrite(&location[i],sizeof(location[i]),1,fp);
			}
			newByte = 0x00;
			fwrite(&newByte,sizeof(newByte),1,fp);
			break;
		case 0x2C:
			fseek(fp,MarkerStone5Desc,SEEK_SET);
			for(int i = 0; bossName[i] != '\0'; i++)
			{
				fwrite(&bossName[i],sizeof(bossName[i]),1,fp);
			}
			newByte = '@';
			fwrite(&newByte,sizeof(newByte),1,fp);
			location = "FrostElemental";
			for(int i = 0; location[i] != '\0'; i++)
			{
				fwrite(&location[i],sizeof(location[i]),1,fp);
			}
			newByte = 0x00;
			fwrite(&newByte,sizeof(newByte),1,fp);
			break;
		case 0x37:
			fseek(fp,MarkerStone6Desc,SEEK_SET);
			for(int i = 0; bossName[i] != '\0'; i++)
			{
				fwrite(&bossName[i],sizeof(bossName[i]),1,fp);
			}
			newByte = '@';
			fwrite(&newByte,sizeof(newByte),1,fp);
			location = "Medusa";
			for(int i = 0; location[i] != '\0'; i++)
			{
				fwrite(&location[i],sizeof(location[i]),1,fp);
			}
			newByte = 0x00;
			fwrite(&newByte,sizeof(newByte),1,fp);
			break;
		case 0x3C:
			fseek(fp,MarkerStone7Desc,SEEK_SET);
			for(int i = 0; bossName[i] != '\0'; i++)
			{
				fwrite(&bossName[i],sizeof(bossName[i]),1,fp);
			}
			newByte = '@';
			fwrite(&newByte,sizeof(newByte),1,fp);
			location = "ThunderElemental";
			for(int i = 0; location[i] != '\0'; i++)
			{
				fwrite(&location[i],sizeof(location[i]),1,fp);
			}
			newByte = 0x00;
			fwrite(&newByte,sizeof(newByte),1,fp);
			break;
		case 0x12:
			fseek(fp,MarkerStone8Desc,SEEK_SET);
			for(int i = 0; bossName[i] != '\0'; i++)
			{
				fwrite(&bossName[i],sizeof(bossName[i]),1,fp);
			}
			newByte = '@';
			fwrite(&newByte,sizeof(newByte),1,fp);
			location = "Succubus";
			for(int i = 0; location[i] != '\0'; i++)
			{
				fwrite(&location[i],sizeof(location[i]),1,fp);
			}
			newByte = 0x00;
			fwrite(&newByte,sizeof(newByte),1,fp);
			break;
		case 0x03:
			fseek(fp,UnlockJewelDesc,SEEK_SET);
			for(int i = 0; bossName[i] != '\0'; i++)
			{
				fwrite(&bossName[i],sizeof(bossName[i]),1,fp);
			}
			newByte = '@';
			fwrite(&newByte,sizeof(newByte),1,fp);
			location = "ForgottenOne";
			for(int i = 0; location[i] != '\0'; i++)
			{
				fwrite(&location[i],sizeof(location[i]),1,fp);
			}
			newByte = 0x00;
			fwrite(&newByte,sizeof(newByte),1,fp);
			break;
		default:
			break;
	}
	
}
void setup_limit_power_ups(FILE* fp)
{
	//Limit Max HP to not exceed 300
	newByte = 0x96;
	fseek(fp,0x108604,SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	newByte = 0x43;
	fseek(fp,0x108605,SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	newByte = 0x00;
	fseek(fp,0x108609,SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	//Max HP decrement goes down to 1
	newByte = 0x80;
	fseek(fp,0x108618,SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	newByte = 0x3F;
	fseek(fp,0x108619,SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	
	//
	newByte = 0x96;
	fseek(fp,0x108774,SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	newByte = 0x43;
	fseek(fp,0x108775,SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	newByte = 0x00;
	fseek(fp,0x108779,SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	//
	newByte = 0x00;
	fseek(fp,0x108788,SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	newByte = 0x00;
	fseek(fp,0x108789,SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	
	//
	newByte = 0x12;
	fseek(fp,0x108C38,SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	newByte = 0x0C;
	fseek(fp,0x108C39,SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	newByte = 0x12;
	fseek(fp,0x108C4C,SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	newByte = 0x0C;
	fseek(fp,0x108C4D,SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	//
}
void setup_destroy_texts(FILE* fp)
{
	const int HoSRMapDesc = 0x01195388; 
	const int endHoSRMapDesc = 0x011953A9; //Blue Orb
	const int ASMLMapDesc = 0x01195360; 
	const int endASMLMapDesc = 0x01195381; //Red Orb
	const int DPoWMapDesc = 0x01195338; 
	const int endDPoWMapDesc = 0x0119535B; //Green Orb
	const int GFbTMapDesc = 0x01195310; 
	const int endGFbTMapDesc = 0x01195332; //Purple Orb
	const int TheatreMapDesc = 0x011952F0; 
	const int endTheatreMapDesc = 0x01195309; //Yellow Orb

	const int AncientText1Desc = 0x011953F8; 
	const int endAncientText1Desc = 0x0119541D; //Wolf's Foot
	const int AncientText2Desc = 0x011953B0; 
	const int endAncientText2Desc = 0x011953F1; //Curtain Time Bell
	const int AncientText3Desc = 0x01195260; 
	const int endAncientText3Desc = 0x0119529D; //White Orb
	const int AncientText4Desc = 0x01195210; 
	const int endAncientText4Desc = 0x0119525A; //Black Orb
	
	const int MarkerStone1Desc = 0x01194B90; 
	const int endMarkerStone1Desc = 0x01194BDC; //Red Key
	const int MarkerStone2Desc = 0x01194B40; 
	const int endMarkerStone2Desc = 0x01194B8D; //Blue Key
	const int MarkerStone3Desc = 0x01194AE8; 
	const int endMarkerStone3Desc = 0x01194B37; //Yellow Key
	const int MarkerStone4Desc = 0x01194A90; 
	const int endMarkerStone4Desc = 0x01194ADE; //White Key
	const int MarkerStone5Desc = 0x01194A40; 
	const int endMarkerStone5Desc = 0x01194A8D; //Black Key
	const int MarkerStone6Desc = 0x011949E8; 
	const int endMarkerStone6Desc = 0x01194A36; //vi tablet
	const int MarkerStone7Desc = 0x01194990; 
	const int endMarkerStone7Desc = 0x011949E3; //dragon crest
	const int MarkerStone8Desc = 0x01194938; 
	const int endMarkerStone8Desc = 0x01194987; //e tablet
	
	const int UnlockJewelDesc = 0x01195620; 
	const int endUnlockJewelDesc = 0x01195662; //Vampire Killer
	const int ETabletDesc = 0x011956A0; 
	const int endETabletDesc = 0x011956C1; //Whip of Flames
	const int CurtainTimeBellDesc = 0x011955D0; 
	const int endCurtainTimeBellDesc = 0x01195618; //Whip of Ice
	const int DragonCrestDesc = 0x11954D8;
	const int endDragonCrestDesc = 0x01195505;
	
	int hintArray[] = {
		HoSRMapDesc, endHoSRMapDesc,
		ASMLMapDesc, endASMLMapDesc, 
		DPoWMapDesc, endDPoWMapDesc,
		GFbTMapDesc, endGFbTMapDesc,
		TheatreMapDesc, endTheatreMapDesc, 

		AncientText1Desc, endAncientText1Desc,
		AncientText2Desc, endAncientText2Desc,
		AncientText3Desc, endAncientText3Desc,
		AncientText4Desc, endAncientText4Desc, 
		
		MarkerStone1Desc, endMarkerStone1Desc,
		MarkerStone2Desc, endMarkerStone2Desc, 
		MarkerStone3Desc, endMarkerStone3Desc,
		MarkerStone4Desc, endMarkerStone4Desc,
		MarkerStone5Desc, endMarkerStone5Desc,
		MarkerStone6Desc, endMarkerStone6Desc,
		MarkerStone7Desc, endMarkerStone7Desc,
		MarkerStone8Desc, endMarkerStone8Desc,
		
		UnlockJewelDesc, endUnlockJewelDesc,
		ETabletDesc, endETabletDesc, 
		CurtainTimeBellDesc, endCurtainTimeBellDesc,
		DragonCrestDesc, endDragonCrestDesc,
	};
	//clear text to be overwritten
	int hintArrayLength = *(&hintArray + 1) - hintArray;
	newByte = 0x20;
	for(int i = 0; i <= hintArrayLength - 2; i += 2)
	{
		for(int n = 0; n <= (hintArray[i+1] - hintArray[i]); n++)
		{
			fseek(fp,hintArray[i]+n,SEEK_SET);
			fwrite(&newByte,sizeof(newByte),1,fp);
		}
	}
}
void setup_enemies(FILE* fp) 
{	
	new_byte = 0x00;
	//remove all items from enemy drop table
	//set all drop rates to 0%
	//set all rosario drop rates to 0%
	for(int i = 0; i < enemies_Length; i++)
	{
		fseek(fp, enemies[i].common_item_address, SEEK_SET);
		fwrite(&new_byte,sizeof(new_byte),1,fp);
		fseek(fp, enemies[i].common_item_address+1, SEEK_SET);
		fwrite(&new_byte,sizeof(new_byte),1,fp);
		
		fseek(fp, enemies[i].rare_item_address, SEEK_SET);
		fwrite(&new_byte,sizeof(new_byte),1,fp);
		fseek(fp, enemies[i].rare_item_address+1, SEEK_SET);
		fwrite(&new_byte,sizeof(new_byte),1,fp);
		
		for(int j = 0; j < 4; j++)
		{
			fseek(fp, enemies[i].common_item_rate_address+j, SEEK_SET);
			fwrite(&new_byte,sizeof(new_byte),1,fp);
			
			fseek(fp, enemies[i].rare_item_rate_address+j, SEEK_SET);
			fwrite(&new_byte,sizeof(new_byte),1,fp);
			
			fseek(fp, enemies[i].rosario_address+j, SEEK_SET);
			fwrite(&new_byte,sizeof(new_byte),1,fp);
		}
	}
	
	/*
	int SIZE = 1;
	unsigned char buffer[SIZE];
	int n;
	fseek(fp,0x6F0850,SEEK_SET);
	n  = fread(buffer,sizeof(buffer),1,fp);
	printf("%x \n",buffer[0]);
	*/
	
	//remove all tolerances/weaknesses
	//int tolerance_value = 0xA041; //correct order to write
	//int weakness_value = 0x48C2; //correct order to write
}
int items[] = {
	0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x84, 0x2A,
	0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2B, 0x2B,
	0x2B, 0x2B, 0x2B, 0x2C, 0x2C, 0x2C, 0x2D, 0x2D, 0x2E, 0x2F,
	0x2F, 0x30, 0x30, 0x31, 0x42, 0x4C, 0x48, 0x59, 0x5A, 0x5B,
	0x5C, 0x5D, 0x62, 0x61, 0x72, 0x73, 0x63, 0x64, 0x65, 0x66,
	0x67, 0x5F, 0x55, 0x56, 0x5E, 0x8E, 0x86, 0x87, 0x8B, 0x8D,
	0x8F, 0x0A, 0x0B, 0x0E, 0x0F, 0x13, 0x14, 0x15, 0x17, 0x1D,
	0x1E, 0x20, 0x91, 0x91, 0x91, 0x91, 0x91, 0x91, 0x91, 0x92,
	0x92, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x99, 0x99,
	0x99, 0x2A, 0x99, 0x2D, 0x2A, 0x2E, 0x2A, 0x2A, 0x2D, 0x2D,
	0x2A, 0x3D, 0x2A, 0x2F, 0x47, 0x2A, 0x2F, 0x2A, 0x43, 0x2E,
	0x43, 0x2F, 0x3E, 0x2A, 0x2E, 0x40, 0x2A, 0x30, 0x2B, 0x2D,
	0x2D, 0x32, 0x3F, 0x2B, 0x31, 0x41, 0xA2, 0x99, 0x4B, 0x44,
	0xA3, 0x11, 0x49, 0x49, 0x1B, 0x21, 0xA3, 0x12, 0x0D, 0x4A,
	0x1F, 0x0B, 0x1C, 0x19, 0x89, 0x16, 0x46, 0x8A, 0x46, 0x2A,
	0x2B, 0x05, 0x06, 0x07, 0x08, 0x09, 0x18, 0x1A, 0x22, 0x33,
	0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D,
	0x3E, 0x3F, 0x40, 0x41, 0x60, 0x8C, 0x98, 0x98, 0x98, 0x98,
	0x98, 0x98, 0x98, 0x98, 0x98, 0x98, 0x98, 0x98, 0x98, 0xAA,
	0xAA, 0xAA, 0xAA, 0xAA, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB,
	0xAB, 0xAC, 0xAC, 0xAC, 0xAC, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD,
	0xAD, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0x97, 0x95, 0x95, 0x95,
	0x95, 0x95, 0x95, 0x95, 0xA5, 0x58, 0x0C, 
	//23x10= 230 - 3 = 227 items [0 - 226]
};
int items_Length = *(&items + 1) - items;
int extra_items[] = {
	0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x55, 0x42, 0x05, 0x86, 0x8E, 0x02, 0x03, 0x57, 
	0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x55, 0x42, 0x05, 0x86, 0x8E, 0x02, 0x03, 0x57, 
};
int orbs_and_whips_no_bosses[] = {
	0x02, 0x03, 0x04, 0x05, 0x7F, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85,
};
int* setup_what_items(FILE* fp)
{
    int size = 247;
    int* item_list = (int*)malloc(size * sizeof(int)); // Dynamically allocate memory
    if (item_list == NULL) {
        printf("Memory allocation failed\n");
        return NULL; // Return NULL if memory allocation fails
    }

    // Initialize the array with values (or read them from the file if needed)
    int items[] = {
        0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x84, 0x2A,
        0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2B, 0x2B,
        0x2B, 0x2B, 0x2B, 0x2C, 0x2C, 0x2C, 0x2D, 0x2D, 0x2E, 0x2F,
        0x2F, 0x30, 0x30, 0x31, 0x42, 0x4C, 0x48, 0x59, 0x5A, 0x5B,
        0x5C, 0x5D, 0x62, 0x61, 0x72, 0x73, 0x63, 0x64, 0x65, 0x66,
        0x67, 0x5F, 0x55, 0x56, 0x5E, 0x8E, 0x86, 0x87, 0x8B, 0x8D,
        0x8F, 0x0A, 0x0B, 0x0E, 0x0F, 0x13, 0x14, 0x15, 0x17, 0x1D,
        0x1E, 0x20, 0x91, 0x91, 0x91, 0x91, 0x91, 0x91, 0x91, 0x92,
        0x92, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x99, 0x99,
        0x99, 0x2A, 0x99, 0x2D, 0x2A, 0x2E, 0x2A, 0x2A, 0x2D, 0x2D,
        0x2A, 0x3D, 0x2A, 0x2F, 0x47, 0x2A, 0x2F, 0x2A, 0x43, 0x2E,
        0x43, 0x2F, 0x3E, 0x2A, 0x2E, 0x40, 0x2A, 0x30, 0x2B, 0x2D,
        0x2D, 0x32, 0x3F, 0x2B, 0x31, 0x41, 0xA2, 0x99, 0x4B, 0x44,
        0xA3, 0x11, 0x49, 0x49, 0x1B, 0x21, 0xA3, 0x12, 0x0D, 0x4A,
        0x1F, 0x0B, 0x1C, 0x19, 0x89, 0x16, 0x46, 0x8A, 0x46, 0x2A,
        0x2B, 0x05, 0x06, 0x07, 0x08, 0x09, 0x18, 0x1A, 0x22, 0x33,
        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D,
        0x3E, 0x3F, 0x40, 0x41, 0x60, 0x8C, 0x98, 0x98, 0x98, 0x98,
        0x98, 0x98, 0x98, 0x98, 0x98, 0x98, 0x98, 0x98, 0x98, 0xAA,
        0xAA, 0xAA, 0xAA, 0xAA, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB,
        0xAB, 0xAC, 0xAC, 0xAC, 0xAC, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD,
        0xAD, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0x97, 0x0C,
    };

    // Copy values into the dynamically allocated memory
    for (int i = 0; i < size; i++) {
        item_list[i] = items[i]; // Populate the item_list with values
    }

    return item_list; // Return the pointer to the dynamically allocated array
}
void setup_destroy_onGround(FILE* fp)
{
	int locations[] =
	{
		//non-Repeatable
			0x20259A60, //Entrance
			0x2044CAE0, //Entrance
			0x2002CF80, //Entrance
			0x09A6A980, //ASML
			0x1745D4E0, //GFbT
			0x05323DE0, //HoSR
			0x23ED6C10, //Theatre
			0x117ECB60, //DPoW
			0x0A0C6B60, //ASML
			//EntrancePotion = 
			0x20259AE0,
			// GFbTPotion = 
			0x14AB1380,
			// DPoWPotion = 
			0x10BD2300,
			// ASMLFlameElementalPotion = 
			0x0B240470,
			// ASMLMegingjordPotion = 
			0x0C556070,
			// HoSR1stPotion = 
			0x0484E010,
			// HoSR2ndPotion = 
			0x049E4D80,
			// Theatre1stPotion = 
			0x23CE4480,
			// Theatre2ndPotion = 
			0x244AE000,
			// HoSRHighPotion = 
			0x02FC9690,
			// PoETHighPotion = 
			0x1F776C60,
			// ASMLHangedManHighPotion = 
			0x0B09D570,
			// ASMLMegingjordHighPotion = 
			0x0C555FF0,
			// TheatreHighPotion = 
			0x21EE6D10,
			// DPoWSuperPotion = 
			0x134CE560,
			// ASMLSuperPotion = 
			0x0C05D210,
			// PotMMSuperPotion = 
			0x1DE6CB60,
			// DPoWHeartRepair = 
			0x12B56990,
			// TheatreHeartRepair = 
			0x21D03C90,
			0x250F8570, //Theatre
			// EntranceSerum = 
			0x2044CB60,
			// HoSRSerum = 
			0x02C297B0,
			// EntranceUncursePotion = 
			0x20804560,
			// HoSRUncursePotion = 
			0x02C29830,
		//Magical Ticket
			0x208045E0,
		//Curtain Time Bell  
			0x233A5CE0,
			// HoSRNeapolitan = 
			0x0319A080,
			// ASMLShortcake = 
			0x0BBD3080,
			// HoSRRamen = 
			0x08017400, //
			// WhiteTigerKey = 
			0x089FB1E0, //HoSR
			// BlueDragonKey = 
			0x124C2060, //DPoW
			// RedPhoenixKey = 
			0x18D2EA60, //GFbT
			// BlackTurtleKey = 
			0x24F26C60, //Theatre
			// YellowDragonKey = 
			0x0A77A3E0, //ASML
			// AncientText2 = 
			0x2245EEE0, //Theatre
			// AncientText1 = 
			0x0BEFB3F0,
			// AncientText3 = 
			0x0C1BC8F0,
			// AncientText4 = 
			0x09D38F00,
			// Map1 = 
			0x2002D000, //Entrance
			// Map2 = 
			0x0D05C770, //ASML
			// Map3 = 
			0x0ECFB510, //DPoW
			// Map4 = 
			0x1599D480, //GFbT
			// Map5 = 
			0x238EC470, //Theatre
		//Event Items
			// ToolBag = 
			0x14467A60,
			// ETablet = 
			0x0B9E9C60,
			// VITablet = 
			0x1EDC0BE0,
			// DragonCrest = 
			0x1EF44160,
			// UnlockJewel = 
			0x1EAE6B60,
			// Svarog Statue = 
			0x209E5880,
			// WolfsFoot = 
			0x0AB0E960,
			// SaiseiIncense = 
			0x198E0B60,
			// BlackBishop = 
			0x08CFE360,
			// LucifersSword = 
			0x231FF460,
			// LittleHammer = 
			0x1233E0E0,
			// MeditativeIncence = 
			0x13337450, //
			// Draupnir = 
			0x13A638E0,
			// AromaEarring = 
			0x22B66E80,
			// RacoonCharm = 
			0x11C532E0,
			// BloodyCape = 
			0x08EA35E0,
			// RingofFire = 
			0x1491E9E0,
			// ArticRing = 
			0x0C34EAE0,
			// RingofThunder = 
			0x18A1DF60,
			// HeartBrooch = 
			0x22EB31E0,
			// JewelCrush = 
			0x172BE1E0,
			// Megingjord = 
			0x0C6C00E0,
			// Brisingamen = 
			0x23539760,
		//Heart Max Up
			// ASMLHPHeartUp = 
			0x0B240370,
			// ASMLFlameElementalHeartUp = 
			0x0B2403F0,
			// DPoWHeartUp1 = 
			0x11C53360, //1st?
			// DPoWHeartUp2 = 
			0x12197CE0, //2nd?
			// GFbTHeartUp = 
			0x19092400,
			// PotMMHeartUp = 
			0x1E1DDA60,
			// TheatreHeartUp = 
			0x22D0CF60,
		//MP Max Up
			// HoSRMPMaxUp = 
			0x088769E0,
			// ASMLMPMaxUp = 
			0x0BD97A80,
			// DPoWMPMaxUp = 
			0x12FD6ED0,
			// TheatreMPMaxUp = 
			0x22646A10,
			// PotMMMPMaxUp = 
			0x1E371160,
		//HP Max Up
			// HoSRHPMaxUp1 = 
			0x02C29730,
			// HoSRHPMaxUp2 = 
			0x086CF560,
			// ASMLHPMaxUp = 
			0x0B3E7EE0,
			// DPoWHPMaxUpBF1 = 
			0x0F9B0880,
			// HoSRHPMaxUpBF2 = 
			0x12004460,
			// TheatreHPMaxUp1 = 
			0x2305A7E0,
			// TheatreHPMaxUp2 = 
			0x23761F00,
			// GFbTHPMaxUp = 
			0x156654E0,
			//// PotMMDoppelHPMaxUp = 0x1C6B9510, //??? could cause problems
			// PotMMHPMaxUp = 
			0x1EC79C60, //
		//$1000
			// HoSR1000 = 
			0x026F22D0, //replace with whip of lightning?
			// ASML1000 = 
			0x0AC7A7E0, //replace with whip of flames?
			// DPoW1000 = 
			0x134CE5E0, //replace with whip of ice?
		//$400
			// GFbT4001 = 
			0x15665560, //replace with red orb?
			// GFbT4002 = 
			0x16343290, //replace with blue orb?
			// GFbT4003 = 
			0x1653B400, //replace with yellow orb?
			// GFbT4004 = 
			0x17120200, //replace with green orb?
			// HoSR400 = 
			0x04B91A10, //replace with purple orb?   

			// ASML4001 = 
			0x09908580, //replace with white bishop?

			// ASML4002 = 
			0x0ADDC6F0, //replace with Sacrificial doll?
			// DPoW4001 = 
			0x12B56890, //replace with Jade Mask?

			// DPoW4002 = 
			0x12B56910, //replace with Diamond?
			
			// Theatre4001 = 
			0x23ED6C90, //replace with earth plate?
			// Theatre4002 = 
			0x240C9D80, //replace with meteor plate?
			// Theatre4003 = 
			0x242B7880, //replace with moonlight plate?
			// Theatre4004 = 
			0x24A24E80, //replace with solar plate?
		//repeatable
		//Torches
		//Knives
			// HoSRKnife = 
			0x0708D580,
			// ASMLKnife = 
			0x0DA607F0,
			// GFbTKnife = 
			0x15FBCB00,
			// PotMMKnife = 
			0x1A6882F0,
			// TheatreKnife = 
			0x26237B70,
		//Axes
			// HoSRAxe = 
			0x0335EA80,
			// ASMLAxe = 
			0x0CED53F0,
			// DPoWFrostElementalAxe = 
			0x10171D80,
			// DPoWBridgeLeverAxe = 
			0x11034C70,
			// GFbTAxe = 
			0x15827A70,
			// PotMMAxe = 
			0x1A688370,
			// TheatreAxe = 
			0x242B5610,
		//Holy Water
			// HoSRHolyWater = 
			0x063C3EF0,
			// GFbTHolyWater =
			0x17778C00,
			// PotMMHolyWater = 
			0x1A6883F0,
			// TheatreHolyWater = 
			0x240C7A90,
		//Crystal
			// HoSRCrystal = 
			0x02DF7310,
			// ASMLCrystal = 
			0x0D05AA70,
			// DPoWCrystal = 
			0x114C3AF0,
			// GFbTCrystal = 
			0x17EC3EF0,
			// PotMMCrystal = 
			0x1A688470,
			// TheatreCrystal = 
			0x265E8270,
		//Cross
			// HoSRCross = 
			0x06703A00,
			// ASMLWhiteOrbCross = 
			0x0D1A24F0,
			// ASML3FCross = 
			0x0DE96970,
			// DPoWCross = 
			0x11636670,
			// PotMMCross = 
			0x1A6884F0,
		//$250
			0x14E46170, //replace with black orb?
	};
	int location_Length = *(&locations + 1) - locations;
	for(int i = 0; i < location_Length; i++)
	{
		fseek(fp,locations[i],SEEK_SET);
		new_byte = 0x00;
		fwrite(&new_byte,sizeof(new_byte),1,fp);
		
		fseek(fp,locations[i]+4,SEEK_SET);
		new_byte = 0x02;
		fwrite(&new_byte,sizeof(new_byte),1,fp);
	}
}
//setup caller
void call_setup(FILE* fp)
{
	setup_loadzone_changes(fp); //REQUIRE
	setup_change_sprites(fp); //REQUIRE
	setup_destroy_shop(fp); //
	if(DEBUG)
	{
		unsigned char buffer[1];
		fseek(fp,0x3DAC50,SEEK_SET);
		fread(buffer,sizeof(buffer),1,fp);
		if(buffer[0] != 0x32)
		{
			printf("An Error has Occured! \n");
			printf("0x3DAC50 has been overwritten \n");
			printf("current is 0x%x\n",buffer[0]);
		}
	}
	setup_destroy_joachim_mode(fp); //REQUIRE
	setup_destroy_pumpkin_mode(fp); //REQUIRE
	setup_enemies(fp); //REQUIRE
		//setup_destroy_enemy(fp);
	int* item_list = setup_what_items(fp);
	//setup_what_locations(fp);
	setup_destroy_onGround(fp); //REQUIRE
	setup_destroy_texts(fp); //
	setup_limit_power_ups(fp); //REQUIRE
	setup_trap_items_or_fake_items(fp); //REQUIRE
	
	free(item_list);
}

//randomization
void randomize_area_locking(FILE* fp)
{	
	int BossEntranceAddress[] = {
		0x82C30F0, //AND 0x82C3070 //UndeadParasite
		0xB147570, //FlameElemental
		0xD5410F0, //Golem
		0x13551370, //Joachim
		0x133F0270, //FrostElemental
		0x19B5A1F0, //Medusa
		0x1A05E870, //Thunder Elemental
		0x237F94F0, //Succubus
		0x82C3070,
		//0x1F51DD70 //ForgottenOne
	};
	int BossDataLength = *(&BossEntranceAddress + 1) - BossEntranceAddress;
	
	int AreaFirstRoomDoors[] = {
		0x0913B585, //ASML (East) -	00 00 00 00 00 24 00 01
		0x0913B605, //ASML (West) - 00 01 00 00 00 2A 00 01
		0x1450D485, //GFbT - 		00 00 00 00 00 0D 00 03
		0x02906105, //HoSR - 		00 00 00 00 00 2D 00 00
		0x0E28B585, //DPoW - 		00 00 00 00 00 11 00 02
		0x21A68F05, //Theatre - 	00 00 00 00 00 21 00 08
	};
	
	//Lock the doors
	int n = 0; 
	unsigned char newByte = 0x03;
	fseek(fp, AreaFirstRoomDoors[n], SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	fseek(fp, AreaFirstRoomDoors[n+1], SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	fseek(fp, BossEntranceAddress[n+1]+0x15, SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	fseek(fp, BossEntranceAddress[n+2]+0x15, SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	newByte = 0x02;
	fseek(fp, AreaFirstRoomDoors[n+2], SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	fseek(fp, BossEntranceAddress[n+5]+0x15, SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	fseek(fp, BossEntranceAddress[n+6]+0x15, SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	newByte = 0x05;
	fseek(fp, AreaFirstRoomDoors[n+3], SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	fseek(fp, BossEntranceAddress[n+0]+0x15, SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	fseek(fp, BossEntranceAddress[n+8]+0x15, SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	newByte = 0x04;
	fseek(fp, AreaFirstRoomDoors[n+4], SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	fseek(fp, BossEntranceAddress[n+3]+0x15, SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	fseek(fp, BossEntranceAddress[n+4]+0x15, SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	newByte = 0x01;
	fseek(fp, AreaFirstRoomDoors[n+5], SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	fseek(fp, BossEntranceAddress[n+7]+0x15, SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	
	//Unlock 1 area:
	
	int randVal = rand() % 5;
	newByte = 0x00;
	switch(randVal)
	{
		case 0: //ASML
			fseek(fp, AreaFirstRoomDoors[n], SEEK_SET);
			fwrite(&newByte,sizeof(newByte),1,fp);
			fseek(fp, AreaFirstRoomDoors[n+1], SEEK_SET);
			//fwrite(&newByte,sizeof(newByte),1,fp);
			//fseek(fp, BossEntranceAddress[n+1]+0x15, SEEK_SET);
			//fwrite(&newByte,sizeof(newByte),1,fp);
			//fseek(fp, BossEntranceAddress[n+2]+0x15, SEEK_SET);
			//fwrite(&newByte,sizeof(newByte),1,fp);
			break;
		case 1: //GFbT
			fseek(fp, AreaFirstRoomDoors[n+2], SEEK_SET);
			fwrite(&newByte,sizeof(newByte),1,fp);
			//fseek(fp, BossEntranceAddress[n+5]+0x15, SEEK_SET);
			//fwrite(&newByte,sizeof(newByte),1,fp);
			//fseek(fp, BossEntranceAddress[n+6]+0x15, SEEK_SET);
			//fwrite(&newByte,sizeof(newByte),1,fp);
			break;
		case 2: //HoSR
			fseek(fp, AreaFirstRoomDoors[n+3], SEEK_SET);
			fwrite(&newByte,sizeof(newByte),1,fp);
			//fseek(fp, BossEntranceAddress[n+0]+0x15, SEEK_SET);
			//fwrite(&newByte,sizeof(newByte),1,fp);
			//fseek(fp, BossEntranceAddress[n+8]+0x15, SEEK_SET);
			//fwrite(&newByte,sizeof(newByte),1,fp);
			break;
		case 3: //DPoW
			fseek(fp, AreaFirstRoomDoors[n+4], SEEK_SET);
			fwrite(&newByte,sizeof(newByte),1,fp);
			//fseek(fp, BossEntranceAddress[n+3]+0x15, SEEK_SET);
			//fwrite(&newByte,sizeof(newByte),1,fp);
			//fseek(fp, BossEntranceAddress[n+4]+0x15, SEEK_SET);
			//fwrite(&newByte,sizeof(newByte),1,fp);
			break;
		case 4: //Theatre
			fseek(fp, AreaFirstRoomDoors[n+5], SEEK_SET);
			fwrite(&newByte,sizeof(newByte),1,fp);
			//fseek(fp, BossEntranceAddress[n+7]+0x15, SEEK_SET);
			//fwrite(&newByte,sizeof(newByte),1,fp);
			break;
		default:
			break;
	}
		
	//place keys:
	
	//TESTING:
	 int DPoW_torches[] =  		
	 {
		 0x10171D00, //Black = 0x5C;
	 };
	 int GFbT_torches[] =		
	 {
		 0x158279F0, //Blue = 0x5A;
	 };
	 int ASML_torches[] =
	 {
		 0xD1A2470, //Red = 0x5B;
	 };
	 int HoSR_torches[] = 		
	 {
		 //0x6703980, //Yellow = 0x5D;
		 0x2DF7290, 
	 };
	 int Theatre_torches[] =  	
	 {
		 0x26237AF0, //White = 0x59;
	 };	
	int keys[] = {
		0x5B,0x5A,0x5D,0x5C,0x59
	};		
		
	// Torch addresses aligned to same order
	int torches[5] = {
		ASML_torches[0], GFbT_torches[0], HoSR_torches[0], DPoW_torches[0], Theatre_torches[0]
	};	
	
	torches[0] = ASML_torches[rand() % (sizeof(ASML_torches)/sizeof(ASML_torches[0]))];
	torches[1] = GFbT_torches[rand() % (sizeof(GFbT_torches)/sizeof(GFbT_torches[0]))];
	torches[2] = HoSR_torches[rand() % (sizeof(HoSR_torches)/sizeof(HoSR_torches[0]))];
	torches[3] = DPoW_torches[rand() % (sizeof(DPoW_torches)/sizeof(DPoW_torches[0]))];
	torches[4] = Theatre_torches[rand() % (sizeof(Theatre_torches)/sizeof(Theatre_torches[0]))];
	
	// 1. Create an array of indices 0–4
	int order[5] = {0,1,2,3,4};

	// 2. Shuffle (Fisher–Yates)
	for (int i = 4; i > 0; i--)
	{
		int j = rand() % (i + 1);
		int temp = order[i];
		order[i] = order[j];
		order[j] = temp;
	}

	// 3. Enforce the 5-cycle by shifting mapping by 1 (Code Generated by you that says it does what you seem to be saying is a problem?)
	//    area order[i] gets key from order[(i+1) % 5]
	for (int pos = 0; pos < 5; pos++)
	{
		int area_index = order[pos];
		int next_index = order[(pos + 1) % 5];

		unsigned char key_to_write = keys[next_index];

		fseek(fp, torches[area_index], SEEK_SET);
		fwrite(&key_to_write, 1, 1, fp);
	}
}

int torches[] = {
		0x0226B830,
		0x0226B8B0,
		0x0226B930,
		0x0226B9B0,
		0x0226BA30,
		0x0226BAB0,
		0x0282C990,
		0x0282CA10,
		0x0282CA90,
		0x0282CB10,
		0x0282CB90,
		0x0282CC10,
		0x02A3B030,
		0x02A3B0B0,
		0x02A3B130,
		0x02A3B1B0,
		0x02A3B230,
		0x02A3B2B0,
		0x02C27610,
		0x02C27690,
		0x02C27710,
		0x02C27790,
		0x02C27810,
		0x02C27890,
		0x02DF7190,
		0x02DF7210,
		0x02DF7290,
		0x02DF7390,
		0x02FC7700,
		0x02FC7780,
		0x02FC7800,
		0x02FC7880,
		0x02FC7900,
		0x02FC7980,
		0x02FC7A00,
		
		0x03197E80,
		0x03197F00,
		0x03197F80,
		0x03198000,
		0x03198080,
		0x0335E880,
		0x0335E900,
		0x0335E980,
		0x0335EA00,
		0x03533A80,
		0x03533B00,
		0x03533B80,
		0x03533C00,
		0x03533C80,
		0x038DE070,
		0x03AD55F0,
		0x03C765F0,
		0x03C76670,
		
		0x0401BCF0,
		0x0401BD70,
		0x0401BDF0,
		0x041D4B60,
		0x041D4BE0,
		0x043CDA70,
		0x043CDAF0,
		0x0484C4C0,
		0x0484C540,
		0x0484C5C0,
		0x0484C640,
		0x0484C6C0,
		0x049E3530,
		0x049E35B0,
		0x049E3630,
		0x049E3730,
		0x04B901B0,
		0x04B90230,
		0x04B902B0,
		0x04D06000,
		0x04D06080,
		0x04D06100,
		0x04D06180,
		0x04E9CF80,
		0x04E9D000,
		0x04E9D080,
		0x04E9D100,
		0x04E9D180,
		0x04E9D200,
		
		0x0502B000,
		0x0502B080,
		0x0502B100,
		0x0502B180,
		0x0502B200,
		0x0502B280,
		0x05D23080,
		0x05D23100,
		0x05D23180,
		0x05D23200,
		0x05D23280,
		0x05D23300,
		0x05D23380,
		0x05D23400,
		0x05E7BF60,
		0x05E7BFE0,
		0x05E7C060,
		0x05E7C0E0,
		0x05E7C160,
		0x05E7C1E0,
		0x05E7C260,
		0x05FD5800,
		0x05FD5880,
		0x05FD5900,
		0x05FD5980,
		0x05FD5A00,
		0x05FD5A80,
		0x05FD5B00,
		
		0x0612FA00,
		0x0612FA80,
		0x0612FB00,
		0x0612FB80,
		0x0612FC00,
		0x0612FC80,
		0x0612FD00,
		0x063C3CF0,
		0x063C3D70,
		0x063C3DF0,
		0x063C3E70,
		0x063C3F70,
		0x063C3FF0,
		0x063C4070,
		0x06564900,
		0x06564980,
		0x06564A00,
		0x06564A80,
		0x06564B00,
		0x06564B80,
		0x06564C00,
		0x06703900,
		0x06703980,
		0x06703A80,
		0x06703B00,
		0x06703B80,
		0x06703C00,
		0x068FB2E0,
		0x068FB360,
		0x068FB3E0,
		0x068FB460,
		0x068FB4E0,
		0x068FB560,
		0x068FB5E0,
		0x06A52A70,
		0x06A52AF0,
		0x06A52B70,
		0x06A52BF0,
		0x06A52C70,
		0x06A52CF0,
		0x06A52D70,
		0x06A52DF0,
		0x06B950F0,
		0x06B95170,
		0x06B951F0,
		0x06B95270,
		0x06B952F0,
		0x06B95370,
		0x06B953F0,
		0x06B95470,
		0x06D07E70,
		0x06D07EF0,
		0x06D07F70,
		0x06D07FF0,
		0x06D08070,
		0x06D080F0,
		0x06D08170,
		0x06ED50F0,
		0x06ED5170,
		0x06ED51F0,
		0x06ED5270,
		0x06ED52F0,
		0x06ED5370,
		0x06ED53F0,
		
		0x0708D500,
		0x0708D600,
		0x0708D680,
		0x0708D700,
		0x0708D780,
		0x0708D800,
		0x07255700,
		0x07255780,
		0x07255800,
		0x07255880,
		0x07255900,
		0x07255980,
		0x07255A00,
		0x074256F0,
		0x07425770,
		0x074257F0,
		0x07425870,
		0x074258F0,
		0x07425970,
		0x074259F0,
		0x07563B70,
		0x07563BF0,
		0x07563C70,
		0x07563CF0,
		0x07563D70,
		0x076AA480,
		0x076AA500,
		0x076AA580,
		0x076AA600,
		0x076AA680,
		0x077F2B70,
		0x077F2BF0,
		0x077F2C70,
		0x077F2CF0,
		0x077F2D70,
		0x07942A00,
		0x07942A80,
		0x07942B00,
		0x07942B80,
		0x07942C00,
		0x07A9C300,
		0x07A9C380,
		0x07A9C400,
		0x07A9C480,
		0x07A9C500,
		0x07BF2980,
		0x07BF2A00,
		0x07BF2A80,
		0x07BF2B00,
		0x07BF2B80,
		0x07D49380,
		0x07D49400,
		0x07D49480,
		0x07D49500,
		0x07D49580,
		0x07EAF580,
		0x07EAF600,
		0x07EAF680,
		0x07EAF700,
		0x07EAF780,
		
		0x08015E80,
		0x08015F00,
		0x08015F80,
		0x08016000,
		0x08016080,
		0x081CC3E0,
		0x081CC460,
		0x081CC4E0,
		0x081CC560,
		0x081CC5E0,
		0x081CC660,
		0x083C2FF0,
		0x083C3070,
		0x083C30F0,
		0x083C3170,
		0x083C31F0,
		0x083C3270,
		0x083C32F0,
		0x08529680,
		0x08529700,
		0x08529780,
		0x08529800,
		0x08529880,
		0x08B56780,
		0x08B56800,
		0x08B56880,
		0x08B56900,
		
		0x09348040,
		0x0956D370,
		0x0956D3F0,
		0x0956D470,
		0x0956D4F0,
		0x0979B8F0,
		0x0979B970,
		0x0979B9F0,
		0x0979BA70,
		0x09906FC0,
		0x09907040,
		0x09A694C0,
		0x09A69540,
		0x09BD79C0,
		0x09BD7A40,
		0x09BD7AC0,
		0x09BD7B40,
		0x09D37AC0,
		0x09D37B40,
		0x09F5DFF0,
		0x09F5E070,
		0x09F5E0F0,
		0x09F5E170,
		
		0x0A2FC700,
		0x0A2FC780,
		0x0A2FC800,
		0x0A2FC880,
		0x0A5F43C0,
		0x0A5F4440,
		0x0A5F44C0,
		0x0A5F4540,
		0x0ADDBC80,
		0x0ADDBD00,
		0x0AF3E670,
		0x0AF3E6F0,
		0x0AF3E770,
		0x0AF3E7F0,
		
		0x0B09CC80,
		0x0B09CD00,
		0x0B23ECC0,
		0x0B23ED40,
		0x0B23EE40,
		0x0B23EEC0,
		0x0B701270,
		0x0B7012F0,
		0x0B701370,
		0x0B7013F0,
		0x0B87FFD0,
		0x0B880050,
		0x0BD96440,
		0x0BD964C0,
		0x0BEF9DC0,
		0x0BEF9E40,
		
		0x0C05BCC0,
		0x0C05BD40,
		0x0C1BB440,
		0x0C1BB4C0,
		0x0C8E91F0,
		0x0C8E9270,
		0x0C8E92F0,
		0x0C8E9370,
		0x0CA5DCC0,
		0x0CA5DD40,
		0x0CA5DDC0,
		0x0CA5DE40,
		0x0CBFF1F0,
		0x0CBFF270,
		0x0CBFF2F0,
		0x0CBFF370,
		0x0CBFF3F0,
		0x0CD8E870,
		0x0CD8E8F0,
		0x0CED5370,
		0x0CED5470,
		0x0CED54F0,
		0x0CED5570,
		0x0CED55F0,
		
		0x0D05A970,
		0x0D05A9F0,
		0x0D05AAF0,
		0x0D1A2470,
		0x0D1A2570,
		0x0D1A25F0,
		0x0D329DF0,
		0x0D329E70,
		0x0D329EF0,
		0x0D329F70,
		0x0D4CA7F0,
		0x0D4CA870,
		0x0D4CA8F0,
		0x0D4CA970,
		0x0D629AF0,
		0x0D629B70,
		0x0D629BF0,
		0x0D629C70,
		0x0D629CF0,
		0x0D629D70,
		0x0D781B70,
		0x0D781BF0,
		0x0D781C70,
		0x0D781CF0,
		0x0D781D70,
		0x0D781DF0,
		0x0D8DC3F0,
		0x0D8DC470,
		0x0D8DC4F0,
		0x0D8DC570,
		0x0D8DC5F0,
		0x0D8DC670,
		0x0DA60870,
		0x0DBAD9F0,
		0x0DBADA70,
		0x0DBADAF0,
		0x0DBADB70,
		0x0DBADBF0,
		0x0DBADC70,
		0x0DD0B170,
		0x0DD0B1F0,
		0x0DD0B270,
		0x0DD0B2F0,
		0x0DD0B370,
		0x0DD0B3F0,
		0x0DE96870,
		0x0DE968F0,
		0x0DE969F0,
		0x0DFF5670,
		0x0DFF56F0,
		0x0DFF5770,
		0x0DFF57F0,
		0x0DFF5870,
		0x0DFF58F0,
		
		0x0E150670,
		0x0E1506F0,
		0x0E150770,
		0x0E1507F0,
		0x0E150870,
		0x0E1508F0,
		0x0E519EF0,
		0x0E519F70,
		0x0E519FF0,
		0x0E51A070,
		0x0E6AD3F0,
		0x0E6AD470,
		0x0E6AD4F0,
		0x0E6AD570,
		0x0E6AD5F0,
		0x0E6AD670,
		0x0E83AAF0,
		0x0E83AB70,
		0x0E83ABF0,
		0x0E83AC70,
		0x0E83ACF0,
		0x0E83AD70,
		0x0E9D3970,
		0x0E9D39F0,
		0x0E9D3A70,
		0x0E9D3AF0,
		0x0E9D3B70,
		0x0EB64070,
		0x0EB640F0,
		0x0EB64170,
		0x0EB641F0,
		0x0EB64270,
		0x0EB642F0,
		0x0ECFA770,
		0x0ECFA7F0,
		0x0ECFA870,
		0x0ECFA8F0,
		0x0EEA14F0,
		0x0EEA1570,
		0x0EEA15F0,
		0x0EEA1670,
		
		0x0F032170,
		0x0F0321F0,
		0x0F032270,
		0x0F0322F0,
		0x0F1C5370,
		0x0F1C53F0,
		0x0F1C5470,
		0x0F1C54F0,
		0x0F1C5570,
		0x0F1C55F0,
		0x0F354770,
		0x0F3547F0,
		0x0F354870,
		0x0F3548F0,
		0x0F354970,
		0x0F4E5F70,
		0x0F4E5FF0,
		0x0F4E6070,
		0x0F4E60F0,
		0x0F4E6170,
		0x0F678570,
		0x0F6785F0,
		0x0F678670,
		0x0F6786F0,
		0x0F678770,
		0x0F6787F0,
		0x0F80C0F0,
		0x0F80C170,
		0x0F80C1F0,
		0x0F80C270,
		0x0F80C2F0,
		0x0F80C370,
		0x0FB656F0,
		0x0FB65770,
		0x0FB657F0,
		0x0FB65870,
		0x0FB658F0,
		0x0FD6BAF0,
		0x0FD6BB70,
		0x0FD6BBF0,
		0x0FD6BC70,
		0x0FD6BCF0,
		0x0FD6BD70,
		0x0FF705F0,
		0x0FF70670,
		0x0FF706F0,
		0x0FF70770,
		0x0FF707F0,
		0x0FF70870,
		
		0x10171B80,
		0x10171C00,
		0x10171C80,
		0x10171D00,
		0x10171E00,
		0x103697F0,
		0x10369870,
		0x103698F0,
		0x10369970,
		0x103699F0,
		0x10369A70,
		0x105803F0,
		0x10580470,
		0x105804F0,
		0x10580570,
		0x105805F0,
		0x10580670,
		0x105806F0,
		0x10785DF0,
		0x10785E70,
		0x10785EF0,
		0x10785F70,
		0x10785FF0,
		0x10786070,
		0x109BD370,
		0x109BD3F0,
		0x109BD470,
		0x109BD4F0,
		0x109BD570,
		0x109BD5F0,
		0x109BD670,
		0x109BD6F0,
		0x10BD1570,
		0x10BD15F0,
		0x10BD1670,
		0x10BD16F0,
		0x10BD1770,
		0x10BD17F0,
		0x10D4A8F0,
		0x10D4A970,
		0x10D4A9F0,
		0x10D4AA70,
		0x10EC34F0,
		0x10EC3570,
		0x10EC35F0,
		0x10EC3670,
		
		0x11034B70,
		0x11034BF0,
		0x11034CF0,
		0x11034D70,
		0x11034DF0,
		0x111AB380,
		0x111AB400,
		0x111AB480,
		0x111AB500,
		0x111AB580,
		0x111AB600,
		0x11351780,
		0x11351800,
		0x11351880,
		0x11351900,
		0x114C3970,
		0x114C39F0,
		0x114C3A70,
		0x114C3B70,
		0x114C3BF0,
		0x11636570,
		0x116365F0,
		0x116366F0,
		0x11636770,
		0x116367F0,
		0x11C52710,
		0x11C52790,
		0x11C52810,
		0x11C52890,
		0x11D5B510,
		0x11D5B590,
		0x11E53990,
		0x11E53A10,
		
		0x12B54A30,
		0x12B54AB0,
		0x12B54B30,
		0x12B54BB0,
		0x12B54C30,
		0x12B54CB0,
		0x12B54D30,
		
		0x1361C9F0,
		0x1361CA70,
		0x1361CAF0,
		0x1361CB70,
		0x138BA870,
		0x138BA8F0,
		0x138BA970,
		0x138BA9F0,
		0x13BAD2F0,
		0x13BAD370,
		0x13BAD3F0,
		0x13BAD470,
		0x13D90BF0,
		0x13D90C70,
		0x13D90CF0,
		0x13D90D70,
		0x13D90DF0,
		0x13D90E70,
		0x13D90EF0,
		0x13D90F70,
		
		0x14135370,
		0x141353F0,
		0x14135470,
		0x141354F0,
		0x14135570,
		0x141355F0,
		0x14135670,
		0x142C29F0,
		0x142C2A70,
		0x142C2AF0,
		0x142C2B70,
		0x142C2BF0,
		0x142C2C70,
		0x142C2CF0,
		0x142C2D70,
		0x145F1D10,
		0x145F1D90,
		0x145F1E10,
		0x145F1E90,
		0x1477E880,
		0x1477E900,
		0x1477E980,
		0x1477EA00,
		0x1477EA80,
		0x1477EB00,
		0x1477EB80,
		0x1477EC00,
		0x14AAFC80,
		0x14AAFD00,
		0x14AAFD80,
		0x14AAFE00,
		0x14C583F0,
		0x14C58470,
		0x14C584F0,
		0x14C58570,
		0x14C585F0,
		0x14C58670,
		0x14C586F0,
		0x14E45F70,
		0x14E45FF0,
		0x14E46070,
		0x14E460F0,
		
		0x15172C70,
		0x15172CF0,
		0x15172D70,
		0x15172DF0,
		0x15172E70,
		0x15172EF0,
		0x15172F70,
		0x15172FF0,
		0x1531AC70,
		0x1531ACF0,
		0x1531AD70,
		0x1531ADF0,
		0x1531AE70,
		0x1531AEF0,
		0x1531AF70,
		0x15664A40,
		0x15664B40,
		0x15664BC0,
		0x15664C40,
		0x15664CC0,
		0x15827870,
		0x158278F0,
		0x15827970,
		0x158279F0,
		0x15827AF0,
		0x15827B70,
		0x15827BF0,
		0x1599C340,
		0x1599C3C0,
		0x1599C440,
		0x1599C4C0,
		0x1599C540,
		0x1599C5C0,
		0x15B0CB10,
		0x15B0CB90,
		0x15B0CC10,
		0x15B0CC90,
		0x15C7C6F0,
		0x15C7C770,
		0x15C7C7F0,
		0x15C7C870,
		0x15C7C8F0,
		0x15C7C970,
		0x15C7C9F0,
		0x15C7CA70,
		0x15E02090,
		0x15E02110,
		0x15E02190,
		0x15E02210,
		0x15FBCA00,
		0x15FBCA80,
		0x15FBCB80,
		0x15FBCC00,
		0x15FBCC80,
		0x15FBCD00,
		0x15FBCD80,
		
		0x1614B0F0,
		0x1614B170,
		0x1614B1F0,
		0x1614B270,
		0x1614B2F0,
		0x1614B370,
		0x1614B3F0,
		0x1614B470,
		0x16341BF0,
		0x16341C70,
		0x16341CF0,
		0x16341D70,
		0x16341DF0,
		0x16341E70,
		0x16341EF0,
		0x16341F70,
		0x16539EF0,
		0x16539F70,
		0x16539FF0,
		0x1653A070,
		0x1653A0F0,
		0x1653A170,
		0x1653A1F0,
		0x1653A270,
		0x166AEEB0,
		0x166AEF30,
		0x166AEFB0,
		0x166AF030,
		0x166AF0B0,
		0x166AF130,
		0x166AF1B0,
		0x166AF230,
		0x169783F0,
		0x16978470,
		0x169784F0,
		0x16978570,
		0x169785F0,
		0x16978670,
		0x16B704F0,
		0x16B70570,
		0x16B705F0,
		0x16B70670,
		0x16B706F0,
		0x16B70770,
		0x16B707F0,
		0x16B70870,
		0x16D35A00,
		0x16D35A80,
		0x16D35B00,
		0x16D35B80,
		0x16D35C00,
		0x16D35C80,
		0x16D35D00,
		0x16D35D80,
		0x16F2DE70,
		0x16F2DEF0,
		0x16F2DF70,
		0x16F2DFF0,
		0x16F2E070,
		0x16F2E0F0,
		0x16F2E170,
		0x16F2E1F0,
		
		0x1711EFF0,
		0x1711F070,
		0x1711F0F0,
		0x1711F170,
		0x175BBEF0,
		0x175BBF70,
		0x175BBFF0,
		0x175BC070,
		0x175BC0F0,
		0x17778A80,
		0x17778B00,
		0x17778B80,
		0x17778C80,
		0x17778D00,
		0x17778D80,
		0x17778E00,
		0x17905770,
		0x179057F0,
		0x17905870,
		0x179058F0,
		0x17905970,
		0x179059F0,
		0x17905A70,
		0x17905AF0,
		0x17B57770,
		0x17B577F0,
		0x17B57870,
		0x17B578F0,
		0x17D39FF0,
		0x17D3A070,
		0x17D3A0F0,
		0x17D3A170,
		0x17D3A1F0,
		0x17D3A270,
		0x17D3A2F0,
		0x17EC3DF0,
		0x17EC3E70,
		0x17EC3F70,
		0x17EC3FF0,
		0x17EC4070,
		
		0x180B7C70,
		0x180B7CF0,
		0x180B7D70,
		0x180B7DF0,
		0x180B7E70,
		0x180B7EF0,
		0x180B7F70,
		0x180B7FF0,
		0x182999F0,
		0x18299A70,
		0x18299AF0,
		0x18299B70,
		0x18299BF0,
		0x18299C70,
		0x18299CF0,
		0x18299D70,
		0x184804F0,
		0x18480570,
		0x184805F0,
		0x18480670,
		0x184806F0,
		0x18480770,
		0x184807F0,
		0x18480870,
		0x1882F500,
		0x1882F580,
		0x1882F600,
		0x1882F680,
		0x18A1DA70,
		0x18A1DAF0,
		0x18A1DB70,
		0x18A1DBF0,
		0x18BA8D70,
		0x18BA8DF0,
		0x18BA8E70,
		0x18BA8EF0,
		0x18BA8F70,
		0x18BA8FF0,
		
		0x192A3170,
		0x192A31F0,
		0x192A3270,
		0x192A32F0,
		0x19440E70,
		0x19440EF0,
		0x19440F70,
		0x19440FF0,
		0x19441070,
		0x194410F0,
		0x19441170,
		0x194411F0,
		0x195AFA90,
		0x195AFB10,
		0x195AFB90,
		0x195AFC10,
		0x1973D5F0,
		0x1973D670,
		0x1973D6F0,
		0x1973D770,
		0x1973D7F0,
		0x1973D870,
		0x1973D8F0,
		0x1973D970,
		0x19C62B70,
		0x19C62BF0,
		0x19C62C70,
		0x19C62CF0,
		0x19C62D70,
		0x19C62DF0,
		0x19C62E70,
		0x19C62EF0,
		
		0x1A15F1F0,
		0x1A15F270,
		0x1A15F2F0,
		0x1A15F370,
		0x1A15F3F0,
		0x1A15F470,
		0x1A15F4F0,
		0x1A15F570,
		0x1A829370,
		0x1A8293F0,
		0x1A829470,
		0x1A8294F0,
		0x1A829570,
		0x1A9D70F0,
		0x1A9D7170,
		0x1A9D71F0,
		0x1A9D7270,
		0x1A9D72F0,
		0x1AB63670,
		0x1AB636F0,
		0x1AB63770,
		0x1AB637F0,
		0x1AB63870,
		0x1AB638F0,
		0x1AB63970,
		0x1AD24270,
		0x1AD242F0,
		0x1AD24370,
		0x1AD243F0,
		0x1AD24470,
		0x1AEB3480,
		0x1AEB3500,
		0x1AEB3580,
		0x1AEB3600,
		
		0x1B05E080,
		0x1B05E100,
		0x1B05E180,
		0x1B05E200,
		0x1B1ECC00,
		0x1B1ECC80,
		0x1B1ECD00,
		0x1B1ECD80,
		0x1B1ECE00,
		0x1B38DEF0,
		0x1B38DF70,
		0x1B38DFF0,
		0x1B38E070,
		0x1B38E0F0,
		0x1B38E170,
		0x1B52E0F0,
		0x1B52E170,
		0x1B52E1F0,
		0x1B52E270,
		0x1B52E2F0,
		0x1B52E370,
		0x1B69C100,
		0x1B69C180,
		0x1B69C200,
		0x1B69C280,
		0x1B813F80,
		0x1B814000,
		0x1B9B1900,
		0x1B9B1980,
		0x1B9B1A00,
		0x1BB3D980,
		0x1BB3DA00,
		0x1BCAAC80,
		0x1BCAAD00,
		0x1BCAAD80,
		0x1BCAAE00,
		0x1BE23080,
		0x1BE23100,
		0x1BFC0080,
		0x1BFC0100,
		0x1BFC0180,
		
		0x1C15A280,
		0x1C15A300,
		0x1C2D4A00,
		0x1C2D4A80,
		0x1C6B85F0,
		0x1C6B8670,
		0x1C6B86F0,
		0x1C6B8770,
		0x1C6B87F0,
		0x1C6B8870,
		0x1C6B88F0,
		0x1C875070,
		0x1C8750F0,
		0x1C875170,
		0x1C8751F0,
		0x1C875270,
		0x1C8752F0,
		0x1C875370,
		0x1C9FBE70,
		0x1C9FBEF0,
		0x1C9FBF70,
		0x1C9FBFF0,
		0x1C9FC070,
		0x1C9FC0F0,
		0x1CB99D70,
		0x1CB99DF0,
		0x1CB99E70,
		0x1CB99EF0,
		0x1CB99F70,
		0x1CB99FF0,
		0x1CB9A070,
		0x1CD35B70,
		0x1CD35BF0,
		0x1CD35C70,
		0x1CD35CF0,
		0x1CD35D70,
		0x1CD35DF0,
		0x1CD35E70,
		0x1CEEA7F0,
		0x1CEEA870,
		0x1CEEA8F0,
		0x1CEEA970,
		
		0x1D08DFF0,
		0x1D08E070,
		0x1D08E0F0,
		0x1D08E170,
		0x1D2316F0,
		0x1D231770,
		0x1D2317F0,
		0x1D231870,
		0x1D3D5070,
		0x1D3D50F0,
		0x1D3D5170,
		0x1D3D51F0,
		0x1D5786F0,
		0x1D578770,
		0x1D5787F0,
		0x1D578870,
		0x1D71BC70,
		0x1D71BCF0,
		0x1D71BD70,
		0x1D71BDF0,
		0x1D8BF370,
		0x1D8BF3F0,
		0x1D8BF470,
		0x1D8BF4F0,
		0x1DA625F0,
		0x1DA62670,
		0x1DA626F0,
		0x1DA62770,
		0x1DC8B2F0,
		0x1DC8B370,
		0x1DC8B3F0,
		0x1DC8B470,
		0x1DC8B4F0,
		0x1DC8B570,
		0x1DC8B5F0,
		0x1DE6ABB0,
		0x1DE6AC30,
		0x1DE6ACB0,
		0x1DE6AD30,
		
		0x1E04A190,
		0x1E04A210,
		0x1E04A290,
		
		0x1F37F570,
		0x1F37F5F0,
		0x1F37F670,
		0x1F37F6F0,
		0x1F5D00F0,
		0x1F5D0170,
		0x1F5D01F0,
		0x1F5D0270,
		0x1FADFE30,
		0x1FADFEB0,
		0x1FADFF30,
		0x1FADFFB0,
		0x1FAE0030,
		0x1FAE00B0,
		0x1FAE0130,
		0x1FC65E70,
		0x1FC65EF0,
		0x1FC65F70,
		0x1FC65FF0,
		0x1FC66070,
		0x1FE02FF0,
		0x1FE03070,
		0x1FE030F0,
		
		0x2002C880,
		0x2002C900,
		0x2002C980,
		0x2002CA00,
		0x20258D70,
		0x20258DF0,
		0x20258E70,
		0x20258EF0,
		0x20258F70,
		0x20258FF0,
		0x20259070,
		0x202590F0,
		0x2044BB10,
		0x2044BB90,
		0x20803A10,
		0x20803A90,
		0x20803B10,
		0x20803B90,
		
		0x209E4A70,
		0x209E4AF0,
		0x209E4B70,
		0x209E4BF0,
		0x209E4C70,
		0x209E4CF0,
		0x209E4D70,
		
		0x21B41D90,
		0x21B41E10,
		0x21B41E90,
		0x21B41F10,
		0x21B41F90,
		0x21B42010,
		0x21D02D80,
		0x21D02E00,
		0x21D02E80,
		0x21D02F00,
		0x21EE5790,
		0x21EE5810,
		0x21EE5890,
		0x21EE5910,
		
		0x229AB2B0,
		0x229AB330,
		0x229AB3B0,
		0x229AB430,
		0x229AB4B0,
		0x22B653F0,
		
		0x23760600,
		0x23760680,
		0x23760700,
		0x23760780,
		0x238EB070,
		0x238EB0F0,
		0x238EB170,
		0x238EB1F0,
		0x23CE2190,
		0x23CE2210,
		0x23CE2290,
		0x23CE2310,
		0x23CE2390,
		0x23ED4990,
		0x23ED4A10,
		0x23ED4A90,
		0x23ED4B10,
		
		0x240C7B10,
		0x240C7B90,
		0x240C7C10,
		0x240C7C90,
		0x242B5590,
		0x242B5690,
		0x242B5710,
		0x242B5790,
		0x244ABD10,
		0x244ABD90,
		0x244ABE10,
		0x244ABE90,
		0x244ABF10,
		0x24671C70,
		0x24671CF0,
		0x24671D70,
		0x24671DF0,
		0x24835F70,
		0x24835FF0,
		0x24836070,
		0x248360F0,
		0x24836170,
		0x248361F0,
		0x24A22C90,
		0x24A22D10,
		0x24A22D90,
		0x24A22E10,
		0x24BDCF70,
		0x24BDCFF0,
		0x24BDD070,
		0x24DA0E70,
		0x24DA0EF0,
		0x24DA0F70,
		0x24DA0FF0,
		0x24DA1070,
		
		0x250F7200,
		0x250F7300,
		0x250F7380,
		0x250F7400,
		0x252B3860,
		0x252B38E0,
		0x252B3960,
		0x25471870,
		0x254718F0,
		0x25471970,
		0x256369F0,
		0x25636A70,
		0x25636AF0,
		0x25636B70,
		0x25636BF0,
		0x257DC570,
		0x257DC5F0,
		0x257DC670,
		0x257DC6F0,
		0x257DC770,
		0x257DC7F0,
		0x25910870,
		0x259108F0,
		0x25910970,
		0x25CDDC70,
		0x25ED93F0,
		0x25ED9470,
		0x25ED94F0,
		0x25ED9570,
		0x25ED95F0,
		0x260350F0,
		0x26035170,
		0x260351F0,
		0x26035270,
		0x260352F0,
		0x26237AF0,
		0x26237BF0,
		0x26237C70,
		0x26237CF0,
		0x26237D70,
		0x26237DF0,
		0x263D9760,
		0x263D97E0,
		0x265E8070,
		0x265E80F0,
		0x265E8170,
		0x265E81F0,
		0x265E82F0,
		0x265E8370,
		0x267F7D70,
		0x267F7DF0,
		0x267F7E70,
		0x267F7EF0,
		0x267F7F70,
		0x267F7FF0,
		0x267F8070,
		0x267F80F0,
		0x26997680,
		0x26997700,
		0x26997780,
		0x26997800,
		0x26997880,
		0x26B38100,
		0x26B38180,
		0x26B38200,
		0x26B38280,
		0x26B38300,
		0x26CD7E80,
		0x26CD7F00,
		0x26CD7F80,
		0x26CD8000,
		0x26CD8080,
		0x26E78180,
		0x26E78200,
		0x26E78280,
		0x26E78300,
		0x26E78380,
		0x26FBED80,
		0x26FBEE00,
		
		0x270F71F0,
		0x270F7270,
		0x272D5FF0,
		0x272D6070,
		0x274B4AE0,
		0x276AB970,
		0x276AB9F0,
		0x276ABA70,
		0x276ABAF0,
		0x278A0F70,
		0x278A0FF0,
		0x278A1070,
		0x278A10F0,
		0x279D1CF0,
	};
int torchesLEN = *(&torches + 1) - torches;

		//TODO: 
		/*
			figure out which addresses corrispond to which torch in the game.
			
		*/

void randomize_torchsanity(FILE* fp)
{
	int items_no_progression[] = 
	{
		0x01, 0x0C, 0x0D, 0x0E, 0x11, 0x12, 0x13, 0x14, 0x15, 0x19, 0x1B, 0x1F, 0x21, 0x22, 
		0x2A, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x3B, 0x3C, 0x3D, 0x3E, 0x41, 0x43, 0x44,
		0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D,
		0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x86, 0x8A, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
		0x99, 0xA2, 0xA3, 0xA4, 0xA5, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 
		0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0xA2, 0xA3, 0xA4,
		0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0xA2, 0xA3, 0xA4,
		0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0xA2, 0xA3, 0xA4,
		0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0xA2, 0xA3, 0xA4,
		0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0xA2, 0xA3, 0xA4,
		0x00, 0x00, 0x00, 0x00, 0x00, 
		0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0xA2, 0xA3, 0xA4,
		0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0xA2, 0xA3, 0xA4,
		0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0xA2, 0xA3, 0xA4,
		0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0xA2, 0xA3, 0xA4,
		0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0xA2, 0xA3, 0xA4,
		0x00, 0x00, 0x00, 0x00, 0x00,
		0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0xA2, 0xA3, 0xA4,
		0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0xA2, 0xA3, 0xA4,
		0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0xA2, 0xA3, 0xA4,
		0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0xA2, 0xA3, 0xA4,
		0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0xA2, 0xA3, 0xA4,
		0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0xA2, 0xA3, 0xA4,
		0x00, 0x00, 0x00, 0x00, 0x00,
		0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0xA2, 0xA3, 0xA4,
		0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0xA2, 0xA3, 0xA4,
		0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0xA2, 0xA3, 0xA4,
		0x01, 0x0C, 0x0D, 0x0E, 0x11, 0x12, 0x13, 0x14, 0x15, 0x19, 0x1B, 0x1F, 0x21, 0x22, 
		0x2A, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x3B, 0x3C, 0x3D, 0x3E, 0x41, 0x43, 0x44,
		0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D,
		0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x86, 0x8A, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
		0x99, 0xA2, 0xA3, 0xA4, 0xA5, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 
		0x00, 0x00, 0x00, 0x00, 0x00,
		0xAA, 0xAB, 0xAC, 0xAD, 0xAE,
		0xAA, 0xAB, 0xAC, 0xAD, 0xAE,
		0xAA, 0xAB, 0xAC, 0xAD, 0xAE,
		0xAA, 0xAB, 0xAC, 0xAD, 0xAE,
		0xAA, 0xAB, 0xAC, 0xAD, 0xAE,
		0x00, 0x00, 0x00, 0x00, 0x00,
		0xAA, 0xAB, 0xAC, 0xAD, 0xAE,
		0xAA, 0xAB, 0xAC, 0xAD, 0xAE,
		0xAA, 0xAB, 0xAC, 0xAD, 0xAE,
		0xAA, 0xAB, 0xAC, 0xAD, 0xAE,
		0x00, 0x00, 0x00, 0x00, 0x00,
		0xAA, 0xAB, 0xAC, 0xAD, 0xAE,
		0xAA, 0xAB, 0xAC, 0xAD, 0xAE,
		0xAA, 0xAB, 0xAC, 0xAD, 0xAE,
		0xAA, 0xAB, 0xAC, 0xAD, 0xAE,
		0xAA, 0xAB, 0xAC, 0xAD, 0xAE,
		0xAA, 0xAB, 0xAC, 0xAD, 0xAE,
		0xA5, 0xA5, 0xA5, 0xA5, 0xA5,
		0x00, 0x00, 0x00, 0x00, 0x00,
		
		
	};
	int items_Length = *(&items_no_progression + 1) - items_no_progression;
	
	int randVal = 0;
	unsigned char newByte = 0x00;
	
	int SIZE = 1;
	unsigned char buffer[SIZE];
	
		int item_progression[] = {
		0x7F, 0x80, 0x81, 0x82, 0x83,
		0x02, 0x03, 0x05,
		0x42,
		0x55, 0x58, 0x5E,
		0x86, 0x8D,
	};

	int invalid_overwrite_items[] =
	{
		0x7F, 0x80, 0x81, 0x82, 0x83,
		0x02, 0x03, 0x05,
		0x42,
		0x55, 0x58, 0x5E,
		0x86, 0x8D,
		0x59, 0x5A, 0x5B, 0x5C, 0x5D,
	};

	int item_progression_LEN = sizeof(item_progression) / sizeof(int);
	int invalid_LEN = sizeof(invalid_overwrite_items) / sizeof(int);
	_Bool valid = true;
	
	for(int i = 0; i < torchesLEN; i++)
	{
		valid = true;
		fseek(fp, torches[i], SEEK_SET);
		randVal = rand() % items_Length; 
		fread(buffer,sizeof(buffer),1,fp);
		for(int j = 0; j < invalid_LEN; j++)
		{
			if(buffer[0] == invalid_overwrite_items[j])
			{
				valid = false;
				break;
			}
		}
		if(valid)
		{
			newByte = items_no_progression[randVal];
			fwrite(&newByte,sizeof(newByte),1,fp);
		}
	}


	for(int i = 0; i < item_progression_LEN; i++)
	{
		if ((rand() % 100) < 33)
		{
			int randVal;
			unsigned char buffer[1];
			int attempts = 0;
			_Bool valid;

			do
			{
				valid = true;    // ← FIXED: reset each iteration

				if (attempts++ > 5000)
					break;       // ← safety to avoid infinite loop

				randVal = rand() % torchesLEN;

				fseek(fp, torches[randVal], SEEK_SET);
				fread(buffer, sizeof(buffer), 1, fp);

				for (int j = 0; j < invalid_LEN; j++)
				{
					if (buffer[0] == invalid_overwrite_items[j])
					{
						valid = false;
						break;   // ← optional: small optimization
					}
				}

			} while (!valid);

			if (valid)
			{
				unsigned char newByte = item_progression[i];
				fseek(fp, torches[randVal], SEEK_SET);
				fwrite(&newByte, sizeof(newByte), 1, fp);
			}
		}
	}
}
void randomize_orbs_and_whips_to_anywhere(FILE* fp)
{
	unsigned char newByte = 0x00;
	for(int i = 0x0044F590; i <= 0x44F5BF; i+=4)
	{
		fseek(fp,i,SEEK_SET);
		fwrite(&newByte,sizeof(newByte),1,fp);
	}		
	int locations[] =
	{
		//non-Repeatable
			0x20259A60, //Entrance
			0x2044CAE0, //Entrance
			0x2002CF80, //Entrance
			0x09A6A980, //ASML
			0x1745D4E0, //GFbT
			0x05323DE0, //HoSR
			0x23ED6C10, //Theatre
			0x117ECB60, //DPoW
			0x0A0C6B60, //ASML
			//EntrancePotion = 
			0x20259AE0,
			// GFbTPotion = 
			0x14AB1380,
			// DPoWPotion = 
			0x10BD2300,
			// ASMLFlameElementalPotion = 
			0x0B240470,
			// ASMLMegingjordPotion = 
			0x0C556070,
			// HoSR1stPotion = 
			0x0484E010,
			// HoSR2ndPotion = 
			0x049E4D80,
			// Theatre1stPotion = 
			0x23CE4480,
			// Theatre2ndPotion = 
			0x244AE000,
			// HoSRHighPotion = 
			0x02FC9690,
			// PoETHighPotion = 
			0x1F776C60,
			// ASMLHangedManHighPotion = 
			0x0B09D570,
			// ASMLMegingjordHighPotion = 
			0x0C555FF0,
			// TheatreHighPotion = 
			0x21EE6D10,
			// DPoWSuperPotion = 
			0x134CE560,
			// ASMLSuperPotion = 
			0x0C05D210,
			// PotMMSuperPotion = 
			0x1DE6CB60,
			// DPoWHeartRepair = 
			0x12B56990,
			// TheatreHeartRepair = 
			0x21D03C90,
			0x250F8570, //Theatre
			// EntranceSerum = 
			0x2044CB60,
			// HoSRSerum = 
			0x02C297B0,
			// EntranceUncursePotion = 
			0x20804560,
			// HoSRUncursePotion = 
			0x02C29830,
		//Magical Ticket
			0x208045E0,
		//Curtain Time Bell  
			0x233A5CE0,
			// HoSRNeapolitan = 
			0x0319A080,
			// ASMLShortcake = 
			0x0BBD3080,
			// HoSRRamen = 
			0x08017400, //
			// WhiteTigerKey = 
			0x089FB1E0, //HoSR
			// BlueDragonKey = 
			0x124C2060, //DPoW
			// RedPhoenixKey = 
			0x18D2EA60, //GFbT
			// BlackTurtleKey = 
			0x24F26C60, //Theatre
			// YellowDragonKey = 
			0x0A77A3E0, //ASML
			// AncientText2 = 
			0x2245EEE0, //Theatre
			// AncientText1 = 
			0x0BEFB3F0,
			// AncientText3 = 
			0x0C1BC8F0,
			// AncientText4 = 
			0x09D38F00,
			// Map1 = 
			0x2002D000, //Entrance
			// Map2 = 
			0x0D05C770, //ASML
			// Map3 = 
			0x0ECFB510, //DPoW
			// Map4 = 
			0x1599D480, //GFbT
			// Map5 = 
			0x238EC470, //Theatre
		//Event Items
			// ToolBag = 
			0x14467A60,
			// ETablet = 
			0x0B9E9C60,
			// VITablet = 
			0x1EDC0BE0,
			// DragonCrest = 
			0x1EF44160,
			// UnlockJewel = 
			0x1EAE6B60,
			// Svarog Statue = 
			0x209E5880,
			// WolfsFoot = 
			0x0AB0E960,
			// SaiseiIncense = 
			0x198E0B60,
			// BlackBishop = 
			0x08CFE360,
			// LucifersSword = 
			0x231FF460,
			// LittleHammer = 
			0x1233E0E0,
			// MeditativeIncence = 
			0x13337450, //
			// Draupnir = 
			0x13A638E0,
			// AromaEarring = 
			0x22B66E80,
			// RacoonCharm = 
			0x11C532E0,
			// BloodyCape = 
			0x08EA35E0,
			// RingofFire = 
			0x1491E9E0,
			// ArticRing = 
			0x0C34EAE0,
			// RingofThunder = 
			0x18A1DF60,
			// HeartBrooch = 
			0x22EB31E0,
			// JewelCrush = 
			0x172BE1E0,
			// Megingjord = 
			0x0C6C00E0,
			// Brisingamen = 
			0x23539760,
		//Heart Max Up
			// ASMLHPHeartUp = 
			0x0B240370,
			// ASMLFlameElementalHeartUp = 
			0x0B2403F0,
			// DPoWHeartUp1 = 
			0x11C53360, //1st?
			// DPoWHeartUp2 = 
			0x12197CE0, //2nd?
			// GFbTHeartUp = 
			0x19092400,
			// PotMMHeartUp = 
			0x1E1DDA60,
			// TheatreHeartUp = 
			0x22D0CF60,
		//MP Max Up
			// HoSRMPMaxUp = 
			0x088769E0,
			// ASMLMPMaxUp = 
			0x0BD97A80,
			// DPoWMPMaxUp = 
			0x12FD6ED0,
			// TheatreMPMaxUp = 
			0x22646A10,
			// PotMMMPMaxUp = 
			0x1E371160,
		//HP Max Up
			// HoSRHPMaxUp1 = 
			0x02C29730,
			// HoSRHPMaxUp2 = 
			0x086CF560,
			// ASMLHPMaxUp = 
			0x0B3E7EE0,
			// DPoWHPMaxUpBF1 = 
			0x0F9B0880,
			// HoSRHPMaxUpBF2 = 
			0x12004460,
			// TheatreHPMaxUp1 = 
			0x2305A7E0,
			// TheatreHPMaxUp2 = 
			0x23761F00,
			// GFbTHPMaxUp = 
			0x156654E0,
			//// PotMMDoppelHPMaxUp = 0x1C6B9510, //??? could cause problems
			// PotMMHPMaxUp = 
			0x1EC79C60, //
		//$1000
			// HoSR1000 = 
			0x026F22D0, //replace with whip of lightning?
			// ASML1000 = 
			0x0AC7A7E0, //replace with whip of flames?
			// DPoW1000 = 
			0x134CE5E0, //replace with whip of ice?
		//$400
			// GFbT4001 = 
			0x15665560, //replace with red orb?
			// GFbT4002 = 
			0x16343290, //replace with blue orb?
			// GFbT4003 = 
			0x1653B400, //replace with yellow orb?
			// GFbT4004 = 
			0x17120200, //replace with green orb?
			// HoSR400 = 
			0x04B91A10, //replace with purple orb?   

			// ASML4001 = 
			0x09908580, //replace with white bishop?

			// ASML4002 = 
			0x0ADDC6F0, //replace with Sacrificial doll?
			// DPoW4001 = 
			0x12B56890, //replace with Jade Mask?

			// DPoW4002 = 
			0x12B56910, //replace with Diamond?
			
			// Theatre4001 = 
			0x23ED6C90, //replace with earth plate?
			// Theatre4002 = 
			0x240C9D80, //replace with meteor plate?
			// Theatre4003 = 
			0x242B7880, //replace with moonlight plate?
			// Theatre4004 = 
			0x24A24E80, //replace with solar plate?
		//repeatable
		//Torches
		//Knives
			// HoSRKnife = 
			0x0708D580,
			// ASMLKnife = 
			0x0DA607F0,
			// GFbTKnife = 
			0x15FBCB00,
			// PotMMKnife = 
			0x1A6882F0,
			// TheatreKnife = 
			0x26237B70,
		//Axes
			// HoSRAxe = 
			0x0335EA80,
			// ASMLAxe = 
			0x0CED53F0,
			// DPoWFrostElementalAxe = 
			0x10171D80,
			// DPoWBridgeLeverAxe = 
			0x11034C70,
			// GFbTAxe = 
			0x15827A70,
			// PotMMAxe = 
			0x1A688370,
			// TheatreAxe = 
			0x242B5610,
		//Holy Water
			// HoSRHolyWater = 
			0x063C3EF0,
			// GFbTHolyWater =
			0x17778C00,
			// PotMMHolyWater = 
			0x1A6883F0,
			// TheatreHolyWater = 
			0x240C7A90,
		//Crystal
			// HoSRCrystal = 
			0x02DF7310,
			// ASMLCrystal = 
			0x0D05AA70,
			// DPoWCrystal = 
			0x114C3AF0,
			// GFbTCrystal = 
			0x17EC3EF0,
			// PotMMCrystal = 
			0x1A688470,
			// TheatreCrystal = 
			0x265E8270,
		//Cross
			// HoSRCross = 
			0x06703A00,
			// ASMLWhiteOrbCross = 
			0x0D1A24F0,
			// ASML3FCross = 
			0x0DE96970,
			// DPoWCross = 
			0x11636670,
			// PotMMCross = 
			0x1A6884F0,
		//$250
			0x14E46170, //replace with black orb?
	};
	int locationLEN = *(&locations + 1) - locations;
	
	int randVal;
	int SIZE = 1;
	unsigned char buffer[SIZE];
	unsigned char new_byte;
	for(int i = 0x7F; i <= 0x85; i++)
	{
		if(i == 0x84)
			i++;
		do
		{
			randVal = rand() % locationLEN;
			fseek(fp,locations[randVal],SEEK_SET);
			fread(buffer,sizeof(buffer),1,fp);
			//printf("item: 0x%2X \n",buffer[0]);
		}while(!(
			(buffer[0] == 0x00) || 
			(buffer[0] >= 0x2A && buffer[0] <= 0x2F) || 
			(buffer[0] >= 0x43 && buffer[0] <= 0x4C) || 
			(buffer[0] >= 0x93 && buffer[0] <= 0xA9)));
		
		fseek(fp,locations[randVal],SEEK_SET);
		new_byte = (unsigned char)i;
		fwrite(&new_byte,sizeof(new_byte),1,fp);
	}
	for(int i = 0x02; i <= 0x04; i++)
	{
		do
		{
			randVal = rand() % locationLEN;
			fseek(fp,locations[randVal],SEEK_SET);
			fread(buffer,sizeof(buffer),1,fp);
			//printf("item: 0x%2X \n",buffer[0]);
		}while(!(
			(buffer[0] == 0x00) || 
			(buffer[0] >= 0x2A && buffer[0] <= 0x2F) || 
			(buffer[0] >= 0x43 && buffer[0] <= 0x4C) || 
			(buffer[0] >= 0x93 && buffer[0] <= 0xA9)));
		
		fseek(fp,locations[randVal],SEEK_SET);
		new_byte = (unsigned char)i;
		fwrite(&new_byte,sizeof(new_byte),1,fp);
	}
	
}
void randomize_boss_music(FILE* fp)
{
	int music_addresses[] = 
	{
		/* wrong */
		
		/*unknown*/
		//Golem 0x80EA
		//Flame Elemental 0x7FEA 
		//Thunder Elemental 0x7FEA
		//Cursed Memories 0x70EA
		//Admiration of a Clan 0x89EA
		
		/*correct*/
		0x1D4F088, //Undead Parasite 0x87EA
		0x13640088, //Joachim 0x83EA
		0x128C2E08, //Frost Elemental 0x7FEA
		0x23916F08, //Succubus 0x81EA
		0x1F7BD188, //Forgotten One 0x86EA 
		0x19904908, //Medusa 0x82EA
		0x1EF75688, //Walter 0x84EA
		0x1E4FC008, //Death 0x85EA
		
		0x20CD4104, // 1 //Old Man's Cottage 0x6DEA
		0x211B4E84, // 2 //
	};
	int music_address_Len = *(&music_addresses + 1) - music_addresses;
	
	int song_list[] = {
		//0x80EA,
		//0x7FEA,
		//0x7FEA,
		//0x70EA,
		//0x89EA,
		0x87EA,
		0x83EA,
		0x7FEA,
		0x81EA,
		0x86EA,
		0x82EA,
		0x84EA,
		0x85EA,
		
		0x6DEA,
		0x6DEA,
	};
	
	for(int i = 0; i < music_address_Len; i++)
	{
		fseek(fp,music_addresses[i],SEEK_SET);
		
		do
		{
			randVal = rand() % music_address_Len;
		}while(song_list[randVal] == 0x0000);
		
		fwrite(&song_list[randVal],sizeof(song_list[randVal]),1,fp);
		
		song_list[randVal] = 0x0000;
	}
}
void randomize_enemy_HP(FILE* fp)
{
	float newByte = 0.15;
	int randValue;
	
	for(int i = 0; i < 66; i++)
	{
		if(i != 7)
		{
			randValue = rand() % 66;
			newByte = enemies[randValue].HP;
			fseek(fp,enemies[i].HP_address,SEEK_SET);
			fwrite(&newByte,sizeof(newByte),1,fp);
		}
	}
	for(int i = 68; i < enemies_Length; i++)
	{
		randValue = rand() % (enemies_Length - 68);
		newByte = enemies[randValue+68].HP;
		randValue = rand() % 3;
		if(randValue == 2)
		{
			newByte*=2;
		}
		fseek(fp,enemies[i].HP_address,SEEK_SET);
		fwrite(&newByte,sizeof(newByte),1,fp);
	}
}
void randomize_enemy_tolerance_and_or_weaknesses(FILE* fp)
{
	int randVal;
	int tolerance_value = 0x3C03126E; 
	int weakness_value = 0xC248;
	int none_value = 0x0000;
	
	/*
	fseek(fp,2,SEEK_SET);
	fwrite(&tolerance_value,sizeof(tolerance_value),1,fp);
	*/
	
	for(int i = 0; i < enemies_Length; i++)
	{
		for(int j = 0; j < 7; j++)
		{
			randVal = rand() % 5;
			if(randVal == 1) {
				fseek(fp,enemies[i].fire_tolerance_weakness_address+(j*4),SEEK_SET);
				fwrite(&tolerance_value,sizeof(tolerance_value),1,fp);
			} else if(randVal == 2) {
				fseek(fp,enemies[i].fire_tolerance_weakness_address+(j*4),SEEK_SET);
				fwrite(&weakness_value,sizeof(weakness_value),1,fp);
			} else {
				fseek(fp,enemies[i].fire_tolerance_weakness_address+(j*4),SEEK_SET);
				fwrite(&none_value,sizeof(none_value),1,fp);
			}
		}
	}
	
	/*
	int SIZE = 1;
	unsigned char buffer[SIZE];
	int n;
	fseek(fp,0x6F0850,SEEK_SET);
	n  = fread(buffer,sizeof(buffer),1,fp);
	printf("%x \n",buffer[0]);
	*/
}
void randomize_item_locations(FILE* fp)
{
	int locations[] =
	{
		//non-Repeatable
			0x20259A60, //Entrance
			0x2044CAE0, //Entrance
			0x2002CF80, //Entrance
			0x09A6A980, //ASML
			0x1745D4E0, //GFbT
			0x05323DE0, //HoSR
			0x23ED6C10, //Theatre
			0x117ECB60, //DPoW
			0x0A0C6B60, //ASML
			//EntrancePotion = 
			0x20259AE0,
			// GFbTPotion = 
			0x14AB1380,
			// DPoWPotion = 
			0x10BD2300,
			// ASMLFlameElementalPotion = 
			0x0B240470,
			// ASMLMegingjordPotion = 
			0x0C556070,
			// HoSR1stPotion = 
			0x0484E010,
			// HoSR2ndPotion = 
			0x049E4D80,
			// Theatre1stPotion = 
			0x23CE4480,
			// Theatre2ndPotion = 
			0x244AE000,
			// HoSRHighPotion = 
			0x02FC9690,
			// PoETHighPotion = 
			0x1F776C60,
			// ASMLHangedManHighPotion = 
			0x0B09D570,
			// ASMLMegingjordHighPotion = 
			0x0C555FF0,
			// TheatreHighPotion = 
			0x21EE6D10,
			// DPoWSuperPotion = 
			0x134CE560,
			// ASMLSuperPotion = 
			0x0C05D210,
			// PotMMSuperPotion = 
			0x1DE6CB60,
			// DPoWHeartRepair = 
			0x12B56990,
			// TheatreHeartRepair = 
			0x21D03C90,
			0x250F8570, //Theatre
			// EntranceSerum = 
			0x2044CB60,
			// HoSRSerum = 
			0x02C297B0,
			// EntranceUncursePotion = 
			0x20804560,
			// HoSRUncursePotion = 
			0x02C29830,
		//Magical Ticket
			0x208045E0,
		//Curtain Time Bell  
			0x233A5CE0,
			// HoSRNeapolitan = 
			0x0319A080,
			// ASMLShortcake = 
			0x0BBD3080,
			// HoSRRamen = 
			0x08017400, //
			// WhiteTigerKey = 
			0x089FB1E0, //HoSR
			// BlueDragonKey = 
			0x124C2060, //DPoW
			// RedPhoenixKey = 
			0x18D2EA60, //GFbT
			// BlackTurtleKey = 
			0x24F26C60, //Theatre
			// YellowDragonKey = 
			0x0A77A3E0, //ASML
			// AncientText2 = 
			0x2245EEE0, //Theatre
			// AncientText1 = 
			0x0BEFB3F0,
			// AncientText3 = 
			0x0C1BC8F0,
			// AncientText4 = 
			0x09D38F00,
			// Map1 = 
			0x2002D000, //Entrance
			// Map2 = 
			0x0D05C770, //ASML
			// Map3 = 
			0x0ECFB510, //DPoW
			// Map4 = 
			0x1599D480, //GFbT
			// Map5 = 
			0x238EC470, //Theatre
		//Event Items
			// ToolBag = 
			0x14467A60,
			// ETablet = 
			0x0B9E9C60,
			// VITablet = 
			0x1EDC0BE0,
			// DragonCrest = 
			0x1EF44160,
			// UnlockJewel = 
			0x1EAE6B60,
			// Svarog Statue = 
			0x209E5880,
			// WolfsFoot = 
			0x0AB0E960,
			// SaiseiIncense = 
			0x198E0B60,
			// BlackBishop = 
			0x08CFE360,
			// LucifersSword = 
			0x231FF460,
			// LittleHammer = 
			0x1233E0E0,
			// MeditativeIncence = 
			0x13337450, //
			// Draupnir = 
			0x13A638E0,
			// AromaEarring = 
			0x22B66E80,
			// RacoonCharm = 
			0x11C532E0,
			// BloodyCape = 
			0x08EA35E0,
			// RingofFire = 
			0x1491E9E0,
			// ArticRing = 
			0x0C34EAE0,
			// RingofThunder = 
			0x18A1DF60,
			// HeartBrooch = 
			0x22EB31E0,
			// JewelCrush = 
			0x172BE1E0,
			// Megingjord = 
			0x0C6C00E0,
			// Brisingamen = 
			0x23539760,
		//Heart Max Up
			// ASMLHPHeartUp = 
			0x0B240370,
			// ASMLFlameElementalHeartUp = 
			0x0B2403F0,
			// DPoWHeartUp1 = 
			0x11C53360, //1st?
			// DPoWHeartUp2 = 
			0x12197CE0, //2nd?
			// GFbTHeartUp = 
			0x19092400,
			// PotMMHeartUp = 
			0x1E1DDA60,
			// TheatreHeartUp = 
			0x22D0CF60,
		//MP Max Up
			// HoSRMPMaxUp = 
			0x088769E0,
			// ASMLMPMaxUp = 
			0x0BD97A80,
			// DPoWMPMaxUp = 
			0x12FD6ED0,
			// TheatreMPMaxUp = 
			0x22646A10,
			// PotMMMPMaxUp = 
			0x1E371160,
		//HP Max Up
			// HoSRHPMaxUp1 = 
			0x02C29730,
			// HoSRHPMaxUp2 = 
			0x086CF560,
			// ASMLHPMaxUp = 
			0x0B3E7EE0,
			// DPoWHPMaxUpBF1 = 
			0x0F9B0880,
			// HoSRHPMaxUpBF2 = 
			0x12004460,
			// TheatreHPMaxUp1 = 
			0x2305A7E0,
			// TheatreHPMaxUp2 = 
			0x23761F00,
			// GFbTHPMaxUp = 
			0x156654E0,
			//// PotMMDoppelHPMaxUp = 0x1C6B9510, //??? could cause problems
			// PotMMHPMaxUp = 
			0x1EC79C60, //
		//$1000
			// HoSR1000 = 
			0x026F22D0, //replace with whip of lightning?
			// ASML1000 = 
			0x0AC7A7E0, //replace with whip of flames?
			// DPoW1000 = 
			0x134CE5E0, //replace with whip of ice?
		//$400
			// GFbT4001 = 
			0x15665560, //replace with red orb?
			// GFbT4002 = 
			0x16343290, //replace with blue orb?
			// GFbT4003 = 
			0x1653B400, //replace with yellow orb?
			// GFbT4004 = 
			0x17120200, //replace with green orb?
			// HoSR400 = 
			0x04B91A10, //replace with purple orb?   

			// ASML4001 = 
			0x09908580, //replace with white bishop?

			// ASML4002 = 
			0x0ADDC6F0, //replace with Sacrificial doll?
			// DPoW4001 = 
			0x12B56890, //replace with Jade Mask?

			// DPoW4002 = 
			0x12B56910, //replace with Diamond?
			
			// Theatre4001 = 
			0x23ED6C90, //replace with earth plate?
			// Theatre4002 = 
			0x240C9D80, //replace with meteor plate?
			// Theatre4003 = 
			0x242B7880, //replace with moonlight plate?
			// Theatre4004 = 
			0x24A24E80, //replace with solar plate?
		//repeatable
		//Torches
		//Knives
			// HoSRKnife = 
			0x0708D580,
			// ASMLKnife = 
			0x0DA607F0,
			// GFbTKnife = 
			0x15FBCB00,
			// PotMMKnife = 
			0x1A6882F0,
			// TheatreKnife = 
			0x26237B70,
		//Axes
			// HoSRAxe = 
			0x0335EA80,
			// ASMLAxe = 
			0x0CED53F0,
			// DPoWFrostElementalAxe = 
			0x10171D80,
			// DPoWBridgeLeverAxe = 
			0x11034C70,
			// GFbTAxe = 
			0x15827A70,
			// PotMMAxe = 
			0x1A688370,
			// TheatreAxe = 
			0x242B5610,
		//Holy Water
			// HoSRHolyWater = 
			0x063C3EF0,
			// GFbTHolyWater =
			0x17778C00,
			// PotMMHolyWater = 
			0x1A6883F0,
			// TheatreHolyWater = 
			0x240C7A90,
		//Crystal
			// HoSRCrystal = 
			0x02DF7310,
			// ASMLCrystal = 
			0x0D05AA70,
			// DPoWCrystal = 
			0x114C3AF0,
			// GFbTCrystal = 
			0x17EC3EF0,
			// PotMMCrystal = 
			0x1A688470,
			// TheatreCrystal = 
			0x265E8270,
		//Cross
			// HoSRCross = 
			0x06703A00,
			// ASMLWhiteOrbCross = 
			0x0D1A24F0,
			// ASML3FCross = 
			0x0DE96970,
			// DPoWCross = 
			0x11636670,
			// PotMMCross = 
			0x1A6884F0,
		//$250
			0x14E46170, //replace with black orb?
		//common drops
			0x6FCAE8,
			0x6E7CE8,
			0x776FE8,
			0x7626E8,
			0x6E7AE8,
			0x6EF868,
			0x776BE8,
			0x760DE8,
			0x6E78E8,
			0x6FC8E8,
			0x7618E8,
			0x6EC7E8,
			0x6F02E8,
			0x6EFDE8,
			0x7763E8,
			0x6EFCE8,
			0x761FE8,
			0x6EC5E8,
			0x7619E8,
			0x6F04E8,
			0x7617E8,
			0x6F01E8,
			0x6F03E8,
			0x6F0D68,
			0x6FD368, //Axe Armor
			0x6FC7E8,
			0x761BE8,
			0x6FCBE8,
			0x7622E8,
			0x760FE8,
			0x6FD968,
			0x7608E8,
			0x760CE8,
			0x7620E8,
			0x6FD468,
			0x6EC6E8,
			0x761AE8,
			0x6EFFE8,
			0x7764E8,
			0x6F0968,
			0x6F0A68,
			0x6EFEE8,
			0x776AE8,
			0x761CE8,
			0x6FCCE8,
			0x7621E8,
			0x7765E8,
			0x7769E8,
			0x761DE8,
			0x6ECCE8,
			0x7762E8,
			0x7616E8,
			0x6ECBE8,
			0x6FCDE8,
			0x6ECAE8,
			0x7623E8,
			0x6E7BE8,
			0x6EF768,
			0x6F00E8,
			0x761EE8,
			0x6F0B68,
			0x6FD568,
			0x6E79E8,
			0x760EE8,
			0x6ED5E8,
		//rare drops
			0x6FCAEA,
			0x6E7CEA,
			0x776FEA,
			0x7626EA,
			0x6E7AEA,
			0x6EF86A,
			0x776BEA,
			0x760DEA,
			0x6E78EA,
			0x6FC8EA,
			0x7618EA,
			0x6EC7EA,
			0x6F02EA,
			0x6EFDEA,
			0x7763EA,
			0x6EFCEA,
			0x761FEA,
			0x6EC5EA,
			0x7619EA,
			0x6F04EA,
			0x7617EA,
			0x6F01EA,
			0x6F03EA,
			0x6F0D6A,
			0x6FD36A,
			0x6FC7EA,
			0x761BEA,
			0x6FCBEA,
			0x7622EA,
			0x760FEA,
			0x6FD96A,
			0x7608EA,
			0x760CEA,
			0x7620EA,
			0x6FD46A,
			0x6EC6EA,
			0x761AEA,
			0x6EFFEA,
			0x7764EA,
			0x6F096A,
			0x6F0A6A,
			0x6EFEEA,
			0x776AEA,
			0x761CEA,
			0x6FCCEA,
			0x7621EA,
			0x7765EA,
			0x7769EA,
			0x761DEA,
			0x6ECCEA,
			0x7762EA,
			0x7616EA,
			0x6ECBEA,
			0x6FCDEA,
			0x6ECAEA,
			0x7623EA,
			0x6E7BEA,
			0x6EF76A,
			0x6F00EA,
			0x761EEA,
			0x6F0B6A,
			0x6FD56A,
			0x6E79EA,
			0x760EEA,
			0x6ED5EA,
	};
	int locations_Length = *(&locations + 1) - locations; //define max value of randVal to determine location
	float new_byte = 0.15; //drop rate of enemies drops
	unsigned char write_item; 
	for(int i = 0; i < items_Length; i++) //every item must be placed.
	{
		do
		{
			randVal = rand() % locations_Length; 
		}while(locations[randVal] == 0xFF || randVal == 16 + (locations_Length - 65) || randVal == 16 + (locations_Length - (65*2))); //choose random location. also can't be red skeleton drop
		fseek(fp,locations[randVal],SEEK_SET); //goto where the item will be written in the iso
		if(randVal > 110) //is this a repeatable location?
		{
			if(items[i] == 0x90 || items[i] == 0x91 || items[i] == 0x92) //is it a power-up?
			{
				switch(items[i]) //if it is replace it with the item associated with it.
				{
					case 0x90:
						items[i] = 0x2A; //HPups become potions
						break;
					case 0x91:
						items[i] = 0x2D;
						break;
					case 0x92:
						items[i] = 0x2E;
						break;
				}
			}
		}
		write_item = (char)items[i]; //make sure we are writing the 1 byte item id to the location as a char
		fwrite(&write_item,sizeof(write_item),1,fp); //write the item to where fseek went at line 4441
		//printf("item = %d, ID = 0x%x, randVal= %d, locationAddress = 0x%x \n",i+1, items[i], randVal, locations[randVal]);
		if(randVal >= locations_Length - 65) { //is it a rare drop?
			fseek(fp,locations[randVal]+6,SEEK_SET); 
			fwrite(&new_byte,sizeof(new_byte),1,fp); //set the drop rate to value of new_byte
			//locations[randVal-65] = 0xFF;
		} else if ((randVal >= (locations_Length - 130)) && (randVal < (locations_Length - 65))) { //is it a common drop?
			fseek(fp,locations[randVal]+4,SEEK_SET);
			fwrite(&new_byte,sizeof(new_byte),1,fp); //set the drop rate to value of new_byte
			locations[randVal+65] = 0xFF; //if we got the common drop address don't allow the rare drop address to be chosen.
		}
		locations[randVal] = 0xFF; //make sure this location isn't chosen again
		if(items[i] == 0x05 || items[i] == 0x59 || items[i] == 0x5A || items[i] == 0x5B || items[i] == 0x5C || items[i] == 0x5D || items[i] == 0x5E || items[i] == 0x42 || items[i] == 0x55 || items[i] == 0x58)
		{
			hints_items(fp,items[i],randVal,locations_Length-130); //give hints for certain items.
		}
		printf("item = %d \n",i+1); //show user which number of item had been randomized. (+1 because 0 indexed.)
	}
}
/*
void show_item_list()
{
	printf("The available items in the game are: \n");
	for(int i = 0; i < (*(&itemNames + 1) - itemNames); i++)
	{
		printf("%d: %s \n",i,itemNames[i]);
	}
	printf("\n");
}
void custom_item_list_rando(FILE* fp)
{
	int locations[] =
	{
		//non-Repeatable
			0x20259A60, //Entrance
			0x2044CAE0, //Entrance
			0x2002CF80, //Entrance
			0x09A6A980, //ASML
			0x1745D4E0, //GFbT
			0x05323DE0, //HoSR
			0x23ED6C10, //Theatre
			0x117ECB60, //DPoW
			0x0A0C6B60, //ASML
			//EntrancePotion = 
			0x20259AE0,
			// GFbTPotion = 
			0x14AB1380,
			// DPoWPotion = 
			0x10BD2300,
			// ASMLFlameElementalPotion = 
			0x0B240470,
			// ASMLMegingjordPotion = 
			0x0C556070,
			// HoSR1stPotion = 
			0x0484E010,
			// HoSR2ndPotion = 
			0x049E4D80,
			// Theatre1stPotion = 
			0x23CE4480,
			// Theatre2ndPotion = 
			0x244AE000,
			// HoSRHighPotion = 
			0x02FC9690,
			// PoETHighPotion = 
			0x1F776C60,
			// ASMLHangedManHighPotion = 
			0x0B09D570,
			// ASMLMegingjordHighPotion = 
			0x0C555FF0,
			// TheatreHighPotion = 
			0x21EE6D10,
			// DPoWSuperPotion = 
			0x134CE560,
			// ASMLSuperPotion = 
			0x0C05D210,
			// PotMMSuperPotion = 
			0x1DE6CB60,
			// DPoWHeartRepair = 
			0x12B56990,
			// TheatreHeartRepair = 
			0x21D03C90,
			0x250F8570, //Theatre
			// EntranceSerum = 
			0x2044CB60,
			// HoSRSerum = 
			0x02C297B0,
			// EntranceUncursePotion = 
			0x20804560,
			// HoSRUncursePotion = 
			0x02C29830,
		//Magical Ticket
			0x208045E0,
		//Curtain Time Bell  
			0x233A5CE0,
			// HoSRNeapolitan = 
			0x0319A080,
			// ASMLShortcake = 
			0x0BBD3080,
			// HoSRRamen = 
			0x08017400, //
			// WhiteTigerKey = 
			0x089FB1E0, //HoSR
			// BlueDragonKey = 
			0x124C2060, //DPoW
			// RedPhoenixKey = 
			0x18D2EA60, //GFbT
			// BlackTurtleKey = 
			0x24F26C60, //Theatre
			// YellowDragonKey = 
			0x0A77A3E0, //ASML
			// AncientText2 = 
			0x2245EEE0, //Theatre
			// AncientText1 = 
			0x0BEFB3F0,
			// AncientText3 = 
			0x0C1BC8F0,
			// AncientText4 = 
			0x09D38F00,
			// Map1 = 
			0x2002D000, //Entrance
			// Map2 = 
			0x0D05C770, //ASML
			// Map3 = 
			0x0ECFB510, //DPoW
			// Map4 = 
			0x1599D480, //GFbT
			// Map5 = 
			0x238EC470, //Theatre
		//Event Items
			// ToolBag = 
			0x14467A60,
			// ETablet = 
			0x0B9E9C60,
			// VITablet = 
			0x1EDC0BE0,
			// DragonCrest = 
			0x1EF44160,
			// UnlockJewel = 
			0x1EAE6B60,
			// Svarog Statue = 
			0x209E5880,
			// WolfsFoot = 
			0x0AB0E960,
			// SaiseiIncense = 
			0x198E0B60,
			// BlackBishop = 
			0x08CFE360,
			// LucifersSword = 
			0x231FF460,
			// LittleHammer = 
			0x1233E0E0,
			// MeditativeIncence = 
			0x13337450, //
			// Draupnir = 
			0x13A638E0,
			// AromaEarring = 
			0x22B66E80,
			// RacoonCharm = 
			0x11C532E0,
			// BloodyCape = 
			0x08EA35E0,
			// RingofFire = 
			0x1491E9E0,
			// ArticRing = 
			0x0C34EAE0,
			// RingofThunder = 
			0x18A1DF60,
			// HeartBrooch = 
			0x22EB31E0,
			// JewelCrush = 
			0x172BE1E0,
			// Megingjord = 
			0x0C6C00E0,
			// Brisingamen = 
			0x23539760,
		//Heart Max Up
			// ASMLHPHeartUp = 
			0x0B240370,
			// ASMLFlameElementalHeartUp = 
			0x0B2403F0,
			// DPoWHeartUp1 = 
			0x11C53360, //1st?
			// DPoWHeartUp2 = 
			0x12197CE0, //2nd?
			// GFbTHeartUp = 
			0x19092400,
			// PotMMHeartUp = 
			0x1E1DDA60,
			// TheatreHeartUp = 
			0x22D0CF60,
		//MP Max Up
			// HoSRMPMaxUp = 
			0x088769E0,
			// ASMLMPMaxUp = 
			0x0BD97A80,
			// DPoWMPMaxUp = 
			0x12FD6ED0,
			// TheatreMPMaxUp = 
			0x22646A10,
			// PotMMMPMaxUp = 
			0x1E371160,
		//HP Max Up
			// HoSRHPMaxUp1 = 
			0x02C29730,
			// HoSRHPMaxUp2 = 
			0x086CF560,
			// ASMLHPMaxUp = 
			0x0B3E7EE0,
			// DPoWHPMaxUpBF1 = 
			0x0F9B0880,
			// HoSRHPMaxUpBF2 = 
			0x12004460,
			// TheatreHPMaxUp1 = 
			0x2305A7E0,
			// TheatreHPMaxUp2 = 
			0x23761F00,
			// GFbTHPMaxUp = 
			0x156654E0,
			//// PotMMDoppelHPMaxUp = 0x1C6B9510, //??? could cause problems
			// PotMMHPMaxUp = 
			0x1EC79C60, //
		//$1000
			// HoSR1000 = 
			0x026F22D0, //replace with whip of lightning?
			// ASML1000 = 
			0x0AC7A7E0, //replace with whip of flames?
			// DPoW1000 = 
			0x134CE5E0, //replace with whip of ice?
		//$400
			// GFbT4001 = 
			0x15665560, //replace with red orb?
			// GFbT4002 = 
			0x16343290, //replace with blue orb?
			// GFbT4003 = 
			0x1653B400, //replace with yellow orb?
			// GFbT4004 = 
			0x17120200, //replace with green orb?
			// HoSR400 = 
			0x04B91A10, //replace with purple orb?   

			// ASML4001 = 
			0x09908580, //replace with white bishop?

			// ASML4002 = 
			0x0ADDC6F0, //replace with Sacrificial doll?
			// DPoW4001 = 
			0x12B56890, //replace with Jade Mask?

			// DPoW4002 = 
			0x12B56910, //replace with Diamond?
			
			// Theatre4001 = 
			0x23ED6C90, //replace with earth plate?
			// Theatre4002 = 
			0x240C9D80, //replace with meteor plate?
			// Theatre4003 = 
			0x242B7880, //replace with moonlight plate?
			// Theatre4004 = 
			0x24A24E80, //replace with solar plate?
		//repeatable
		//Torches
		//Knives
			// HoSRKnife = 
			0x0708D580,
			// ASMLKnife = 
			0x0DA607F0,
			// GFbTKnife = 
			0x15FBCB00,
			// PotMMKnife = 
			0x1A6882F0,
			// TheatreKnife = 
			0x26237B70,
		//Axes
			// HoSRAxe = 
			0x0335EA80,
			// ASMLAxe = 
			0x0CED53F0,
			// DPoWFrostElementalAxe = 
			0x10171D80,
			// DPoWBridgeLeverAxe = 
			0x11034C70,
			// GFbTAxe = 
			0x15827A70,
			// PotMMAxe = 
			0x1A688370,
			// TheatreAxe = 
			0x242B5610,
		//Holy Water
			// HoSRHolyWater = 
			0x063C3EF0,
			// GFbTHolyWater =
			0x17778C00,
			// PotMMHolyWater = 
			0x1A6883F0,
			// TheatreHolyWater = 
			0x240C7A90,
		//Crystal
			// HoSRCrystal = 
			0x02DF7310,
			// ASMLCrystal = 
			0x0D05AA70,
			// DPoWCrystal = 
			0x114C3AF0,
			// GFbTCrystal = 
			0x17EC3EF0,
			// PotMMCrystal = 
			0x1A688470,
			// TheatreCrystal = 
			0x265E8270,
		//Cross
			// HoSRCross = 
			0x06703A00,
			// ASMLWhiteOrbCross = 
			0x0D1A24F0,
			// ASML3FCross = 
			0x0DE96970,
			// DPoWCross = 
			0x11636670,
			// PotMMCross = 
			0x1A6884F0,
		//$250
			0x14E46170, //replace with black orb?
		//common drops
			0x6FCAE8,
			0x6E7CE8,
			0x776FE8,
			0x7626E8,
			0x6E7AE8,
			0x6EF868,
			0x776BE8,
			0x760DE8,
			0x6E78E8,
			0x6FC8E8,
			0x7618E8,
			0x6EC7E8,
			0x6F02E8,
			0x6EFDE8,
			0x7763E8,
			0x6EFCE8,
			0x761FE8,
			0x6EC5E8,
			0x7619E8,
			0x6F04E8,
			0x7617E8,
			0x6F01E8,
			0x6F03E8,
			0x6F0D68,
			0x6FD368,
			0x6FC7E8,
			0x761BE8,
			0x6FCBE8,
			0x7622E8,
			0x760FE8,
			0x6FD968,
			0x7608E8,
			0x760CE8,
			0x7620E8,
			0x6FD468,
			0x6EC6E8,
			0x761AE8,
			0x6EFFE8,
			0x7764E8,
			0x6F0968,
			0x6F0A68,
			0x6EFEE8,
			0x776AE8,
			0x761CE8,
			0x6FCCE8,
			0x7621E8,
			0x7765E8,
			0x7769E8,
			0x761DE8,
			0x6ECCE8,
			0x7762E8,
			0x7616E8,
			0x6ECBE8,
			0x6FCDE8,
			0x6ECAE8,
			0x7623E8,
			0x6E7BE8,
			0x6EF768,
			0x6F00E8,
			0x761EE8,
			0x6F0B68,
			0x6FD568,
			0x6E79E8,
			0x760EE8,
			0x6ED5E8,
		//rare drops
			0x6FCAEA,
			0x6E7CEA,
			0x776FEA,
			0x7626EA,
			0x6E7AEA,
			0x6EF86A,
			0x776BEA,
			0x760DEA,
			0x6E78EA,
			0x6FC8EA,
			0x7618EA,
			0x6EC7EA,
			0x6F02EA,
			0x6EFDEA,
			0x7763EA,
			0x6EFCEA,
			0x761FEA,
			0x6EC5EA,
			0x7619EA,
			0x6F04EA,
			0x7617EA,
			0x6F01EA,
			0x6F03EA,
			0x6F0D6A,
			0x6FD36A,
			0x6FC7EA,
			0x761BEA,
			0x6FCBEA,
			0x7622EA,
			0x760FEA,
			0x6FD96A,
			0x7608EA,
			0x760CEA,
			0x7620EA,
			0x6FD46A,
			0x6EC6EA,
			0x761AEA,
			0x6EFFEA,
			0x7764EA,
			0x6F096A,
			0x6F0A6A,
			0x6EFEEA,
			0x776AEA,
			0x761CEA,
			0x6FCCEA,
			0x7621EA,
			0x7765EA,
			0x7769EA,
			0x761DEA,
			0x6ECCEA,
			0x7762EA,
			0x7616EA,
			0x6ECBEA,
			0x6FCDEA,
			0x6ECAEA,
			0x7623EA,
			0x6E7BEA,
			0x6EF76A,
			0x6F00EA,
			0x761EEA,
			0x6F0B6A,
			0x6FD56A,
			0x6E79EA,
			0x760EEA,
			0x6ED5EA,
	};
	int locations_Length = *(&locations + 1) - locations;
	printf("The checker will likely NOT work for this \n");
	printf("locations_Length: %d \n", locations_Length);
	float new_byte = 0.15;
	unsigned char write_item;
	char boss_item_choice;
	do
	{
		if(boss_item_choice == 'Y' || boss_item_choice == 'N')
			break;
		printf("Do you want to remove the items from after the boss fights? Y/N \n");
		scanf("%s",&boss_item_choice);
	}while(boss_item_choice != 'Y' || boss_item_choice != 'N');
	if(boss_item_choice == 'Y')
	{
		for(int i = 0x0044F590; i <= 0x44F5BC; i+=4)
		{
			fseek(fp,i,SEEK_SET);
			char new_byte = 0x7E;
			fwrite(&new_byte,sizeof(new_byte),1,fp);
		}			
	}
	int SIZE;
	do
	{
		printf("how many items do you want? \n");
		printf("(This must be greater than 22, and should be less than %d) \n",locations_Length-65);
		printf("(This program could loop forever for any value greater than 205) \n");
		printf("(This MUST be less than %d, if it is higher this program will loop FOREVER!!! \n",locations_Length-2);
		
		printf("(The normal is 227 (for now)) \n");
		scanf("%d",&SIZE);
	}while(SIZE <= 22 || SIZE > (locations_Length - 2));
	int custom_item_list[SIZE];
	show_item_list(); //function 
	printf("This does not include ANY item by default! \n");
	printf("RECOMMEND: you include: \n2,3,5,24,29,60,64,66,85,87,88,89,90,91,92,93,94,127,128,129,130,131,135\n");
	printf("That means boss requirements, keys, etc will not be included by default \n");
	for(int i = 0; i < SIZE; i++)
	{
		custom_item_list[i] = 0x00;
		fflush(stdin);
		printf("enter item id in decimal #%d \n",i);
		scanf("%d",&custom_item_list[i]);
		if(custom_item_list[i] >= 0xB0)
			custom_item_list[i] = 0x00;
		fflush(stdin);
	}
	for(int i = 0; i < SIZE; i++)
	{
		do
		{
			randVal = rand() % locations_Length;
		}while(locations[randVal] == 0xFF || randVal == 16 + (locations_Length - 65) || randVal == 16 + (locations_Length - (65*2)));
		fseek(fp,locations[randVal],SEEK_SET);
		write_item = (char)custom_item_list[i];
		fwrite(&write_item,sizeof(write_item),1,fp);
		//printf("item = %d, ID = 0x%x, randVal= %d, locationAddress = 0x%x \n",i+1, custom_item_list[i], randVal, locations[randVal]);
		if(randVal >= locations_Length - 65) {
			fseek(fp,locations[randVal]+6,SEEK_SET);
			fwrite(&new_byte,sizeof(new_byte),1,fp);
			//locations[randVal-65] = 0xFF;
		} else if ((randVal >= (locations_Length - 130)) && (randVal < (locations_Length - 65))) {
			fseek(fp,locations[randVal]+4,SEEK_SET);
			fwrite(&new_byte,sizeof(new_byte),1,fp);
			locations[randVal+65] = 0xFF;
		}
		locations[randVal] = 0xFF;
		if(custom_item_list[i] == 0x05 || custom_item_list[i] == 0x59 || custom_item_list[i] == 0x5A || custom_item_list[i] == 0x5B || custom_item_list[i] == 0x5C || custom_item_list[i] == 0x5D || custom_item_list[i] == 0x5E || custom_item_list[i] == 0x42 || custom_item_list[i] == 0x55 || custom_item_list[i] == 0x58)
		{
			hints_items(fp,custom_item_list[i],randVal,locations_Length-130);
		}
		printf("item = %d \n",i+1);
	}
}
*/
void randomize_key_doors(FILE* fp)
{
	//ASML
	int red_address = 0xD7F5785; //0x03
	//GFbT
	int blue_address = 0x141C2585; //0x02
	//HoSR
	int yellow_address = 0x8A98305; //0x05
	//DPoW
	int black_address = 0x108AA885; //0x04
	//Theatre
	int white_address = 0x27915F85; //0x01
	int doors[] = {red_address, blue_address, yellow_address, black_address, white_address};
	int keys[] = {1,2,3,4,5};
	int randValue;
	for(int i = 0; i < 5; i++)
	{
		do
		{
			randValue = rand() % 5;
		}while(keys[randValue] == 0);
		fseek(fp,doors[i],SEEK_SET);
		new_byte = keys[randValue];
		fwrite(&new_byte,sizeof(new_byte),1,fp);
		keys[randValue] = 0;
	}
}
void randomize_relic_MP(FILE* fp)
{
	int randValue;
	int newByte;
	int MPCosts[] = 
	{
		0x40F376, //Saisei Incense
		0x40F38E, //Meditative Incense
		0x40F3A6, //Invincible Vase
		0x40F3BE, //Crystal Skull
		0x40F426, //Black Bishop
		0x40F3EE, //White Bishop
		0x40F406, //Lucifer's Sword
		0x40F41E, //Svarog Statue
		0x40F436  //Little Hammer
	};
	int MPcostsAmounts[] = {20,18,18,10,20,12,25,20,40,30};
	for(int i = 0; i < 9; i++)
	{
		randValue = rand() % 10;
		switch(MPcostsAmounts[randValue])
		{
			case 20:
				newByte = 0xA0;
				fseek(fp,MPCosts[i],SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
				newByte = 0x41;
				fseek(fp,MPCosts[i]+1,SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
				break;
			case 18:
				newByte = 0x90;
				fseek(fp,MPCosts[i],SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
				newByte = 0x41;
				fseek(fp,MPCosts[i]+1,SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
				break;
			case 10:
				newByte = 0x20;
				fseek(fp,MPCosts[i],SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
				newByte = 0x41;
				fseek(fp,MPCosts[i]+1,SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
				break;
			case 12:
				newByte = 0x40;
				fseek(fp,MPCosts[i],SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
				newByte = 0x41;
				fseek(fp,MPCosts[i]+1,SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
				break;
			case 25:
				newByte = 0xC8;
				fseek(fp,MPCosts[i],SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
				newByte = 0x41;
				fseek(fp,MPCosts[i]+1,SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
				break;
			case 40:
				newByte = 0x20;
				fseek(fp,MPCosts[i],SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
				newByte = 0x42;
				fseek(fp,MPCosts[i]+1,SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
				break;
			case 30:
				newByte = 0xF0;
				fseek(fp,MPCosts[i],SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
				newByte = 0x41;
				fseek(fp,MPCosts[i]+1,SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
				break;
			default:
				break;
		}
	}
}
void fix_ASML_boss_doors(FILE* fp)
{
	int BossExitAdress[] = {
		//TODO: fix
		0x8F739F0,	//AND 0x1D9C770							//UndeadParasite
		0xB480070, //corrected 								//FlameElemental
		0xA865DF0, //0xB656C70, 							//Golem
		0x1367AD70, //AND 0x1367ADF0 //0x1188D3F0, TODO		//Joachim
		0x12911070, //0xEDB7570,							//FrostElemental
		0x199B3970, //corrected								//Medusa
		0x1A1E9670, //0x19EE0570,							//ThunderElemental
		0x239DA1F0, //0x23672EF0,							//Succubus
		0x1F804FF0,											//ForgottenOne
	};
	unsigned char new_byte = 0x00;
	int Flame_Elemental_address = 0xB147570; //Flame Elemental
	int Golem_address = 0xD5410F0; //Golem
	int SIZE = 1;
	unsigned char buffer[SIZE];
	fseek(fp,Flame_Elemental_address,SEEK_SET);
	fread(buffer,sizeof(buffer),1,fp);
	switch(buffer[0])
	{
		case 0x00: //undead parasite
			//0x8F739F0 AND 0x1D9C770
			fseek(fp,0x8F739F0+6,SEEK_SET);
			fwrite(&new_byte,sizeof(new_byte),1,fp);
			fseek(fp,0x1D9C770+6,SEEK_SET);
			fwrite(&new_byte,sizeof(new_byte),1,fp);
			break;
		case 0x01: //ASML
			fseek(fp,Flame_Elemental_address+2,SEEK_SET);
			fread(buffer,sizeof(buffer),1,fp);
			switch(buffer[0])
			{
				case 0x16: //Flame Elemental 
					fseek(fp,0xB480070+6,SEEK_SET);
					fwrite(&new_byte,sizeof(new_byte),1,fp);	
					break;
				case 0x0E: //Golem
					fseek(fp,0xA865DF0+6,SEEK_SET);
					fwrite(&new_byte,sizeof(new_byte),1,fp);
					break;
				default:
					break;
			}
			break;
		case 0x02: //DPoW
			fseek(fp,Flame_Elemental_address+2,SEEK_SET);
			fread(buffer,sizeof(buffer),1,fp);
			switch(buffer[0])
			{
				case 0x33: //Joachim 
					//0x1367AD70 AND 0x1367ADF0
					fseek(fp,0x1367AD70+6,SEEK_SET);
					fwrite(&new_byte,sizeof(new_byte),1,fp);	
					fseek(fp,0x1367ADF0+6,SEEK_SET);
					fwrite(&new_byte,sizeof(new_byte),1,fp);	
					break;
				case 0x2C: //Frost Elemental
					fseek(fp,0x12911070+6,SEEK_SET);
					fwrite(&new_byte,sizeof(new_byte),1,fp);	
					break;
				default:
					break;
			}
			break;
		case 0x03: //GFbT
			fseek(fp,Flame_Elemental_address+2,SEEK_SET);
			fread(buffer,sizeof(buffer),1,fp);
			switch(buffer[0])
			{
				case 0x37: //Medusa
					fseek(fp,0x199B3970+6,SEEK_SET);
					fwrite(&new_byte,sizeof(new_byte),1,fp);	
					break;
				case 0x3C: //Thunder Elemental
					fseek(fp,0x1A1E9670+6,SEEK_SET);
					fwrite(&new_byte,sizeof(new_byte),1,fp);	
					break;
				default:
					break;
			}
			break;
		case 0x08: //Succubus
			fseek(fp,0x239DA1F0+6,SEEK_SET);
			fwrite(&new_byte,sizeof(new_byte),1,fp);	
			break;
		case 0x06: //ForgottenOne
			fseek(fp,0x1F804FF0+6,SEEK_SET);
			fwrite(&new_byte,sizeof(new_byte),1,fp);	
			break;
		default:
			break;
	}
	fseek(fp,Golem_address,SEEK_SET);
	fread(buffer,sizeof(buffer),1,fp);
	switch(buffer[0])
	{
		case 0x00: //undead parasite
			//0x8F739F0 AND 0x1D9C770
			fseek(fp,0x8F739F0+6,SEEK_SET);
			fwrite(&new_byte,sizeof(new_byte),1,fp);
			fseek(fp,0x1D9C770+6,SEEK_SET);
			fwrite(&new_byte,sizeof(new_byte),1,fp);
			break;
		case 0x01: //ASML
			fseek(fp,Golem_address+2,SEEK_SET);
			fread(buffer,sizeof(buffer),1,fp);
			switch(buffer[0])
			{
				case 0x16: //Flame Elemental 
					fseek(fp,0xB480070+6,SEEK_SET);
					fwrite(&new_byte,sizeof(new_byte),1,fp);	
					break;
				case 0x0E: //Golem
					fseek(fp,0xA865DF0+6,SEEK_SET);
					fwrite(&new_byte,sizeof(new_byte),1,fp);
					break;
				default:
					break;
			}
			break;
		case 0x02: //DPoW
			fseek(fp,Golem_address+2,SEEK_SET);
			fread(buffer,sizeof(buffer),1,fp);
			switch(buffer[0])
			{
				case 0x33: //Joachim 
					//0x1367AD70 AND 0x1367ADF0
					fseek(fp,0x1367AD70+6,SEEK_SET);
					fwrite(&new_byte,sizeof(new_byte),1,fp);	
					fseek(fp,0x1367ADF0+6,SEEK_SET);
					fwrite(&new_byte,sizeof(new_byte),1,fp);	
					break;
				case 0x2C: //Frost Elemental
					fseek(fp,0x12911070+6,SEEK_SET);
					fwrite(&new_byte,sizeof(new_byte),1,fp);	
					break;
				default:
					break;
			}
			break;
		case 0x03: //GFbT
			fseek(fp,Golem_address+2,SEEK_SET);
			fread(buffer,sizeof(buffer),1,fp);
			switch(buffer[0])
			{
				case 0x37: //Medusa
					fseek(fp,0x199B3970+6,SEEK_SET);
					fwrite(&new_byte,sizeof(new_byte),1,fp);	
					break;
				case 0x3C: //Thunder Elemental
					fseek(fp,0x1A1E9670+6,SEEK_SET);
					fwrite(&new_byte,sizeof(new_byte),1,fp);	
					break;
				default:
					break;
			}
			break;
		case 0x08: //Succubus
			fseek(fp,0x239DA1F0+6,SEEK_SET);
			fwrite(&new_byte,sizeof(new_byte),1,fp);	
			break;
		case 0x06: //ForgottenOne
			fseek(fp,0x1F804FF0+6,SEEK_SET);
			fwrite(&new_byte,sizeof(new_byte),1,fp);	
			break;
		default:
			break;
	}
}
void randomize_boss_loadzone(FILE* fp)
{
	int randValue;
	
	struct BossData {
		int EntranceAddress;
		int ExitAddress;
		int EntranceData0;
		int EntranceData2;
		int ExitData0;
		int ExitData2;
		char used;
		int ExitDoorNumber;
	};
	int BossEntranceAddress[] = {
		0x82C30F0, //AND 0x82C3070 //UndeadParasite
		0xB147570, //FlameElemental
		0xD5410F0, //Golem
		0x13551370, //Joachim
		0x133F0270, //FrostElemental
		0x19B5A1F0, //Medusa
		0x1A05E870, //Thunder Elemental
		0x237F94F0, //Succubus
		0x1F51DD70 //ForgottenOne
	};
	int BossExitAdress[] = {
		//TODO: fix
		0x8F739F0,	//AND 0x1D9C770
		0xB480070, //corrected 
		0xA865DF0, //0xB656C70, 
		0x1367AD70, //AND 0x1367ADF0 //0x1188D3F0, TODO
		0x12911070, //0xEDB7570,
		0x199B3970, //corrected
		
		0x1A1E9670, //0x19EE0570,
		0x239DA1F0, //0x23672EF0,
		0x1F804FF0
	};
	int BossEntranceData[] = {
		0x00, 0x00, 0x00, 0x00, 
		0x01, 0x00, 0x16, 0x00, 
		0x01, 0x00, 0x0E, 0x00, 
		0x02, 0x00, 0x33, 0x00,
		0x02, 0x00, 0x2C, 0x00,
		0x03, 0x00, 0x37, 0x00,
		0x03, 0x00, 0x3C, 0x00,
		0x08, 0x00, 0x12, 0x00,
		0x06, 0x00, 0x03, 0x00,
	};
	int BossExitData[] = {
		0x00, 0x00, 0x3E, 0x00, 
		0x01, 0x00, 0x14, 0x00, //00
		0x01, 0x00, 0x2B, 0x00, 
		0x02, 0x00, 0x32, 0x00,
		0x02, 0x00, 0x31, 0x00,
		0x03, 0x00, 0x38, 0x00, //01
		0x03, 0x00, 0x3B, 0x00,
		0x08, 0x00, 0x11, 0x00,
		0x06, 0x00, 0x01, 0x00,
	};
	int ExitDoorNumber[] = {
		0x01, //Undead Parasite
		0x00, //Flame Elemental
		0x00, //Golem
		0x01, //Joachim
		0x01, //Frost Elemental
		0x01, //Medusa
		0x01, //Thunder Elemental
		0x01, //Succubus
		0x01, //Forgotten One
	};
	int BossDataLength = *(&BossEntranceAddress + 1) - BossEntranceAddress;
	struct BossData arr_BossData[BossDataLength];
	for(int i = 0; i < BossDataLength; i++)
	{
		arr_BossData[i].EntranceAddress = BossEntranceAddress[i];
		arr_BossData[i].ExitAddress = BossExitAdress[i];
		arr_BossData[i].EntranceData0 = BossEntranceData[i*4];
		arr_BossData[i].EntranceData2 = BossEntranceData[i*4 + 2];
		arr_BossData[i].ExitData0 = BossExitData[i*4];
		arr_BossData[i].ExitData2 = BossExitData[i*4 + 2];
		arr_BossData[i].used = 'N';
		arr_BossData[i].ExitDoorNumber = ExitDoorNumber[i];
	}
	char* bossName;
	int new_byte;
	for(int i = 0; i < BossDataLength; i++)
	{
		do
		{
			randValue = rand() % BossDataLength;
		}while(arr_BossData[randValue].used == 'U');
		
		fseek(fp,arr_BossData[i].EntranceAddress,SEEK_SET);
		fwrite(&arr_BossData[randValue].EntranceData0,sizeof(arr_BossData[randValue].EntranceData0),1,fp);
		fseek(fp,arr_BossData[i].EntranceAddress+2,SEEK_SET);
		fwrite(&arr_BossData[randValue].EntranceData2,sizeof(arr_BossData[randValue].EntranceData2),1,fp);
		
		fseek(fp,arr_BossData[randValue].ExitAddress,SEEK_SET);
		fwrite(&arr_BossData[i].ExitData0,sizeof(arr_BossData[i].ExitData0),1,fp);
		fseek(fp,arr_BossData[randValue].ExitAddress+2,SEEK_SET);
		fwrite(&arr_BossData[i].ExitData2,sizeof(arr_BossData[i].ExitData2),1,fp);
		
		fseek(fp,arr_BossData[i].ExitAddress+0x10+0x04,SEEK_SET);
		new_byte = 0x01018000;
		fwrite(&new_byte,sizeof(new_byte),1,fp);
		
		arr_BossData[randValue].used = 'U';
		
		if(randValue == 0)
		{
			fseek(fp,0x1D9C770,SEEK_SET);
			fwrite(&arr_BossData[i].ExitData0,sizeof(arr_BossData[i].ExitData0),1,fp);
			fseek(fp,0x1D9C770+2,SEEK_SET);
			fwrite(&arr_BossData[i].ExitData2,sizeof(arr_BossData[i].ExitData2),1,fp);	
			fseek(fp,0x1D9C770+6,SEEK_SET);
			new_byte = ExitDoorNumber[randValue]; 
			fwrite(&new_byte,sizeof(new_byte),1,fp);
			
			fseek(fp,0x1D9C770+0x14,SEEK_SET);
			new_byte = 0x00;
			fwrite(&new_byte,sizeof(new_byte),1,fp);
		}
		if(randValue == 3)
		{
			fseek(fp,0x1367ADF0,SEEK_SET);
			fwrite(&arr_BossData[i].ExitData0,sizeof(arr_BossData[i].ExitData0),1,fp);
			fseek(fp,0x1367ADF0+2,SEEK_SET);
			fwrite(&arr_BossData[i].ExitData2,sizeof(arr_BossData[i].ExitData2),1,fp);	
			fseek(fp,0x1367ADF0+6,SEEK_SET);
			new_byte = ExitDoorNumber[randValue]; 
			fwrite(&new_byte,sizeof(new_byte),1,fp);
			
			fseek(fp,0x1367ADF0+0x14,SEEK_SET);
			new_byte = 0x00;
			fwrite(&new_byte,sizeof(new_byte),1,fp);
		}
		if( i == 0 )
		{
			fseek(fp,0x82C3070,SEEK_SET);
			fwrite(&arr_BossData[randValue].EntranceData0,sizeof(arr_BossData[randValue].EntranceData0),1,fp);
			fseek(fp,0x82C3070+2,SEEK_SET);
			fwrite(&arr_BossData[randValue].EntranceData2,sizeof(arr_BossData[randValue].EntranceData2),1,fp);	
		}
		
		fseek(fp,arr_BossData[randValue].ExitAddress+6,SEEK_SET);
		new_byte = ExitDoorNumber[i];
		fwrite(&new_byte,sizeof(new_byte),1,fp);
		switch(randValue)
		{
			case 0:
				bossName = "Undead Parasite";
				break;
			case 1:
				bossName = "Flame Elemental";
				break;
			case 2:
				bossName = "Golem";
				break;
			case 3:
				bossName = "Joachim";
				break;
			case 4:
				bossName = "Frost Elemental";
				break;
			case 5:
				bossName = "Medusa";
				break;
			case 6:
				bossName = "Thunder Elemental";
				break;
			case 7:
				bossName = "Succubus";
				break;
			case 8:
				bossName = "Forgotten One";
				break;
			default:
				break;
		}
		hints_bosses(fp,bossName,arr_BossData[i].EntranceData2);
	}
	fix_ASML_boss_doors(fp);
}
void randomize_orbs_and_whips(FILE* fp)
{
	int OrbsWhipArray[] = {0x7F, 0x80, 0x81, 0x82, 0x83, 0x85, 0x02, 0x03, 0x04};
	int OrbsWhipArrayLength = *(&OrbsWhipArray + 1) - OrbsWhipArray;
	const int startOrbsWhipsTable = 0x0044F590;
	int randValue;
	for(int i = 0; i < 12; i++)
	{
		if(((i < 5 || i > 7) || (i == 6 && true)) && (i != 11))
		{
			fseek(fp, startOrbsWhipsTable + (4*i), SEEK_SET);
			do
			{
				randValue = rand() % OrbsWhipArrayLength;
			} while(OrbsWhipArray[randValue] == 0);
			newByte = OrbsWhipArray[randValue];
			OrbsWhipArray[randValue] = 0;
			fwrite(&newByte,sizeof(newByte),1,fp);
		}
	}
}
void randomize_doppel_fight(FILE* fp)
{
	int randVal;
	struct BossData {
		char* name;
		int EntranceAddress;
		int reverseEntranceAddress;
		int ExitAddress;
		int reverseExitAddress;
		int EntranceData0;
		int EntranceData2;
		int reverseEntranceData0;
		int reverseEntranceData2;
		int reverseEntranceDoorAddress;
		int reverseEntranceDoorNumber;
		int ExitData0;
		int ExitData2;
		int reverseExitData0;
		int reverseExitData2;
	};
	
	struct BossData arr_BossData[2];
	//Blue Doppelganger
	arr_BossData[0].name = "Blue"; //name
	arr_BossData[0].EntranceAddress = 0x10ABECF0;	//EntranceAddress
	arr_BossData[0].reverseEntranceAddress = 0xFA57F70;	//reverseEntranceAddress //door 2?
	arr_BossData[0].ExitAddress = 0xFA57FF0;	//ExitAddress
	arr_BossData[0].reverseExitAddress = 0x115546F0;	//reverseExitAddress
	arr_BossData[0].EntranceData0 = 0x02;	//EntranceData0
	arr_BossData[0].EntranceData2 = 0x10;	//EntranceData2
	arr_BossData[0].reverseEntranceData0 = 0x02;	//reverseEntranceData0
	arr_BossData[0].reverseEntranceData2 = 0x18;	//reverseEntranceData2 
	arr_BossData[0].reverseEntranceDoorAddress = 0xFA57F76; //reverseEntranceDoorAddress
	arr_BossData[0].reverseEntranceDoorNumber = 0x02;	//reverseEntranceDoorNumber
	arr_BossData[0].ExitData0 = 0x02;	//ExitData0
	arr_BossData[0].ExitData2 = 0x1F;	//ExitData2
	arr_BossData[0].reverseExitData0 = arr_BossData[0].EntranceData0;	//reverseExitData0
	arr_BossData[0].reverseExitData2 = arr_BossData[0].EntranceData2;	//reverseExitData2
	//Red Doppelganger
	arr_BossData[1].name = "Red"; //name
	arr_BossData[1].EntranceAddress = 0x1A7288F0;	//EntranceAddress
	arr_BossData[1].reverseEntranceAddress = 0x1C5EBDF0;	//reverseEntranceAddress //door 1?
	arr_BossData[1].ExitAddress = 0x1C5EBE70;	//ExitAddress
	arr_BossData[1].reverseExitAddress = 0x1A8D9670;	//reverseExitAddress
	arr_BossData[1].EntranceData0 = 0x05;	//EntranceData0
	arr_BossData[1].EntranceData2 = 0x16;	//EntranceData2
	arr_BossData[1].reverseEntranceData0 = 0x05;	//reverseEntranceData0
	arr_BossData[1].reverseEntranceData2 = 0x03;	//reverseEntranceData2 
	arr_BossData[1].reverseEntranceDoorAddress = 0x1C5EBDF6; //reverseEntranceDoorAddress
	arr_BossData[1].reverseEntranceDoorNumber = 0x01;	//reverseEntranceDoorNumber
	arr_BossData[1].ExitData0 = 0x05;	//ExitData0
	arr_BossData[1].ExitData2 = 0x04;	//ExitData2
	arr_BossData[1].reverseExitData0 = arr_BossData[1].EntranceData0;	//reverseExitData0
	arr_BossData[1].reverseExitData2 = arr_BossData[1].EntranceData2;	//reverseExitData2
	
	randVal = rand() % 3;
	if(randVal == 0 || randVal == 1)
	{
		fseek(fp,arr_BossData[0].EntranceAddress,SEEK_SET);
		fwrite(&arr_BossData[1].EntranceData0,sizeof(arr_BossData[1].EntranceData0),1,fp);
		fseek(fp,arr_BossData[0].EntranceAddress+2,SEEK_SET);
		fwrite(&arr_BossData[1].EntranceData2,sizeof(arr_BossData[1].EntranceData2),1,fp);
		
		fseek(fp,arr_BossData[1].EntranceAddress,SEEK_SET);
		fwrite(&arr_BossData[0].EntranceData0,sizeof(arr_BossData[0].EntranceData0),1,fp);
		fseek(fp,arr_BossData[1].EntranceAddress+2,SEEK_SET);
		fwrite(&arr_BossData[0].EntranceData2,sizeof(arr_BossData[0].EntranceData2),1,fp);
		
		fseek(fp,arr_BossData[0].ExitAddress,SEEK_SET);
		fwrite(&arr_BossData[1].ExitData0,sizeof(arr_BossData[1].ExitData0),1,fp);
		fseek(fp,arr_BossData[0].ExitAddress+2,SEEK_SET);
		fwrite(&arr_BossData[1].ExitData2,sizeof(arr_BossData[1].ExitData2),1,fp);
		
		fseek(fp,arr_BossData[1].ExitAddress,SEEK_SET);
		fwrite(&arr_BossData[0].ExitData0,sizeof(arr_BossData[0].ExitData0),1,fp);
		fseek(fp,arr_BossData[1].ExitAddress+2,SEEK_SET);
		fwrite(&arr_BossData[0].ExitData2,sizeof(arr_BossData[0].ExitData2),1,fp);
		
		fseek(fp,arr_BossData[0].reverseExitAddress,SEEK_SET);
		fwrite(&arr_BossData[1].reverseExitData0,sizeof(arr_BossData[1].reverseExitData0),1,fp);
		fseek(fp,arr_BossData[0].reverseExitAddress+2,SEEK_SET);
		fwrite(&arr_BossData[1].reverseExitData2,sizeof(arr_BossData[1].reverseExitData2),1,fp);
		
		fseek(fp,arr_BossData[1].reverseExitAddress,SEEK_SET);
		fwrite(&arr_BossData[0].reverseExitData0,sizeof(arr_BossData[0].reverseExitData0),1,fp);
		fseek(fp,arr_BossData[1].reverseExitAddress+2,SEEK_SET);
		fwrite(&arr_BossData[0].reverseExitData2,sizeof(arr_BossData[0].reverseExitData2),1,fp);
		
		fseek(fp,arr_BossData[0].reverseEntranceAddress,SEEK_SET);
		fwrite(&arr_BossData[1].reverseEntranceData0,sizeof(arr_BossData[1].reverseEntranceData0),1,fp);
		fseek(fp,arr_BossData[0].reverseEntranceAddress+2,SEEK_SET);
		fwrite(&arr_BossData[1].reverseEntranceData2,sizeof(arr_BossData[1].reverseEntranceData2),1,fp);
		
		fseek(fp,arr_BossData[1].reverseEntranceAddress,SEEK_SET);
		fwrite(&arr_BossData[0].reverseEntranceData0,sizeof(arr_BossData[0].reverseEntranceData0),1,fp);
		fseek(fp,arr_BossData[1].reverseEntranceAddress+2,SEEK_SET);
		fwrite(&arr_BossData[0].reverseEntranceData2,sizeof(arr_BossData[0].reverseEntranceData2),1,fp);
		
		fseek(fp,arr_BossData[0].reverseEntranceDoorAddress,SEEK_SET);
		fwrite(&arr_BossData[1].reverseEntranceDoorNumber,sizeof(arr_BossData[1].reverseEntranceDoorNumber),1,fp);
		
		fseek(fp,arr_BossData[1].reverseEntranceDoorAddress,SEEK_SET);
		fwrite(&arr_BossData[0].reverseEntranceDoorNumber,sizeof(arr_BossData[0].reverseEntranceDoorNumber),1,fp);
	}
}
void randomize_sub_weapon_attacks(FILE* fp)
{
	int subWeaponAbilityAddresses[] = {
		0x4112BC,
		0x4112E8,
		0x411314,
		0x411340,
		0x41136C,
		0x411398,
		0x4113C4,
		0x4113F0,
		0x41141C,
		0x411448,
		0x411474,
		0x4114A0,
		//0x4114CC,
		0x4114F8,
		0x411524,
		0x411550,
		0x41157C,
		0x4115A8,
		0x4115D4,
		0x411600,
		0x41162C,
		0x411658,
		0x411684,
		0x4116B0,
		0x4116DC,
		0x411708,
		0x411734,
		0x411760,
		0x41178C,
		0x4117B8,
		0x4117E4,
		0x411810,
		0x41183C,
		0x411868,
		0x411894,
		0x4118C0,
		0x4118EC,
		0x411918,
		0x411944,
		0x411970,
		0x41199C,
		0x4119C8,
		0x4119F4,
		0x411A20,
		//0x411A4C,
		0x411A78,
		0x411AA4,
		0x411AD0
	};
	unsigned char subWeaponAbilityIDs[] = {
		0x00,
		0x01,
		0x02,
		0x03,
		0x04,
		0x05,
		0x06,
		0x07,
		0x08,
		0x09,
		0x0A,
		0x0B,
		0x0C,
		0x0D,
		0x0E,
		//0x0F,
		0x10,
		0x11,
		0x12,
		0x13,
		0x14,
		0x15,
		0x16,
		0x17,
		0x18,
		0x19,
		0x1A,
		0x1B,
		0x1C,
		0x1D,
		0x1E,
		0x1F,
		0x20,
		0x21,
		0x22,
		0x23,
		0x24,
		0x25,
		0x26,
		0x27,
		0x28,
		0x29,
		0x2A,
		0x2B,
		//0x2C,
		0x2D,
		0x2E,
		0x2F
	};
	int name_ids[] =
	{
		0x2A4, //[0]
		0x31, //[1]
		0x33, //[2]
		0x34, //[3]
		0x37, //[4]
		0x35, //[5]
		0x32, //[6]
		0x36, //[7]
		0x02A6, //[8]
		0x19, //[9]
		0x15, //[A]
		0x1B, //[B]
		0x1A, //[C]
		0x16, //[D]
		0x17, //[E]
		//0x18, //[F]
		0x02A5, //[10]
		0x23, //[11]
		0x26, //[12]
		0x25, //[13]
		0x27, //[14]
		0x24, //[15]
		0x29, //[16]
		0x28, //[17]
		0x02A7, //[18]
		0x1C, //[19]
		0x20, //[1A]
		0x1D, //[1B]
		0x21, //[1C]
		0x22, //[1D]
		0x1F, //[1E]
		0x1E, //[1F]
		0x02A8, //[20]
		0x2A, //[21]
		0x2B, //[22]
		0x2D, //[23]
		0x2F, //[24]
		0x30, //[25]
		0x2C, //[26]
		0x2E, //[27]
		0x02A9, //[28]
		0x38, //[29]
		0x39, //[2A]
		0x3A, //[2B]
		//0x3B, //[2C]
		0x3C, //[2D]
		0x3D, //[2E]
		0x3E, //[2F]
	};
	for(int i = 0; i < (*(&subWeaponAbilityAddresses + 1) - subWeaponAbilityAddresses); i++)
	{
		fseek(fp,subWeaponAbilityAddresses[i],SEEK_SET);
		do
		{
			randValue = rand() % (*(&subWeaponAbilityIDs + 1) - subWeaponAbilityIDs);
		}while(subWeaponAbilityIDs[randValue] == 0xFF);
		fwrite(&subWeaponAbilityIDs[randValue],sizeof(subWeaponAbilityIDs[randValue]),1,fp);
		subWeaponAbilityIDs[randValue] = 0xFF;
		fseek(fp,subWeaponAbilityAddresses[i]-0x08,SEEK_SET);
		//printf("%d ",i);
		//printf("%d ",randValue);
		//printf("0x%x \n",name_ids[randValue]);
		fwrite(&name_ids[randValue],sizeof(name_ids[randValue]),1,fp);
	}
}
void randomize_sub_weapon_costs(FILE* fp)
{
	int HeartCosts[] = {
		0x4112C6,
		0x4112F2,
		0x41131E,
		0x41134A,
		0x411376,
		0x4113A2,
		0x4113CE,
		0x4113FA,
		0x411426,
		0x411452,
		0x41147E,
		0x4114AA,
		0x4114D6,
		0x411502,
		0x41152E,
		0x41155A,
		0x411586,
		0x4115B2,
		0x4115DE,
		0x41160A,
		0x411636,
		0x411662,
		0x41168E,
		0x4116BA,
		0x4116E6,
		0x411712,
		0x41173E,
		0x41176A,
		0x411796,
		0x4117C2,
		0x4117EE,
		0x41181A,
		0x411846,
		0x411872,
		0x41189E,
		0x4118CA,
		0x4118F6,
		0x411922,
		0x41194E,
		0x41197A
	};
	int HeartCostsLen = *(&HeartCosts + 1) - HeartCosts;
	float new_byte;
	for(int i = 0; i < HeartCostsLen; i++)
	{
		randVal = rand() % 81;
		new_byte = (float)(-1.0*randVal);
		fseek(fp,HeartCosts[i]-2,SEEK_SET);
		fwrite(&new_byte,sizeof(new_byte),1,fp);
	}
}
int randomize_start_sub_weapon(FILE* fp,FILE* fptr)
{
	randValue = rand() % 5;
	newByte = randValue + 0xAA;
	int startingSubWeapon = 0x3E1E4C;
	fseek(fp,startingSubWeapon,SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	
	return newByte;
}
void randomize_area_warp_rooms(FILE* fp)
{
	uint32_t addresses[] = {
		0x1FF125F0, // HoSR
		0x1FF12570, // ASML
		0x1FF12670, // DPoW
		0x1FF126F0, // GFbT
		0x1FF12770,  // Theatre
	};
	
	// Define the data as an array of byte arrays
	uint8_t data[][8] = {
		{0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x01, 0x00},
		{0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00},
		{0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x03, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00},
	};
	
	int areas[] = {1,2,3,4,5};
	
	for(int i = 0; i < 5; i++)
	{
		do
		{
			randVal = rand() % 5;
		}while(areas[randVal] == 0);
		for(int j = 0; j < 8; j++)
		{
			fseek(fp,addresses[i]+j,SEEK_SET);
			fwrite(&data[randVal][j],sizeof(data[randVal][j]),1,fp);
		}
		areas[randVal] = 0;
	}
	
}
void randomize_switch_rooms(FILE* fp)
{
	struct switch_door_stuff
	{
		int entrance_door_address;
		int area_number;
		int room_number;
		int exit_door_address;
		int exit_door_exit_value;
		int exit_room;
		char used;
	};
	struct switch_door_stuff switch_rooms[9];
	int entrance_addresses[] = 
	{
		0x15B9C6F0,
		0x15072770,
		0x17007FF0, 
		//0x13E46FF0, //Storm Skeletons
		0x174DAC70,
		
		0x37D22F0,
		0x360FF70,
		//0x2B233F0,
		0x44A1E70,
		0x4619D70,
		
		0x10F52D70,
	};
	int exit_addresses[] = 
	{
		//guesses
		0x153C9B70, //CORRECT!
		0x14EF4B70, //test
		0x171CC870, //CORRECT!
		0x1736BB70, //test
		
		0x56863F0, //test
		0x5232370, //test
		//0x50BB7F0, //0x6099170, //test //VALIDATE
		0x59733F0, //test 
		0x57FCBF0, //test 
		
		0x125306F0, //CORRECT!
	};
	int roomNumber[] =
	{
		0x0E,
		0x0B,
		0x20, //Jewel Crush
		0x21, //Marker Stone 5
		
		0x21,
		0x1E, //Marker Stone 6
		//0x1D, //Bookshelf
		0x23,
		0x22,
		
		0x2A,
	};
	int areaNumber[] = 
	{
		0x03,
		0x03,
		0x03,
		0x03,
		
		0x00,
		0x00,
		//0x00,
		0x00,
		0x00,
		
		0x02,
	};
	int exitRoomNumber[] = 
	{
		0x13,
		0x0C,
		0x1F,
		0x22,
		
		0x0E,
		0x0D,
		//0x07,
		0x15,
		0x16,
		
		0x1B,
	};
	int exit_val[] = 
	{
		//guesses //room
		0x02, //0x03001300 --CORRECT!
		0x00, //0x03000C00 --WRONG! (try 0)
		0x02, //0x03001F00 --CORRECT!
		0x00, //0x03002200 --WRONG! (try 0)
		
		0x00, //0x00000E00 --WRONG! (try 0)
		0x00, //0x00000D00 --WRONG! (try 0)
		//0x00, //0x00000700 --WRONG! (try 0)
		0x00, //0x00001500 --WRONG! (try 0)
		0x00, //0x00001600 --WRONG! (try 0)
		
		0x01, //0x02001B00 --CORRECT! 
	};
	for(int i = 0; i < 9; i++)
	{
		switch_rooms[i].entrance_door_address = entrance_addresses[i];
		switch_rooms[i].area_number = areaNumber[i];
		switch_rooms[i].room_number = roomNumber[i];
		switch_rooms[i].exit_door_address = exit_addresses[i];
		switch_rooms[i].exit_door_exit_value = exit_val[i];
		switch_rooms[i].exit_room = exitRoomNumber[i];
		switch_rooms[i].used = 'n';
	}	
	for(int i = 0; i < 9; i++)
	{
		do
		{
			randVal = rand() % 9;
		}while(switch_rooms[randVal].used == 'u');
		fseek(fp,switch_rooms[i].entrance_door_address,SEEK_SET);
		fwrite(&switch_rooms[randVal].area_number,sizeof(switch_rooms[randVal].area_number),1,fp);
		fseek(fp,switch_rooms[i].entrance_door_address+2,SEEK_SET);
		fwrite(&switch_rooms[randVal].room_number,sizeof(switch_rooms[randVal].room_number),1,fp);
		
		fseek(fp,switch_rooms[randVal].exit_door_address,SEEK_SET);
		//printf("0x%x",switch_rooms[randVal].exit_door_address);
		fwrite(&switch_rooms[i].area_number,sizeof(switch_rooms[i].area_number),1,fp); //doesn't write to expected location?
		//printf(" becomes 0x%x\n",switch_rooms[i].area_number);
		fseek(fp,switch_rooms[randVal].exit_door_address+2,SEEK_SET);
		//printf("0x%x",switch_rooms[randVal].exit_door_address+2);
		fwrite(&switch_rooms[i].exit_room,sizeof(switch_rooms[i].exit_room),1,fp);
		//printf(" becomes 0x%x\n",switch_rooms[i].exit_room);
		fseek(fp,switch_rooms[randVal].exit_door_address+6,SEEK_SET);
		//printf("0x%x",switch_rooms[randVal].exit_door_address+6);
		fwrite(&switch_rooms[i].exit_door_exit_value,sizeof(switch_rooms[i].exit_door_exit_value),1,fp);
		//printf(" becomes 0x%x\n",switch_rooms[i].exit_door_exit_value);
		
		switch_rooms[randVal].used = 'u';
	}
}
void randomize_armor_def(FILE* fp)
{
	const int EarthPlateDEF = 0x3D99CC;
	const int MeteorPlateDEF = 0x3D9A50;
	const int MoonlightPlateDEF = 0x3D9AD4;
	const int SolarPlateDEF = 0x3D9B58;
	int randVal;
	float values[] = {0,0,0,0};
	char* write_value[] = {"00%","01%","05%","10%","20%","25%","30%","42%","50%","65%"};
	float possibleValue[] = {0.0,1.0,5.0,10.0,20.0,25.0,30.0,42.0,50.0,65.0};
	for(int i = 0; i < 4; i++)
	{
		randVal = rand() % 10;
		values[i] = possibleValue[randVal];
	}
	const int SolarPlateRTW = 0x1193E18;
	const int MoonlightPlateRTW = 0x1193E30;
	const int MeteorPlateRTW = 0x1193E48;
	const int EarthPlateRTW = 0x1193E60;
	int DEF_addresses[] = {SolarPlateDEF,MoonlightPlateDEF,MeteorPlateDEF,EarthPlateDEF};
	int text[] = {SolarPlateRTW,MoonlightPlateRTW,MeteorPlateRTW,EarthPlateRTW};
	char* new_text = "Reduces Damage XX%";
	
	for(int i = 0; i < 4; i++)
	{
		fseek(fp,DEF_addresses[i],SEEK_SET);
		fwrite(&values[i],sizeof(values[i]),1,fp);
		
		for(int j = 0; new_text[j] != '\0'; j++)
		{
			fseek(fp,text[i]+j,SEEK_SET);
			fwrite(&new_text[j],sizeof(new_text[j]),1,fp);
			if(j == 15)
			{
				fseek(fp,text[i]+j,SEEK_SET);
				switch((int)values[i])
				{
					case 0:
						fwrite(&write_value[0][0],sizeof(write_value[0][0]),1,fp);
						break;
					case 1:
						fwrite(&write_value[1][0],sizeof(write_value[1][0]),1,fp);
						break;
					case 5:
						fwrite(&write_value[2][0],sizeof(write_value[2][0]),1,fp);
						break;
					case 10:
						fwrite(&write_value[3][0],sizeof(write_value[3][0]),1,fp);
						break;
					case 20:
						fwrite(&write_value[4][0],sizeof(write_value[4][0]),1,fp);
						break;
					case 25:
						fwrite(&write_value[5][0],sizeof(write_value[5][0]),1,fp);
						break;
					case 30:
						fwrite(&write_value[6][0],sizeof(write_value[6][0]),1,fp);
						break;
					case 42:
						fwrite(&write_value[7][0],sizeof(write_value[7][0]),1,fp);
						break;
					case 50:
						fwrite(&write_value[8][0],sizeof(write_value[8][0]),1,fp);
						break;
					case 65:
						fwrite(&write_value[9][0],sizeof(write_value[9][0]),1,fp);
						break;
					default:
						break;
				}
			}
			if(j == 16)
			{
				fseek(fp,text[i]+j,SEEK_SET);
				switch((int)values[i])
				{
					case 0:
						fwrite(&write_value[0][1],sizeof(write_value[0][1]),1,fp);
						break;
					case 1:
						fwrite(&write_value[1][1],sizeof(write_value[1][1]),1,fp);
						break;
					case 5:
						fwrite(&write_value[2][1],sizeof(write_value[2][1]),1,fp);
						break;
					case 10:
						fwrite(&write_value[3][1],sizeof(write_value[3][1]),1,fp);
						break;
					case 20:
						fwrite(&write_value[4][1],sizeof(write_value[4][1]),1,fp);
						break;
					case 25:
						fwrite(&write_value[5][1],sizeof(write_value[5][1]),1,fp);
						break;
					case 30:
						fwrite(&write_value[6][1],sizeof(write_value[6][1]),1,fp);
						break;
					case 42:
						fwrite(&write_value[7][1],sizeof(write_value[7][1]),1,fp);
						break;
					case 50:
						fwrite(&write_value[8][1],sizeof(write_value[8][1]),1,fp);
						break;
					case 65:
						fwrite(&write_value[9][1],sizeof(write_value[9][1]),1,fp);
						break;
					default:
						break;
				}
			}
		}
	}
}
void randomize_start_DEF(FILE* fp)
{
	const int startDEF_address = 0x3E1E34;
	
	float randVal;
	randVal = rand() % 701;
	randVal = -1*randVal;
	fseek(fp,startDEF_address,SEEK_SET);
	fwrite(&randVal,sizeof(randVal),1,fp);
}
void randomize_sub_weapon_model_and_sprites(FILE* fp)
{
	int knife_sprite_address = 0x3DD180;
	int knife_model_address = 0x3DD184;
	int knife_sprite = 0x4F;
	int knife_model = 0x0D;
	
	int axe_sprite_address = 0x3DD1C8;
	int axe_model_address = 0x3DD1CC;
	int axe_sprite = 0x50;
	int axe_model = 0x0F;
	
	int holy_water_sprite_address = 0x3DD210;
	int holy_water_model_address = 0x3DD214;
	int holy_water_sprite = 0x51;
	int holy_water_model = 0x0E;
	
	int crystal_sprite_address = 0x3DD258;
	int crystal_model_address = 0x3DD25C;
	int crystal_sprite = 0x52;
	int crystal_model = 0x10;
	
	int cross_sprite_address = 0x3DD2A0;
	int cross_model_address = 0x3DD2A4;
	int cross_sprite = 0x53;
	int cross_model = 0x11;
	
	int sub_weapon_addresses[] = {
		knife_sprite_address,knife_model_address,
		axe_sprite_address,axe_model_address,
		holy_water_sprite_address,holy_water_model_address,
		crystal_sprite_address,crystal_model_address,
		cross_sprite_address,cross_model_address,
	};
	int sub_weapon_sprites_and_models[] = {
		knife_sprite,knife_model,
		axe_sprite,axe_model,
		holy_water_sprite,holy_water_model,
		crystal_sprite,crystal_model,
		cross_sprite,cross_model,
	};
	
	int randVal = 0;
	
	for(int i = 0; i < 9; i+=2)
	{
		do
		{
			randVal = rand() % 5;
			randVal *= 2;	
		}while(sub_weapon_sprites_and_models[randVal] == 0xFFFF);
		
		fseek(fp,sub_weapon_addresses[i],SEEK_SET);
		fwrite(&sub_weapon_sprites_and_models[randVal],sizeof(sub_weapon_sprites_and_models[randVal]),1,fp);
		fseek(fp,sub_weapon_addresses[i+1],SEEK_SET);
		fwrite(&sub_weapon_sprites_and_models[randVal+1],sizeof(sub_weapon_sprites_and_models[randVal+1]),1,fp);
		
		sub_weapon_sprites_and_models[randVal] = 0xFFFF;
		sub_weapon_sprites_and_models[randVal + 1] = 0xFFFF;
	}
}
long long reverse_bytes(long long value) {
    long long reversed = 0;
    for (int i = 0; i < 8; i++) {
        reversed |= ((value >> (i * 8)) & 0xFF) << ((7 - i) * 8);
    }
    return reversed;
}
void randomize_save_rooms(FILE* fp)
{
	int Prelude_to_the_Dark_Abyss_Address = 0x1FD08B70; 
	long long Prelude_to_the_Dark_Abyss_Data = reverse_bytes(0x070009000C000000); //wrong door //correct
											//0x0700020000000000;
	int Prison_of_Eternal_Torture_Address = 0x1F692970;	
	long long Prison_of_Eternal_Torture_Data = reverse_bytes(0x0600050013000000); //wrong door //correct
											//0x0600020000000000;
	int Pagoda_of_the_Misty_Moon_Address = 0x1DB8CD70;	
	long long Pagoda_of_the_Misty_Moon_Data = reverse_bytes(0x050000000B000000); //wrong door //correct
											//0x0500230000000000;
	int Anti_soul_Mysteries_Lab_1_Address = 0xA5452F0;	
	long long Anti_soul_Mysteries_Lab_1_Data = reverse_bytes(0x0100340000000000); //wrong door //correct
											//0x01000C0000000000;
	int Anti_soul_Mysteries_Lab_2_Address = 0xD97B970;	
	long long Anti_soul_Mysteries_Lab_2_Data = reverse_bytes(0x0100350001000000); //wrong door //correct
											//0x01002E0000000000;
	int Anti_soul_Mysteries_Lab_3_Address = 0xD5411F0;	
	long long Anti_soul_Mysteries_Lab_3_Data = reverse_bytes(0x0100360001000000); //Check //wrong door //correct
											//0x01002B0000000000;
	int Garden_forgotten_by_Time_1_Address = 0x1604ABF0;
	long long Garden_forgotten_by_Time_1_Data = reverse_bytes(0x03003D0008000000); //Check //INCORRECT...
											//0x0300160000000000;
	int Garden_forgotten_by_Time_2_Address = 0x16C27DF0;
	long long Garden_forgotten_by_Time_2_Data = reverse_bytes(0x03003E0009000000); //wrong door //correct
											//0x03001D0000000000;
	int Garden_forgotten_by_Time_3_Address = 0x18903270;
	long long Garden_forgotten_by_Time_3_Data = reverse_bytes(0x03003F000A000000); //wrong door //correct
											//0x03002D0000000000;
	int Garden_forgotten_by_Time_4_Address = 0x19B5A270;
	long long Garden_forgotten_by_Time_4_Data = reverse_bytes(0x0300400012000000); //wrong door //correct
											//0x0300380000000000;
	int House_of_Sacred_Remains_1_Address = 0x6B07470;	
	long long House_of_Sacred_Remains_1_Data = reverse_bytes(0x0000460003000000); //wrong door //correct
											//0x00002E0000000000;
	int House_of_Sacred_Remains_2_Address = 0x62F85F0;	
	long long House_of_Sacred_Remains_2_Data = reverse_bytes(0x0000450002000000); //wrong door //correct
											//0x0000290000000000;
	int House_of_Sacred_Remains_3_Address = 0x82C3170;	
	long long House_of_Sacred_Remains_3_Data = reverse_bytes(0x0000490011000000); //wrong door //correct
											//0x00003E0000000000;
	int Dark_Palace_of_Waterfall_1_Address = 0x102772F0;
	long long Dark_Palace_of_Waterfall_1_Data = reverse_bytes(0x0200370005000000); //wrong door //correct
											//0x0200140000000000;
	//int Dark_Palace_of_Waterfall_2_Address = ;			
	//long long Dark_Palace_of_Waterfall_2_Data = reverse_bytes(0x0200000000000000); //Verify
											//0x0200180000000000;
	int Dark_Palace_of_Waterfall_3_Address = 0x1046DB70;
	long long Dark_Palace_of_Waterfall_3_Data = reverse_bytes(0x0200380006000000); //wrong door //correct
											//0x0200150000000000;
	int Ghostly_Theatre_1_Address = 0x25DB3D70;			
	long long Ghostly_Theatre_1_Data = reverse_bytes(0x080037000D000000); //wrong door //correct
											//0x0800260000000000;
	int Ghostly_Theatre_2_Address = 0x262ED570;			
	long long Ghostly_Theatre_2_Data = reverse_bytes(0x080038000E000000); //CORRECT!
											//0x0800290000000000;
	int Ghostly_Theatre_3_Address = 0x2756DD70;			
	long long Ghostly_Theatre_3_Data = reverse_bytes(0x080039000F000000); //wrong door //correct
											//0x0800340000000000;
											
	int Save_Room_Addresses[] = {
		Prelude_to_the_Dark_Abyss_Address, Prison_of_Eternal_Torture_Address, Pagoda_of_the_Misty_Moon_Address,
		Anti_soul_Mysteries_Lab_1_Address, Anti_soul_Mysteries_Lab_2_Address, Anti_soul_Mysteries_Lab_3_Address,
		Garden_forgotten_by_Time_1_Address, Garden_forgotten_by_Time_2_Address, Garden_forgotten_by_Time_3_Address, Garden_forgotten_by_Time_4_Address,
		House_of_Sacred_Remains_1_Address, House_of_Sacred_Remains_2_Address, House_of_Sacred_Remains_3_Address,
		Dark_Palace_of_Waterfall_1_Address, /* Dark_Palace_of_Waterfall_2_Address, */ Dark_Palace_of_Waterfall_3_Address,
		Ghostly_Theatre_1_Address, Ghostly_Theatre_2_Address, Ghostly_Theatre_3_Address,
	};
	int Save_Room_Addresses_LEN = *(&Save_Room_Addresses + 1) - Save_Room_Addresses;
	long long Save_Room_Data[] = {
		Prelude_to_the_Dark_Abyss_Data, Prison_of_Eternal_Torture_Data, Pagoda_of_the_Misty_Moon_Data,
		Anti_soul_Mysteries_Lab_1_Data, Anti_soul_Mysteries_Lab_2_Data, Anti_soul_Mysteries_Lab_3_Data,
		Garden_forgotten_by_Time_1_Data, Garden_forgotten_by_Time_2_Data, Garden_forgotten_by_Time_3_Data, Garden_forgotten_by_Time_4_Data,
		House_of_Sacred_Remains_1_Data, House_of_Sacred_Remains_2_Data, House_of_Sacred_Remains_3_Data,
		Dark_Palace_of_Waterfall_1_Data, /* Dark_Palace_of_Waterfall_2_Data, */ Dark_Palace_of_Waterfall_3_Data,
		Ghostly_Theatre_1_Data, Ghostly_Theatre_2_Data, Ghostly_Theatre_3_Data,
	};		
	
	int randVal;
	long long newByte;
	 
	for(int i = 0; i < Save_Room_Addresses_LEN; i++)
	{
		do
		{
			randVal = rand() % Save_Room_Addresses_LEN;
		}while(Save_Room_Data[randVal] == 0xFFFFFFFFFFFFFFFF);
		
			fseek(fp,Save_Room_Addresses[i],SEEK_SET);
			newByte = Save_Room_Data[randVal];
			fwrite(&newByte,sizeof(newByte),1,fp);
			
		Save_Room_Data[randVal] = 0xFFFFFFFFFFFFFFFF;
	}
}
void randomize_orbs_model_and_sprites(FILE* fp)
{
	int red_orb_model_color_address = 0x3DC570;
	int red_orb_sprite_address = 0x3DC568;
	int red_orb_color = 0x01;
	int red_orb_sprite = 0x54;
	
	int blue_orb_model_color_address = 0x3DC5B8;
	int blue_orb_sprite_address = 0x3DC5B0;
	int blue_orb_color = 0x02;
	int blue_orb_sprite = 0x55;
	
	int yellow_orb_model_color_address = 0x3DC600;
	int yellow_orb_sprite_address = 0x3DC5F8;
	int yellow_orb_color = 0x03;
	int yellow_orb_sprite = 0x56;
	
	int green_orb_model_color_address = 0x3DC648;
	int green_orb_sprite_address = 0x3DC640;
	int green_orb_color = 0x04;
	int green_orb_sprite = 0x57;
	
	int purple_orb_model_color_address = 0x3DC690;
	int purple_orb_sprite_address = 0x3DC688;
	int purple_orb_color = 0x05;
	int purple_orb_sprite = 0x58;
	
	int white_orb_model_color_address = 0x3DC6D8;
	int white_orb_sprite_address = 0x3DC6D0;
	int white_orb_color = 0x00;
	int white_orb_sprite = 0x59;
	
	int black_orb_model_color_address = 0x3DC720;
	int black_orb_sprite_address = 0x3DC718;
	int black_orb_color = 0x06;
	int black_orb_sprite = 0x5A;
	
	int orbs_models_and_sprites_addresses[] = {
		red_orb_model_color_address,red_orb_sprite_address,
		blue_orb_model_color_address,blue_orb_sprite_address,
		yellow_orb_model_color_address,yellow_orb_sprite_address,
		green_orb_model_color_address,green_orb_sprite_address,
		purple_orb_model_color_address,purple_orb_sprite_address,
		white_orb_model_color_address,white_orb_sprite_address,
		black_orb_model_color_address,black_orb_sprite_address,
	};
	int orbs_colors_and_sprites[] = {
		red_orb_color,red_orb_sprite,
		blue_orb_color,blue_orb_sprite,
		yellow_orb_color,yellow_orb_sprite,
		green_orb_color,green_orb_sprite,
		purple_orb_color,purple_orb_sprite,
		white_orb_color,white_orb_sprite,
		black_orb_color,black_orb_sprite,
	};
	
	int randVal = 0;
	
	for(int i = 0; i < 13; i+=2)
	{
		do
		{
			randVal = rand() % 7;
			randVal *= 2;
		}while(orbs_colors_and_sprites[randVal] == 0xFFFF);
		
		fseek(fp,orbs_models_and_sprites_addresses[i],SEEK_SET);
		fwrite(&orbs_colors_and_sprites[randVal],sizeof(orbs_colors_and_sprites[randVal]),1,fp);
		fseek(fp,orbs_models_and_sprites_addresses[i+1],SEEK_SET);
		fwrite(&orbs_colors_and_sprites[randVal+1],sizeof(orbs_colors_and_sprites[randVal+1]),1,fp);
		
		orbs_colors_and_sprites[randVal] = 0xFFFF;
		orbs_colors_and_sprites[randVal + 1] = 0xFFFF;
		
	}
}

/*
void call_randomizer(FILE* fp,FILE* fptr)
{
	char player_chooses[] = {'0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0'};
	fflush(stdin);
	do
	{
		if(player_chooses[0] == 'Y' || player_chooses[0] == 'N')
			break;
		printf("Do you want the boss loadzones randomized? Y/N \n");
		scanf("%c",&player_chooses[0]);
	}while(player_chooses[0] != 'Y' || player_chooses[0] != 'N');
	if(player_chooses[0] == 'Y')
		randomize_boss_loadzone(fp);
		//printf("boss_loadzone \n");
	randomize_orbs_and_whips(fp); //REQUIRE
	printf("items from bosses randomized \n");
	fflush(stdin);
	do
	{
		if(player_chooses[1] == 'Y' || player_chooses[1] == 'N')
			break;
		printf("Do you want the enemy HP randomized? Y/N \n");
		scanf("%c",&player_chooses[1]);
	}while(player_chooses[1] != 'Y' || player_chooses[1] != 'N');
	if(player_chooses[1] == 'Y')
		randomize_enemy_HP(fp);
		//printf("enemy HP \n");
	fflush(stdin);
	do
	{
		if(player_chooses[5] == 'Y' || player_chooses[5] == 'N')
			break;
		printf("Do you want the enemy tolerance/weakness randomized? Y/N \n");
		scanf("%c",&player_chooses[5]);
	}while(player_chooses[5] != 'Y' || player_chooses[5] != 'N');
	if(player_chooses[5] == 'Y')
		randomize_enemy_tolerance_and_or_weaknesses(fp);
		//printf("enemy weakness \n");
	fflush(stdin);
	do
	{
		if(player_chooses[10] == 'Y' || player_chooses[10] == 'N')
			break;
		printf("Do you want to use default items shuffle? Y/N \n");
		scanf("%c",&player_chooses[10]);
	}while(player_chooses[10] != 'Y' || player_chooses[10] != 'N');
	if(player_chooses[10] == 'N')
	{
		custom_item_list_rando(fp);
	}
	else
	{
		randomize_item_locations(fp); //REQUIRE
	}
	printf("item locations randomized \n");
	//printf("item location \n");
	//printf("orbs \n");
	fflush(stdin);
	do
	{
		if(player_chooses[2] == 'Y' || player_chooses[2] == 'N')
			break;
		printf("Do you want the relic MP drain to be randomized? Y/N \n");
		scanf("%c",&player_chooses[2]);
	}while(player_chooses[2] != 'Y' || player_chooses[2] != 'N');
	if(player_chooses[2] == 'Y')
		randomize_relic_MP(fp);
	//printf("relic \n");
	//printf("Key doors might not require the default key \n");
	fflush(stdin);
	do
	{
		if(player_chooses[14] == 'Y' || player_chooses[14] == 'N')
			break;
		printf("Do you want to have the key doors potentially not require their default key? Y/N \n");
		scanf("%c",&player_chooses[14]);
	}while(player_chooses[14] != 'Y' || player_chooses[14] != 'N');
	if(player_chooses[14] == 'Y')
		randomize_key_doors(fp); //REQUIRE
	//printf("key doors \n");
	fflush(stdin);
	do
	{
		if(player_chooses[3] == 'Y' || player_chooses[3] == 'N')
			break;
		printf("Do you want the doppelganger loadzones randomized? Y/N \n");
		scanf("%c",&player_chooses[3]);
	}while(player_chooses[3] != 'Y' || player_chooses[3] != 'N');
	if(player_chooses[3] == 'Y')
		randomize_doppel_fight(fp);
		//printf("doppel fights \n");
	fflush(stdin);
	do
	{
		if(player_chooses[4] == 'Y' || player_chooses[4] == 'N')
			break;
		printf("Do you want the sub-weapon attacks randomized? Y/N \n");
		scanf("%c",&player_chooses[4]);
	}while(player_chooses[4] != 'Y' || player_chooses[4] != 'N');
	if(player_chooses[4] == 'Y')
		randomize_sub_weapon_attacks(fp);
		//printf("subweapon attacks \n");
	fflush(stdin);
	do
	{
		if(player_chooses[6] == 'Y' || player_chooses[6] == 'N')
			break;
		printf("Do you want to start with a random sub-weapon Y/N \n");
		scanf("%c",&player_chooses[6]);
	}while(player_chooses[6] != 'Y' || player_chooses[6] != 'N');
	int start_sub_weapon = 0xAA;
	if(player_chooses[6] == 'Y')
	{
		start_sub_weapon = randomize_start_sub_weapon(fp,fptr);
		//printf("Start sub weapon \n");
	}
	fflush(stdin);
	do
	{
		if(player_chooses[7] == 'Y' || player_chooses[7] == 'N')
			break;
		printf("Do you want to randomize the warp room Y/N \n");
		scanf("%c",&player_chooses[7]);
	}while(player_chooses[7] != 'Y' || player_chooses[7] != 'N');
	if(player_chooses[7] == 'Y')	
		randomize_area_warp_rooms(fp);
		//printf("Warp room randomization \n");
	fflush(stdin);
	do
	{
		if(player_chooses[8] == 'Y' || player_chooses[8] == 'N')
			break;
		printf("Do you want to randomize the Armor DEF Y/N \n");
		scanf("%c",&player_chooses[8]);
	}while(player_chooses[8] != 'Y' || player_chooses[8] != 'N');
	if(player_chooses[8] == 'Y')
		randomize_armor_def(fp);
		//printf("Armor DEF values randomized \n");
	fflush(stdin);
	do
	{
		if(player_chooses[15] == 'Y' || player_chooses[15] == 'N')
			break;
		printf("Do you want to randomize Switch rooms Y/N \n");
		scanf("%c",&player_chooses[15]);
	}while(player_chooses[15] != 'Y' || player_chooses[15] != 'N');
	if(player_chooses[15] == 'Y')
	{
		randomize_switch_rooms(fp); //REQUIRE
		printf("switch rooms are randomized \n");
	}
	fflush(stdin);
	do
	{
		if(player_chooses[9] == 'Y' || player_chooses[9] == 'N')
			break;
		printf("Do you want to randomize sub-weapon heart costs Y/N \n");
		scanf("%c",&player_chooses[9]);
	}while(player_chooses[9] != 'Y' || player_chooses[9] != 'N');
	if(player_chooses[9] == 'Y')
		randomize_sub_weapon_costs(fp);
	fflush(stdin);
	do
	{
		if(player_chooses[11] == 'Y' || player_chooses[11] == 'N')
			break;
		printf("Do you want to decrease starting DEF Y/N \n");
		scanf("%c",&player_chooses[11]);
	}while(player_chooses[11] != 'Y' || player_chooses[11] != 'N');
	if(player_chooses[11] == 'Y')
		randomize_start_DEF(fp);
	fflush(stdin);
	do
	{
		if(player_chooses[13] == 'Y' || player_chooses[13] == 'N')
			break;
		printf("Do you want to randomize some Important models and sprites Y/N \n");
		scanf("%c",&player_chooses[13]);
	}while(player_chooses[13] != 'Y' || player_chooses[13] != 'N');
	if(player_chooses[13] == 'Y')
	{	
		randomize_orbs_model_and_sprites(fp);
		randomize_sub_weapon_model_and_sprites(fp);
	}
	fflush(stdin);
	do
	{
		if(player_chooses[12] == 'Y' || player_chooses[12] == 'N')
			break;
		printf("Do you want hints Y/N \n");
		scanf("%c",&player_chooses[12]);
	}while(player_chooses[12] != 'Y' || player_chooses[12] != 'N');
	if(player_chooses[12] == 'Y')
	{	
		hints_write(fp);
		printf("Wrote Hints \n");
	}
	if(fptr != NULL)
	{
		//printf("write log part 1 \n");
		fprintf(fptr,"Castlevania: Lament of Innocence Randomizer \n \n");
		//printf("Castlevania: Lament of Innocence Randomizer \n \n");
		fprintf(fptr,"Settings: \n");
		fprintf(fptr,"boss loadzones: %c  \n",(char)player_chooses[0]);
		fprintf(fptr,"enemy HP: %c  \n",(char)player_chooses[1]);
		fprintf(fptr,"relic MP: %c  \n",(char)player_chooses[2]);
		fprintf(fptr,"doppelganger loadzones: %c  \n",(char)player_chooses[3]);
		fprintf(fptr,"sub-weapon attacks: %c  \n",(char)player_chooses[4]);
		fprintf(fptr,"enemy tolerance/weakness: ");fprintf(fptr,"%c",player_chooses[5]);fprintf(fptr,"\n");
		fprintf(fptr,"random start sub-weapon: %c  \n",(char)player_chooses[6]);
		fprintf(fptr," starting sub-weapon is: %s \n",itemNames[start_sub_weapon]);
		fprintf(fptr,"warp room: %c  \n",(char)player_chooses[7]);
		fprintf(fptr,"armor DEF: %c  \n",(char)player_chooses[8]);
		fprintf(fptr,"sub-weapon heart costs: %c  \n",(char)player_chooses[9]);
		fprintf(fptr,"default shuffle: ");fprintf(fptr,"%c \n",(char)player_chooses[10]);
		fprintf(fptr,"decrease start DEF: %c  \n",(char)player_chooses[11]);
		fprintf(fptr,"hints: %c  \n",(char)player_chooses[12]);
		fprintf(fptr,"important models/sprites:"); fprintf(fptr,"%c  \n",(char)player_chooses[13]);
		fprintf(fptr,"key doors: %c  \n",(char)player_chooses[14]);
		fprintf(fptr,"switch rooms: %c  \n",(char)player_chooses[15]);
		fprintf(fptr,": %c  \n",(char)player_chooses[16]);
		//printf("end of log part 1 \n");
	}
}
*/
//QoL
void QoL_WolfFoot(FILE* fp)
{
	const int WolfsFootPrice = 0x003DC780;
	int SIZE = 1;
	unsigned char buffer[SIZE];
	int n;
	newByte = 0xFF;
	fseek(fp,WolfsFootPrice+3,SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	newByte = 0xFF;
	fseek(fp,WolfsFootPrice+2,SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	newByte = 0xFF;
	fseek(fp,WolfsFootPrice+1,SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	newByte = 0xFF;
	fseek(fp,WolfsFootPrice+0,SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	
	/*
	for(int i = 0; i < addressArrayLength; i++)
	{
		fseek(fp,AddressArray[i],SEEK_SET);
		n = fread(buffer,sizeof(buffer),1,fp);
		if(buffer[0] == 0x86)
		{
			newByte = 0xA2;
			fseek(fp,AddressArray[i],SEEK_SET);
			fwrite(&newByte,sizeof(newByte),1,fp);
			break;
		}
	}
	*/
	
	const int WolfsFootMPCost = 0x40F35C;
	newByte = 0x00;
	fseek(fp,WolfsFootMPCost,SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	newByte = 0xC0;
	fseek(fp,WolfsFootMPCost+1,SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	newByte = 0xA8;
	fseek(fp,WolfsFootMPCost+2,SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	newByte = 0x46;
	fseek(fp,WolfsFootMPCost+3,SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	
}

void QoL_shop(FILE* fp)
{
	int shopBuyPriceAddress[] = {
			//Armors
		0x003D99B8,
		0x003D9A3C,
		0x003D9AC0,
		0x003D9B44,
		//Relics
		//Accessories
		0x003D9BCC, //Draupnir [10]
		0x003D9C50, //Aroma Earring
		0x003D9CD4, //Qigong Belt
		0x003D9D58, //Coin of Happiness
		0x003D9DDC, //Raccoon Charm
		0x003D9E60, //Bloody Cape
		0x003D9EE4, //Perseus's Ring
		0x003D9F68, //Anti-poison Ring
		0x003D9FEC, //Cleric's Ring
		0x003DA070, //Ring of Fire
		0x003DA0F4, //Artic Ring
		0x003DA178, //Ring of Thunder
		0x003DA1FC, //Talisman
		0x003DA280, //Heart brooch
		0x003DA304, //Mobius's brooch [24]
		0x003DA388, //Piyo-piyo Shoes
		0x003DA40C, //Sacrificial Doll
		0x003DA490, //Magnetic Necklace [27] //???
		0x003DA514, //Assassin Necklace
		0x003DA598, //Jewel Crush
		0x003DA61C, //Megingjord
		0x003DA6A0, //Member Plate
		0x003DA724, //Brisingamen
		0x003DA7A8, //Piko-piko Hammer
		0x003DA82C, //Jade Mask [34]
		//Usable Items
		0x003DAC4C, //Potion [35]
		0x003DAC9C, //High Potion
		0x003DACEC, //Super Potion
		0x003DAD3C, //Heart Repair
		0x003DAD8C, //Mana Prism
		0x003DADDC, //Serum
		0x003DAE2C, //Uncurse Potion
		0x003DAE7C, //Magical Ticket
		0x003DAECC, //Memorial Ticket [43]
		0x003DB1EC, //Diamond [44]
		0x003DB23C, //Ruby
		0x003DB28C, //Sapphire
		0x003DB2DC, //Opal
		0x003DB32C, //Turquoise
		0x003DB37C, //Zircon [49]
		0x003DB41C, //Small Meat [51]
		0x003DB46C, //Big Meat
		0x003DB4BC, //Ramen
		0x003DB50C, //Wine
		0x003DB55C, //Hamburger
		0x003DB5AC, //Shortcake
		0x003DB5FC, //Sushi
		0x003DB64C, //Curry
		0x003DB69C, //Tomato Juice
		0x003DB6EC, //Neapolitan [60]
	};
	int to_add;
	unsigned char high_byte;
	unsigned char low_byte;
	for(int i = 0; i < (*(&shopBuyPriceAddress + 1) - shopBuyPriceAddress); i++)
	{
		randVal = rand() % 20;
		if(randVal == 0 || i == 36 || i == 37)
		{
			randVal = (rand() % 10000) + 1;
			fseek(fp,shopBuyPriceAddress[i],SEEK_SET);
			fwrite(&randVal,sizeof(randVal),1,fp);
			randVal = (int)(randVal / 8); 
			if(i >= 28)
			{
				to_add = 2;
				fseek(fp,shopBuyPriceAddress[i]+to_add,SEEK_SET);
				low_byte = randVal & 0xFF;
				fwrite(&low_byte,sizeof(low_byte),1,fp);
				fseek(fp,shopBuyPriceAddress[i]+to_add+1,SEEK_SET);
				high_byte = (randVal >> 8) & 0xFF;
				fwrite(&high_byte,sizeof(high_byte),1,fp);
			}
			else
			{
				to_add = 4;
				fseek(fp,shopBuyPriceAddress[i]+to_add,SEEK_SET);
				fwrite(&randVal,sizeof(randVal),1,fp);
			}
		}
	}
}
void QoL_food_change(FILE* fp)
{
	int foodItemType[] = {
		0x003DB414,
		0x003DB464,
		0x003DB4B4,
		0x003DB504,
		0x003DB554,
		0x003DB5A4,
		0x003DB5F4,
		0x003DB644,
		0x003DB694,
		0x003DB6E4
	};
	int foodRTWAddress[] = {
		0x11938B0,
		0x11938A0,
		0x1193890,
		0x1193880,
		0x1193870,
		0x1193860,
		0x1193850,
		0x1193840,
		0x1193830,
		0x1193820,
	};
	int foodItemTypeLen = *(&foodItemType + 1) - foodItemType;
	
	int randValue;
	unsigned char newByte;
	char* new_byte;
	for(int i = 0; i < foodItemTypeLen; i++) 
	{
		fseek(fp,foodItemType[i],SEEK_SET);
		randValue = rand() % 5; //this does modular division by 5 meaning it can only have the values of 0,1,2,3,4...there are only 3 item types doing %5 so HP only happens 1/5 the time and the other 2 types happen 2/5s each.
		if(randValue > 2) //3 and 4 are not possible item types
		{
			if(randValue == 3) //check for the first condition that is unacceptable
				randValue = 1; //assign an acceptible value (technically -1 from the value it will have)
			if(randValue == 4) //check for the 2nd condition that is unacceptable
				randValue = 2; //assign an acceptible value (technically -1 from the value it will have)
		}
		randValue += 1; //set the value to what it was +1 (item types are 1 to 3) and %5 gives 0-4 (the range gets adjusted above)
		newByte = (char)randValue; //not an error, but it is going to be used to write 1 byte which should be of type unsigned char, and randValue should be an int. therefore it is better practice to type-cast it.
		fwrite(&newByte,sizeof(newByte),1,fp);
		newByte = 0x00;
		switch(randValue)
		{
			case 1:
				new_byte = "HP ";
				for(int j = 0; new_byte[j] != '\0'; j++)
				{
					fseek(fp,foodRTWAddress[i]+j,SEEK_SET);
					fwrite(&new_byte[j],sizeof(new_byte[j]),1,fp);
				}
				fseek(fp,foodRTWAddress[i]+3,SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
				break;
			case 2:
				new_byte = "<3 ";
				for(int j = 0; new_byte[j] != '\0'; j++)
				{
					fseek(fp,foodRTWAddress[i]+j,SEEK_SET);
					fwrite(&new_byte[j],sizeof(new_byte[j]),1,fp);
				}
				fseek(fp,foodRTWAddress[i]+3,SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
				break;
			case 3:
				new_byte = "MP ";
				for(int j = 0; new_byte[j] != '\0'; j++)
				{
					fseek(fp,foodRTWAddress[i]+j,SEEK_SET);
					fwrite(&new_byte[j],sizeof(new_byte[j]),1,fp);
				}
				fseek(fp,foodRTWAddress[i]+3,SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
				break;
			default:
				break;
		}
	}
}
void QoL_Start(FILE* fp)
{
	//
	const int StartingHearts = 0x3E1E28;
	int SIZE = 1;
	int n;
	unsigned char buffer[SIZE];
	for(int i = 0; i < 4; i++)
	{
		fseek(fp,StartingHearts+i-4,SEEK_SET);
		n = fread(buffer,sizeof(buffer),1,fp);
		newByte = buffer[0];
		fseek(fp,StartingHearts+i,SEEK_SET);
		fwrite(&newByte,sizeof(newByte),1,fp);
	}
	//
	const int StartingGoldAddress = 0x3E1E40;
	int randVal = rand() % 3001;
	fseek(fp,StartingGoldAddress,SEEK_SET);
	fwrite(&randVal,sizeof(randVal),1,fp);
	//
	const int startingMP = 0x3E1E68;
	randValue = rand() % 0x100;
	newByte = randValue;
	fseek(fp,startingMP,SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
	//
}
void QoL_change_item_limits(FILE* fp, char new_byte)
{
	int normal_mode_limit = 0x108E5C;
	int crazy_mode_limit = 0x108E60;
	
	fseek(fp,normal_mode_limit,SEEK_SET);
	fwrite(&new_byte,sizeof(new_byte),1,fp);
	fseek(fp,crazy_mode_limit,SEEK_SET);
	fwrite(&new_byte,sizeof(new_byte),1,fp);
}
void QoL_change_power_ups(FILE* fp,FILE* fptr)
{
	//printf("Do you want to modify the power_ups? \n");
	//printf("This would mean they can randomly increase or decrease your stats between -20 and 20 \n");
	//printf("Y/N \n");
	char player_choose;
	/*
	do
	{
		if(player_choose == 'Y')
			break;
		if(player_choose == 'N')
			return;
		if(player_choose != 'Y' || player_choose != 'N')
			printf("Please choose ' Y ' or ' N ' \n");
		scanf("%s",&player_choose);
	}while(player_choose != 'Y' || player_choose != 'N');
	if(player_choose == 'N')
			return;
	*/
	const int HPUpIncreaseAddress = 0x3DCA4C;
	const int MPUpIncreaseAddress = 0x3DCADC;
	const int HeartUpIncreaseAddress = 0x3DCA94;
	
	//float NEG_ONE = -1.0;
	float new_byte = 0.00;
	
	for(int i = 1; i < 4; i++)
	{
		int randVal = rand() % 41;
		if(randVal > 20)
			randVal -= 41;
		new_byte = randVal;
		
		switch(i)
		{
			case 1:
				fseek(fp,HPUpIncreaseAddress,SEEK_SET);
				if(fptr != NULL)
				{
					fprintf(fptr," HP up amount %d \n",(int)new_byte);
				}
				break;
			case 2:
				fseek(fp,MPUpIncreaseAddress,SEEK_SET);
				if(fptr != NULL)
				{
					fprintf(fptr," MP up amount %d \n",(int)new_byte);
				}
				break;
			case 3:
				fseek(fp,HeartUpIncreaseAddress,SEEK_SET);
				if(fptr != NULL)
				{
					fprintf(fptr," <3 up amount %d \n",(int)new_byte);
				}
				break;
			default:
				break;
		}
		fwrite(&new_byte,sizeof(new_byte),1,fp);
	}	
}
void QoL_compatibility_items_not_drops(FILE* fp, char hintsChoice)
{
	int key_items[] = {
		0x02, 0x03, 0x05, 0x42, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x7F, 0x80, 0x81, 0x82, 0x83, 0x86, 0x8D, 0x8E,	
		//whip of flames, whip of ice, vampire killer, VI tablet, IV tablet, Dragon Crest, E tablet, Unlock Jewel, Curtain Time Bell, Lucifer's Sword, Svarog Statue, Wolf's foot, Red Orb, Blue Orb, Green Orb, Purple Orb, Yellow Orb, White Tiger Key, Yellow Dragon Key, Blue Dragon Key, Red Phoenix Key, Black Turtle Key,
	};
	int key_itemsLEN = *(&key_items + 1) - key_items;
	int dontoverwrite[] = {
		0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x20, 0x21, 0x22, 0x3B, 0x3C, 0x3D, 0x3E, 0x40, 0x42, 
		0x55, 0x56,	0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x7F, 0x80,
		0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 
	};
	int dontoverwriteLEN = *(&dontoverwrite + 1) - dontoverwrite;
	int locations[] =
	{
		//non-Repeatable
			0x20259A60, //Entrance
			0x2044CAE0, //Entrance
			0x2002CF80, //Entrance
			0x09A6A980, //ASML
			0x1745D4E0, //GFbT
			0x05323DE0, //HoSR
			0x23ED6C10, //Theatre
			0x117ECB60, //DPoW
			0x0A0C6B60, //ASML
			//EntrancePotion = 
			0x20259AE0,
			// GFbTPotion = 
			0x14AB1380,
			// DPoWPotion = 
			0x10BD2300,
			// ASMLFlameElementalPotion = 
			0x0B240470,
			// ASMLMegingjordPotion = 
			0x0C556070,
			// HoSR1stPotion = 
			0x0484E010,
			// HoSR2ndPotion = 
			0x049E4D80,
			// Theatre1stPotion = 
			0x23CE4480,
			// Theatre2ndPotion = 
			0x244AE000,
			// HoSRHighPotion = 
			0x02FC9690,
			// PoETHighPotion = 
			0x1F776C60,
			// ASMLHangedManHighPotion = 
			0x0B09D570,
			// ASMLMegingjordHighPotion = 
			0x0C555FF0,
			// TheatreHighPotion = 
			0x21EE6D10,
			// DPoWSuperPotion = 
			0x134CE560,
			// ASMLSuperPotion = 
			0x0C05D210,
			// PotMMSuperPotion = 
			0x1DE6CB60,
			// DPoWHeartRepair = 
			0x12B56990,
			// TheatreHeartRepair = 
			0x21D03C90,
			0x250F8570, //Theatre
			// EntranceSerum = 
			0x2044CB60,
			// HoSRSerum = 
			0x02C297B0,
			// EntranceUncursePotion = 
			0x20804560,
			// HoSRUncursePotion = 
			0x02C29830,
		//Magical Ticket
			0x208045E0,
		//Curtain Time Bell  
			0x233A5CE0,
			// HoSRNeapolitan = 
			0x0319A080,
			// ASMLShortcake = 
			0x0BBD3080,
			// HoSRRamen = 
			0x08017400, //
			// WhiteTigerKey = 
			0x089FB1E0, //HoSR
			// BlueDragonKey = 
			0x124C2060, //DPoW
			// RedPhoenixKey = 
			0x18D2EA60, //GFbT
			// BlackTurtleKey = 
			0x24F26C60, //Theatre
			// YellowDragonKey = 
			0x0A77A3E0, //ASML
			// AncientText2 = 
			0x2245EEE0, //Theatre
			// AncientText1 = 
			0x0BEFB3F0,
			// AncientText3 = 
			0x0C1BC8F0,
			// AncientText4 = 
			0x09D38F00,
			// Map1 = 
			0x2002D000, //Entrance
			// Map2 = 
			0x0D05C770, //ASML
			// Map3 = 
			0x0ECFB510, //DPoW
			// Map4 = 
			0x1599D480, //GFbT
			// Map5 = 
			0x238EC470, //Theatre
		//Event Items
			// ToolBag = 
			0x14467A60,
			// ETablet = 
			0x0B9E9C60,
			// VITablet = 
			0x1EDC0BE0,
			// DragonCrest = 
			0x1EF44160,
			// UnlockJewel = 
			0x1EAE6B60,
			// Svarog Statue = 
			0x209E5880,
			// WolfsFoot = 
			0x0AB0E960,
			// SaiseiIncense = 
			0x198E0B60,
			// BlackBishop = 
			0x08CFE360,
			// LucifersSword = 
			0x231FF460,
			// LittleHammer = 
			0x1233E0E0,
			// MeditativeIncence = 
			0x13337450, //
			// Draupnir = 
			0x13A638E0,
			// AromaEarring = 
			0x22B66E80,
			// RacoonCharm = 
			0x11C532E0,
			// BloodyCape = 
			0x08EA35E0,
			// RingofFire = 
			0x1491E9E0,
			// ArticRing = 
			0x0C34EAE0,
			// RingofThunder = 
			0x18A1DF60,
			// HeartBrooch = 
			0x22EB31E0,
			// JewelCrush = 
			0x172BE1E0,
			// Megingjord = 
			0x0C6C00E0,
			// Brisingamen = 
			0x23539760,
		//Heart Max Up
			// ASMLHPHeartUp = 
			0x0B240370,
			// ASMLFlameElementalHeartUp = 
			0x0B2403F0,
			// DPoWHeartUp1 = 
			0x11C53360, //1st?
			// DPoWHeartUp2 = 
			0x12197CE0, //2nd?
			// GFbTHeartUp = 
			0x19092400,
			// PotMMHeartUp = 
			0x1E1DDA60,
			// TheatreHeartUp = 
			0x22D0CF60,
		//MP Max Up
			// HoSRMPMaxUp = 
			0x088769E0,
			// ASMLMPMaxUp = 
			0x0BD97A80,
			// DPoWMPMaxUp = 
			0x12FD6ED0,
			// TheatreMPMaxUp = 
			0x22646A10,
			// PotMMMPMaxUp = 
			0x1E371160,
		//HP Max Up
			// HoSRHPMaxUp1 = 
			0x02C29730,
			// HoSRHPMaxUp2 = 
			0x086CF560,
			// ASMLHPMaxUp = 
			0x0B3E7EE0,
			// DPoWHPMaxUpBF1 = 
			0x0F9B0880,
			// HoSRHPMaxUpBF2 = 
			0x12004460,
			// TheatreHPMaxUp1 = 
			0x2305A7E0,
			// TheatreHPMaxUp2 = 
			0x23761F00,
			// GFbTHPMaxUp = 
			0x156654E0,
			//// PotMMDoppelHPMaxUp = 0x1C6B9510, //??? could cause problems
			// PotMMHPMaxUp = 
			0x1EC79C60, //
		//$1000
			// HoSR1000 = 
			0x026F22D0, //replace with whip of lightning?
			// ASML1000 = 
			0x0AC7A7E0, //replace with whip of flames?
			// DPoW1000 = 
			0x134CE5E0, //replace with whip of ice?
		//$400
			// GFbT4001 = 
			0x15665560, //replace with red orb?
			// GFbT4002 = 
			0x16343290, //replace with blue orb?
			// GFbT4003 = 
			0x1653B400, //replace with yellow orb?
			// GFbT4004 = 
			0x17120200, //replace with green orb?
			// HoSR400 = 
			0x04B91A10, //replace with purple orb?   

			// ASML4001 = 
			0x09908580, //replace with white bishop?

			// ASML4002 = 
			0x0ADDC6F0, //replace with Sacrificial doll?
			// DPoW4001 = 
			0x12B56890, //replace with Jade Mask?

			// DPoW4002 = 
			0x12B56910, //replace with Diamond?
			
			// Theatre4001 = 
			0x23ED6C90, //replace with earth plate?
			// Theatre4002 = 
			0x240C9D80, //replace with meteor plate?
			// Theatre4003 = 
			0x242B7880, //replace with moonlight plate?
			// Theatre4004 = 
			0x24A24E80, //replace with solar plate?
		//repeatable
		//Torches
		//Knives
			// HoSRKnife = 
			0x0708D580,
			// ASMLKnife = 
			0x0DA607F0,
			// GFbTKnife = 
			0x15FBCB00,
			// PotMMKnife = 
			0x1A6882F0,
			// TheatreKnife = 
			0x26237B70,
		//Axes
			// HoSRAxe = 
			0x0335EA80,
			// ASMLAxe = 
			0x0CED53F0,
			// DPoWFrostElementalAxe = 
			0x10171D80,
			// DPoWBridgeLeverAxe = 
			0x11034C70,
			// GFbTAxe = 
			0x15827A70,
			// PotMMAxe = 
			0x1A688370,
			// TheatreAxe = 
			0x242B5610,
		//Holy Water
			// HoSRHolyWater = 
			0x063C3EF0,
			// GFbTHolyWater =
			0x17778C00,
			// PotMMHolyWater = 
			0x1A6883F0,
			// TheatreHolyWater = 
			0x240C7A90,
		//Crystal
			// HoSRCrystal = 
			0x02DF7310,
			// ASMLCrystal = 
			0x0D05AA70,
			// DPoWCrystal = 
			0x114C3AF0,
			// GFbTCrystal = 
			0x17EC3EF0,
			// PotMMCrystal = 
			0x1A688470,
			// TheatreCrystal = 
			0x265E8270,
		//Cross
			// HoSRCross = 
			0x06703A00,
			// ASMLWhiteOrbCross = 
			0x0D1A24F0,
			// ASML3FCross = 
			0x0DE96970,
			// DPoWCross = 
			0x11636670,
			// PotMMCross = 
			0x1A6884F0,
		//$250
			0x14E46170, //replace with black orb?
	};
	int locationLEN = *(&locations + 1) - locations;
	int drop_addresses[] = {
			//common drops
			0x6FCAE8,
			0x6E7CE8,
			0x776FE8,
			0x7626E8,
			0x6E7AE8,
			0x6EF868,
			0x776BE8,
			0x760DE8,
			0x6E78E8,
			0x6FC8E8,
			0x7618E8,
			0x6EC7E8,
			0x6F02E8,
			0x6EFDE8,
			0x7763E8,
			0x6EFCE8,
			0x761FE8,
			0x6EC5E8,
			0x7619E8,
			0x6F04E8,
			0x7617E8,
			0x6F01E8,
			0x6F03E8,
			0x6F0D68,
			0x6FD368,
			0x6FC7E8,
			0x761BE8,
			0x6FCBE8,
			0x7622E8,
			0x760FE8,
			0x6FD968,
			0x7608E8,
			0x760CE8,
			0x7620E8,
			0x6FD468,
			0x6EC6E8,
			0x761AE8,
			0x6EFFE8,
			0x7764E8,
			0x6F0968,
			0x6F0A68,
			0x6EFEE8,
			0x776AE8,
			0x761CE8,
			0x6FCCE8,
			0x7621E8,
			0x7765E8,
			0x7769E8,
			0x761DE8,
			0x6ECCE8,
			0x7762E8,
			0x7616E8,
			0x6ECBE8,
			0x6FCDE8,
			0x6ECAE8,
			0x7623E8,
			0x6E7BE8,
			0x6EF768,
			0x6F00E8,
			0x761EE8,
			0x6F0B68,
			0x6FD568,
			0x6E79E8,
			0x760EE8,
			0x6ED5E8,
		//rare drops
			0x6FCAEA,
			0x6E7CEA,
			0x776FEA,
			0x7626EA,
			0x6E7AEA,
			0x6EF86A,
			0x776BEA,
			0x760DEA,
			0x6E78EA,
			0x6FC8EA,
			0x7618EA,
			0x6EC7EA,
			0x6F02EA,
			0x6EFDEA,
			0x7763EA,
			0x6EFCEA,
			0x761FEA,
			0x6EC5EA,
			0x7619EA,
			0x6F04EA,
			0x7617EA,
			0x6F01EA,
			0x6F03EA,
			0x6F0D6A,
			0x6FD36A,
			0x6FC7EA,
			0x761BEA,
			0x6FCBEA,
			0x7622EA,
			0x760FEA,
			0x6FD96A,
			0x7608EA,
			0x760CEA,
			0x7620EA,
			0x6FD46A,
			0x6EC6EA,
			0x761AEA,
			0x6EFFEA,
			0x7764EA,
			0x6F096A,
			0x6F0A6A,
			0x6EFEEA,
			0x776AEA,
			0x761CEA,
			0x6FCCEA,
			0x7621EA,
			0x7765EA,
			0x7769EA,
			0x761DEA,
			0x6ECCEA,
			0x7762EA,
			0x7616EA,
			0x6ECBEA,
			0x6FCDEA,
			0x6ECAEA,
			0x7623EA,
			0x6E7BEA,
			0x6EF76A,
			0x6F00EA,
			0x761EEA,
			0x6F0B6A,
			0x6FD56A,
			0x6E79EA,
			0x760EEA,
			0x6ED5EA,
	};
	int drop_addressesLEN = *(&drop_addresses + 1) - drop_addresses;
	int SIZE = 1;
	unsigned char buffer[SIZE];
	int randVal;
	_Bool proceed = false;
	for(int i = 0; i < drop_addressesLEN; i++)
	{
		fseek(fp,drop_addresses[i],SEEK_SET);
		fread(buffer,sizeof(buffer),1,fp);
		for(int n = 0; n < key_itemsLEN; n++)
		{
			if(buffer[0] == key_items[n])
			{
				do
				{
					proceed = true;
					randVal = rand() % locationLEN;
					fseek(fp,locations[randVal],SEEK_SET);
					fread(buffer,sizeof(buffer),1,fp);
					for(int t = 0; t < dontoverwriteLEN; t++)
					{
						if(buffer[0] == dontoverwrite[t])
						{
							for(int e = 0; e < key_itemsLEN; e++)
							{
								if(buffer[0] == key_items[e])
								{
									//can't use address
									proceed = false; 
									break;
								}
							}
						}
						if(!proceed)
							break;
					}
				}while(proceed == false);
				unsigned char temp = buffer[0];
				switch(temp)
				{
					case 0x90:
						temp = 0x2A; //HPups become potions
						break;
					case 0x91:
						temp = 0x2D;
						break;
					case 0x92:
						temp = 0x2E;
						break;
					default:
						break;
				}
				fseek(fp,locations[randVal],SEEK_SET);
				fwrite(&key_items[n],sizeof(key_items[n]),1,fp);
				fseek(fp,drop_addresses[i],SEEK_SET);
				fwrite(&temp,sizeof(temp),1,fp);
				if(key_items[n] == 0x05 || key_items[n] == 0x59 || key_items[n] == 0x5A || key_items[n] == 0x5B || key_items[n] == 0x5C || key_items[n] == 0x5D || key_items[n] == 0x5E || key_items[n] == 0x42 || key_items[n] == 0x55 || key_items[n] == 0x58)
				{
					hints_items(fp,key_items[n],randVal,locationLEN); //give hints for certain items.
				}
			}
		}
	}
	
	if(hintsChoice == 'Y')
	{	
		hints_write(fp);
		printf("Wrote Hints \n");
	}
	/*
	char hintsChoice;
	do
	{
		if(hintsChoice == 'Y' || hintsChoice == 'N')
			break;
		printf("Did you want hints Y/N \n (This is to correct the hints from having stuff moved) \n");
		scanf("%s",&hintsChoice);
	}while(hintsChoice != 'Y' || hintsChoice != 'N');
	if(hintsChoice == 'Y')
	{	
		hints_write(fp);
		printf("Wrote Hints \n");
	}
	*/
}
void QoL_start_skills(FILE* fp,int SkillListA, int SkillListB)
{
	//what skills?
	/*
		0x01
		0x02
		0x04
		0x08
		0x10
		0x20
		0x40
		0x80
	*/
	//0x3E20C4
		/*
		Extension	0b0000 0001
		Draw Up	0b0000 0010 
		Vertical High	0b0000 0100 
		Rising Shot	0b0000 1000 
		Fast Rising	0b0001 0000 
		Spinning Blast	0b0010 0000 
		Energy Blast	0b0100 0000 
		Sonic Edge	0b1000 0000
		*/
	//0x3E20C5
		/*
		A Extension 1	0b0000 0001
		A Extension 2	0b0000 0010 
		Step Attack	0b0000 0100 
		Falcon Claw	0b0000 1000 
		Quick Step	0b0001 0000 
		Quick Step 2	0b0010 0000 
		Perfect Guard	0b0100 0000 
		(NONE)	0b1000 0000
		*/
	int newByte = 0x00;
	char choose = '\0';
	/*
	fflush(stdin);
	do
	{
		if(choose == 'Y' || choose == 'N')
			break;
		printf("Do you want 'Extension' Y/N \n");
		scanf("%c",&choose);
	}while(choose != 'Y' || choose != 'N');
	if(choose == 'Y')
		newByte += 0x01;
	fflush(stdin);
	choose = '\0';
	do
	{
		if(choose == 'Y' || choose == 'N')
			break;
		printf("Do you want 'Draw Up' Y/N \n");
		scanf("%c",&choose);
	}while(choose != 'Y' || choose != 'N');
	if(choose == 'Y')
		newByte += 0x02;
	fflush(stdin);
	choose = '\0';
	do
	{
		if(choose == 'Y' || choose == 'N')
			break;
		printf("Do you want 'Vertical High' Y/N \n");
		scanf("%c",&choose);
	}while(choose != 'Y' || choose != 'N');
	if(choose == 'Y')
		newByte += 0x04;
	fflush(stdin);
	choose = '\0';
	do
	{
		if(choose == 'Y' || choose == 'N')
			break;
		printf("Do you want 'Rising Shot' Y/N \n");
		scanf("%c",&choose);
	}while(choose != 'Y' || choose != 'N');
	if(choose == 'Y')
		newByte += 0x08;
	fflush(stdin);
	choose = '\0';
	do
	{
		if(choose == 'Y' || choose == 'N')
			break;
		printf("Do you want 'Fast Rising' Y/N \n");
		scanf("%c",&choose);
	}while(choose != 'Y' || choose != 'N');
	if(choose == 'Y')
		newByte += 0x10;
	fflush(stdin);
	choose = '\0';
	do
	{
		if(choose == 'Y' || choose == 'N')
			break;
		printf("Do you want 'Spinning Blast' Y/N \n");
		scanf("%c",&choose);
	}while(choose != 'Y' || choose != 'N');
	if(choose == 'Y')
		newByte += 0x20;
	fflush(stdin);
	choose = '\0';
	do
	{
		if(choose == 'Y' || choose == 'N')
			break;
		printf("Do you want 'Energy Blast' Y/N \n");
		scanf("%c",&choose);
	}while(choose != 'Y' || choose != 'N');
	if(choose == 'Y')
		newByte += 0x40;
	fflush(stdin);
	choose = '\0';
	do
	{
		if(choose == 'Y' || choose == 'N')
			break;
		printf("Do you want 'Sonic Edge' Y/N \n");
		scanf("%c",&choose);
	}while(choose != 'Y' || choose != 'N');
	if(choose == 'Y')
		newByte += 0x80;
	fflush(stdin);
	choose = '\0';
	*/
	fseek(fp,0x3E20C4,SEEK_SET);
	fwrite(&SkillListA,sizeof(SkillListA),1,fp);
	newByte = 0x00;
	/*
	do
	{	
		if(choose == 'Y' || choose == 'N')
			break;
		printf("Do you want 'A Extension 1' Y/N \n");
		scanf("%c",&choose);
	}while(choose != 'Y' || choose != 'N');
	if(choose == 'Y')
		newByte += 0x01;
	fflush(stdin);
	choose = '\0';
	do
	{
		if(choose == 'Y' || choose == 'N')
			break;
		printf("Do you want 'A Extension 2' Y/N \n");
		scanf("%c",&choose);
	}while(choose != 'Y' || choose != 'N');
	if(choose == 'Y')
		newByte += 0x02;
	fflush(stdin);
	choose = '\0';
	do
	{
		if(choose == 'Y' || choose == 'N')
			break;
		printf("Do you want 'Step Attack' Y/N \n");
		scanf("%c",&choose);
	}while(choose != 'Y' || choose != 'N');
	if(choose == 'Y')
		newByte += 0x04;
	fflush(stdin);
	choose = '\0';
	do
	{
		if(choose == 'Y' || choose == 'N')
			break;
		printf("Do you want 'Falcon Claw' Y/N \n");
		scanf("%c",&choose);
	}while(choose != 'Y' || choose != 'N');
	if(choose == 'Y')
		newByte += 0x08;
	fflush(stdin);
	choose = '\0';
	do
	{
		if(choose == 'Y' || choose == 'N')
			break;
		printf("Do you want 'Quick Step' Y/N \n");	
		scanf("%c",&choose);
	}while(choose != 'Y' || choose != 'N');
	if(choose == 'Y')
		newByte += 0x10;
	fflush(stdin);
	choose = '\0';
	do
	{
		if(choose == 'Y' || choose == 'N')
			break;
		printf("Do you want 'Quick Step 2' Y/N \n");
		scanf("%c",&choose);
	}while(choose != 'Y' || choose != 'N');
	if(choose == 'Y')
		newByte += 0x20;
	fflush(stdin);
	choose = '\0';
	do
	{
		if(choose == 'Y' || choose == 'N')
			break;
		printf("Do you want 'Perfect Guard' Y/N \n");
		scanf("%c",&choose);
	}while(choose != 'Y' || choose != 'N');
	if(choose == 'Y')
		newByte += 0x40;
	fflush(stdin);
	*/
	fseek(fp,0x3E20C5,SEEK_SET);
	fwrite(&SkillListB,sizeof(SkillListB),1,fp);
	//printf("Do you want '(NONE)' Y/N \n");
}
/*
void call_QoL(FILE* fp,FILE* fptr)
{
	char wolfFootoption;
	fflush(stdin);
	do
	{
		if(wolfFootoption == 'Y' || wolfFootoption == 'N')
			break;
		printf("Do you want Wolf Foot added to shop for $-1 & Wolf Foot MP decreases VERY slowly? Y/N \n");
		scanf("%c",&wolfFootoption);
	}while(wolfFootoption != 'Y' || wolfFootoption != 'N');
	if(wolfFootoption == 'Y')
		QoL_WolfFoot(fp);
	char shopOption;
	fflush(stdin);
	do
	{
		if(shopOption == 'Y' || shopOption == 'N')
			break;
		printf("Do you want the Shop to be modified? Y/N. \n NOTE: turning this off will result in there being no items in the shop (mostly) \n");
		scanf("%c",&shopOption);
	}while(shopOption != 'Y' || shopOption != 'N');	
	if(shopOption == 'Y')
		QoL_shop(fp);
	char foodOption;
	fflush(stdin);
	do
	{
		if(foodOption == 'Y' || foodOption == 'N')
			break;
		printf("Do you want foods to be changed to heal randomly between HP, MP, and Hearts? Y/N \n");
		scanf("%c",&foodOption);
	}while(foodOption != 'Y' || foodOption != 'N');
	if(foodOption == 'Y')
		QoL_food_change(fp);
	char startOption;
	fflush(stdin);
	do
	{
		if(startOption == 'Y' || startOption == 'N')
			break;
		printf("Do you want to start with Gold, MP, and Hearts? Y/N \n");
		scanf("%c",&startOption);
	}while(startOption != 'Y' || startOption != 'N');
	if(startOption == 'Y')	
		QoL_Start(fp);
	printf("Will change the power_up items increases? \n");
	if(fptr != NULL)
	{
		fprintf(fptr,"Power Up settings: \n");
	}
	QoL_change_power_ups(fp,fptr); //question is in the function.
	//printf("Do you want to make key items not be on drops? Y/N \n");
	char choose_key_items_not_drops;
	fflush(stdin);
	do
	{
		if(choose_key_items_not_drops == 'Y' || choose_key_items_not_drops == 'N')
			break;
		printf("Do you want to make key items not be on drops? Y/N \n");
		scanf("%c",&choose_key_items_not_drops);
	}while(choose_key_items_not_drops != 'Y' || choose_key_items_not_drops != 'N');
	if(choose_key_items_not_drops == 'Y')
	{
		fflush(stdin);
		char proceed_value;
		do
		{
			if(proceed_value == 'Y' || proceed_value == 'N')
				break;
			printf("WARNING: this is an experimental feature that might not work and could cause an issue \nDo you wish to proceed? Y/N \n");
			scanf("%c",&proceed_value);
		}while(proceed_value != 'Y' || proceed_value != 'N');
		if(proceed_value == 'Y')
			QoL_compatibility_items_not_drops(fp);
	}
	char startSkills;
	fflush(stdin);
	do
	{
		if(startSkills == 'Y' || startSkills == 'N')
			break;
		printf("Do you want to start with skills? Y/N \n");
		scanf("%c",&startSkills);
	}while(startSkills != 'Y' || startSkills != 'N');
	if(startSkills == 'Y')
		QoL_start_skills(fp);
	if(fptr != NULL)
	{
		//printf("Castlevania: Lament of Innocence Randomizer \n \n");
		fprintf(fptr,"Settings (cont.): \n");
		fprintf(fptr,"QoL Wolf Foot: %c \n",wolfFootoption);
		fprintf(fptr,"QoL shop: %c \n",shopOption);
		fprintf(fptr,"QoL food: %c \n",foodOption);
		fprintf(fptr,"QoL starting Gold,MP,Hearts: %c \n",startOption);
		fprintf(fptr,"QoL starting skills: %c \n",startSkills);
		fprintf(fptr,"QoL key items not drops: %c \n",choose_key_items_not_drops);
		fprintf(fptr,"\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
	}
}
*/
void extra_pumpkin_sub_weapons(FILE* fp)
{
	int subWeaponAbilityAddresses[] = {
		0x4112BC,
		0x4112E8,
		0x411314,
		0x411340,
		0x41136C,
		0x411398,
		0x4113C4,
		0x4113F0,
		0x41141C,
		0x411448,
		0x411474,
		0x4114A0,
		//0x4114CC,
		0x4114F8,
		0x411524,
		0x411550,
		0x41157C,
		0x4115A8,
		0x4115D4,
		0x411600,
		0x41162C,
		0x411658,
		0x411684,
		0x4116B0,
		0x4116DC,
		0x411708,
		0x411734,
		0x411760,
		0x41178C,
		0x4117B8,
		0x4117E4,
		0x411810,
		0x41183C,
		0x411868,
		0x411894,
		0x4118C0,
		0x4118EC,
		0x411918,
		0x411944,
		0x411970,
		0x41199C,
		0x4119C8,
		0x4119F4,
		0x411A20,
		//0x411A4C,
		0x411A78,
		0x411AA4,
		0x411AD0
	};
	unsigned char subWeaponAbilityIDs[] = {
		0x00,
		0x01,
		0x02,
		0x03,
		0x04,
		0x05,
		0x06,
		0x07,
		0x08,
		0x09,
		0x0A,
		0x0B,
		0x0C,
		0x0D,
		0x0E,
		0x0F,
		0x10,
		0x11,
		0x12,
		0x13,
		0x14,
		0x15,
		0x16,
		0x17,
		0x18,
		0x19,
		0x1A,
		0x1B,
		0x1C,
		0x1D,
		0x1E,
		0x1F,
		0x20,
		0x21,
		0x22,
		0x23,
		0x24,
		0x25,
		0x26,
		0x27,
		0x28,
		0x29,
		0x2A,
		0x2B,
		0x2C,
		0x2D,
		0x2E,
		0x2F
	};
	int name_ids[] =
	{
		0x2A4, //[0]
		0x31, //[1]
		0x33, //[2]
		0x34, //[3]
		0x37, //[4]
		0x35, //[5]
		0x32, //[6]
		0x36, //[7]
		0x02A6, //[8]
		0x19, //[9]
		0x15, //[A]
		0x1B, //[B]
		0x1A, //[C]
		0x16, //[D]
		0x17, //[E]
		0x18, //[F]
		0x02A5, //[10]
		0x23, //[11]
		0x26, //[12]
		0x25, //[13]
		0x27, //[14]
		0x24, //[15]
		0x29, //[16]
		0x28, //[17]
		0x02A7, //[18]
		0x1C, //[19]
		0x20, //[1A]
		0x1D, //[1B]
		0x21, //[1C]
		0x22, //[1D]
		0x1F, //[1E]
		0x1E, //[1F]
		0x02A8, //[20]
		0x2A, //[21]
		0x2B, //[22]
		0x2D, //[23]
		0x2F, //[24]
		0x30, //[25]
		0x2C, //[26]
		0x2E, //[27]
		0x02A9, //[28]
		0x38, //[29]
		0x39, //[2A]
		0x3A, //[2B]
		0x3B, //[2C]
		0x3C, //[2D]
		0x3D, //[2E]
		0x3E, //[2F]
	};
	int SIZE = 1;
	unsigned char buffer[SIZE];
	unsigned char new_byte;
	for(int i = 0; i < (*(&subWeaponAbilityAddresses + 1) - subWeaponAbilityAddresses); i++)
	{
		fseek(fp,subWeaponAbilityAddresses[i],SEEK_SET);
		fread(buffer,sizeof(buffer),1,fp);
		switch(buffer[0])
		{
			case 0x00: //Knife
				fseek(fp,subWeaponAbilityAddresses[i],SEEK_SET);
				new_byte = 0x28;
				fwrite(&new_byte,sizeof(new_byte),1,fp);
				break;
			case 0x05: //Magic Missile
				fseek(fp,subWeaponAbilityAddresses[i],SEEK_SET);
				new_byte = 0x2D;
				fwrite(&new_byte,sizeof(new_byte),1,fp);
				break;
			case 0x07: //Spread Gun
				fseek(fp,subWeaponAbilityAddresses[i],SEEK_SET);
				new_byte = 0x2E;
				fwrite(&new_byte,sizeof(new_byte),1,fp);
				break;
			case 0x04: //Blade Serphant
				fseek(fp,subWeaponAbilityAddresses[i],SEEK_SET);
				new_byte = 0x2F;
				fwrite(&new_byte,sizeof(new_byte),1,fp);
				break;
			case 0x0F: //High Speed Edge
				fseek(fp,subWeaponAbilityAddresses[i],SEEK_SET);
				new_byte = 0x2C;
				fwrite(&new_byte,sizeof(new_byte),1,fp);
				break;
			case 0x1B: //Hail Crystal
				fseek(fp,subWeaponAbilityAddresses[i],SEEK_SET);
				new_byte = 0x2A;
				fwrite(&new_byte,sizeof(new_byte),1,fp);
				break;
			case 0x26: //Grand Cross
				fseek(fp,subWeaponAbilityAddresses[i],SEEK_SET);
				new_byte = 0x2B;
				fwrite(&new_byte,sizeof(new_byte),1,fp);
				break;
			default:
				break;
		}
	}
}
void extra_save_room_shuffle(FILE* fp)
{
	
}
void extra_unused_items_in_pool(FILE* fp)
{
	int locations[] =
	{
		//non-Repeatable
			0x20259A60, //Entrance
			0x2044CAE0, //Entrance
			0x2002CF80, //Entrance
			0x09A6A980, //ASML
			0x1745D4E0, //GFbT
			0x05323DE0, //HoSR
			0x23ED6C10, //Theatre
			0x117ECB60, //DPoW
			0x0A0C6B60, //ASML
			//EntrancePotion = 
			0x20259AE0,
			// GFbTPotion = 
			0x14AB1380,
			// DPoWPotion = 
			0x10BD2300,
			// ASMLFlameElementalPotion = 
			0x0B240470,
			// ASMLMegingjordPotion = 
			0x0C556070,
			// HoSR1stPotion = 
			0x0484E010,
			// HoSR2ndPotion = 
			0x049E4D80,
			// Theatre1stPotion = 
			0x23CE4480,
			// Theatre2ndPotion = 
			0x244AE000,
			// HoSRHighPotion = 
			0x02FC9690,
			// PoETHighPotion = 
			0x1F776C60,
			// ASMLHangedManHighPotion = 
			0x0B09D570,
			// ASMLMegingjordHighPotion = 
			0x0C555FF0,
			// TheatreHighPotion = 
			0x21EE6D10,
			// DPoWSuperPotion = 
			0x134CE560,
			// ASMLSuperPotion = 
			0x0C05D210,
			// PotMMSuperPotion = 
			0x1DE6CB60,
			// DPoWHeartRepair = 
			0x12B56990,
			// TheatreHeartRepair = 
			0x21D03C90,
			0x250F8570, //Theatre
			// EntranceSerum = 
			0x2044CB60,
			// HoSRSerum = 
			0x02C297B0,
			// EntranceUncursePotion = 
			0x20804560,
			// HoSRUncursePotion = 
			0x02C29830,
		//Magical Ticket
			0x208045E0,
		//Curtain Time Bell  
			0x233A5CE0,
			// HoSRNeapolitan = 
			0x0319A080,
			// ASMLShortcake = 
			0x0BBD3080,
			// HoSRRamen = 
			0x08017400, //
			// WhiteTigerKey = 
			0x089FB1E0, //HoSR
			// BlueDragonKey = 
			0x124C2060, //DPoW
			// RedPhoenixKey = 
			0x18D2EA60, //GFbT
			// BlackTurtleKey = 
			0x24F26C60, //Theatre
			// YellowDragonKey = 
			0x0A77A3E0, //ASML
			// AncientText2 = 
			0x2245EEE0, //Theatre
			// AncientText1 = 
			0x0BEFB3F0,
			// AncientText3 = 
			0x0C1BC8F0,
			// AncientText4 = 
			0x09D38F00,
			// Map1 = 
			0x2002D000, //Entrance
			// Map2 = 
			0x0D05C770, //ASML
			// Map3 = 
			0x0ECFB510, //DPoW
			// Map4 = 
			0x1599D480, //GFbT
			// Map5 = 
			0x238EC470, //Theatre
		//Event Items
			// ToolBag = 
			0x14467A60,
			// ETablet = 
			0x0B9E9C60,
			// VITablet = 
			0x1EDC0BE0,
			// DragonCrest = 
			0x1EF44160,
			// UnlockJewel = 
			0x1EAE6B60,
			// Svarog Statue = 
			0x209E5880,
			// WolfsFoot = 
			0x0AB0E960,
			// SaiseiIncense = 
			0x198E0B60,
			// BlackBishop = 
			0x08CFE360,
			// LucifersSword = 
			0x231FF460,
			// LittleHammer = 
			0x1233E0E0,
			// MeditativeIncence = 
			0x13337450, //
			// Draupnir = 
			0x13A638E0,
			// AromaEarring = 
			0x22B66E80,
			// RacoonCharm = 
			0x11C532E0,
			// BloodyCape = 
			0x08EA35E0,
			// RingofFire = 
			0x1491E9E0,
			// ArticRing = 
			0x0C34EAE0,
			// RingofThunder = 
			0x18A1DF60,
			// HeartBrooch = 
			0x22EB31E0,
			// JewelCrush = 
			0x172BE1E0,
			// Megingjord = 
			0x0C6C00E0,
			// Brisingamen = 
			0x23539760,
		//Heart Max Up
			// ASMLHPHeartUp = 
			0x0B240370,
			// ASMLFlameElementalHeartUp = 
			0x0B2403F0,
			// DPoWHeartUp1 = 
			0x11C53360, //1st?
			// DPoWHeartUp2 = 
			0x12197CE0, //2nd?
			// GFbTHeartUp = 
			0x19092400,
			// PotMMHeartUp = 
			0x1E1DDA60,
			// TheatreHeartUp = 
			0x22D0CF60,
		//MP Max Up
			// HoSRMPMaxUp = 
			0x088769E0,
			// ASMLMPMaxUp = 
			0x0BD97A80,
			// DPoWMPMaxUp = 
			0x12FD6ED0,
			// TheatreMPMaxUp = 
			0x22646A10,
			// PotMMMPMaxUp = 
			0x1E371160,
		//HP Max Up
			// HoSRHPMaxUp1 = 
			0x02C29730,
			// HoSRHPMaxUp2 = 
			0x086CF560,
			// ASMLHPMaxUp = 
			0x0B3E7EE0,
			// DPoWHPMaxUpBF1 = 
			0x0F9B0880,
			// HoSRHPMaxUpBF2 = 
			0x12004460,
			// TheatreHPMaxUp1 = 
			0x2305A7E0,
			// TheatreHPMaxUp2 = 
			0x23761F00,
			// GFbTHPMaxUp = 
			0x156654E0,
			//// PotMMDoppelHPMaxUp = 0x1C6B9510, //??? could cause problems
			// PotMMHPMaxUp = 
			0x1EC79C60, //
		//$1000
			// HoSR1000 = 
			0x026F22D0, //replace with whip of lightning?
			// ASML1000 = 
			0x0AC7A7E0, //replace with whip of flames?
			// DPoW1000 = 
			0x134CE5E0, //replace with whip of ice?
		//$400
			// GFbT4001 = 
			0x15665560, //replace with red orb?
			// GFbT4002 = 
			0x16343290, //replace with blue orb?
			// GFbT4003 = 
			0x1653B400, //replace with yellow orb?
			// GFbT4004 = 
			0x17120200, //replace with green orb?
			// HoSR400 = 
			0x04B91A10, //replace with purple orb?   

			// ASML4001 = 
			0x09908580, //replace with white bishop?

			// ASML4002 = 
			0x0ADDC6F0, //replace with Sacrificial doll?
			// DPoW4001 = 
			0x12B56890, //replace with Jade Mask?

			// DPoW4002 = 
			0x12B56910, //replace with Diamond?
			
			// Theatre4001 = 
			0x23ED6C90, //replace with earth plate?
			// Theatre4002 = 
			0x240C9D80, //replace with meteor plate?
			// Theatre4003 = 
			0x242B7880, //replace with moonlight plate?
			// Theatre4004 = 
			0x24A24E80, //replace with solar plate?
		//repeatable
		//Torches
		//Knives
			// HoSRKnife = 
			0x0708D580,
			// ASMLKnife = 
			0x0DA607F0,
			// GFbTKnife = 
			0x15FBCB00,
			// PotMMKnife = 
			0x1A6882F0,
			// TheatreKnife = 
			0x26237B70,
		//Axes
			// HoSRAxe = 
			0x0335EA80,
			// ASMLAxe = 
			0x0CED53F0,
			// DPoWFrostElementalAxe = 
			0x10171D80,
			// DPoWBridgeLeverAxe = 
			0x11034C70,
			// GFbTAxe = 
			0x15827A70,
			// PotMMAxe = 
			0x1A688370,
			// TheatreAxe = 
			0x242B5610,
		//Holy Water
			// HoSRHolyWater = 
			0x063C3EF0,
			// GFbTHolyWater =
			0x17778C00,
			// PotMMHolyWater = 
			0x1A6883F0,
			// TheatreHolyWater = 
			0x240C7A90,
		//Crystal
			// HoSRCrystal = 
			0x02DF7310,
			// ASMLCrystal = 
			0x0D05AA70,
			// DPoWCrystal = 
			0x114C3AF0,
			// GFbTCrystal = 
			0x17EC3EF0,
			// PotMMCrystal = 
			0x1A688470,
			// TheatreCrystal = 
			0x265E8270,
		//Cross
			// HoSRCross = 
			0x06703A00,
			// ASMLWhiteOrbCross = 
			0x0D1A24F0,
			// ASML3FCross = 
			0x0DE96970,
			// DPoWCross = 
			0x11636670,
			// PotMMCross = 
			0x1A6884F0,
		//$250
			0x14E46170, //replace with black orb?
		//common drops
			0x6FCAE8,
			0x6E7CE8,
			0x776FE8,
			0x7626E8,
			0x6E7AE8,
			0x6EF868,
			0x776BE8,
			0x760DE8,
			0x6E78E8,
			0x6FC8E8,
			0x7618E8,
			0x6EC7E8,
			0x6F02E8,
			0x6EFDE8,
			0x7763E8,
			0x6EFCE8,
			0x761FE8,
			0x6EC5E8,
			0x7619E8,
			0x6F04E8,
			0x7617E8,
			0x6F01E8,
			0x6F03E8,
			0x6F0D68,
			0x6FD368,
			0x6FC7E8,
			0x761BE8,
			0x6FCBE8,
			0x7622E8,
			0x760FE8,
			0x6FD968,
			0x7608E8,
			0x760CE8,
			0x7620E8,
			0x6FD468,
			0x6EC6E8,
			0x761AE8,
			0x6EFFE8,
			0x7764E8,
			0x6F0968,
			0x6F0A68,
			0x6EFEE8,
			0x776AE8,
			0x761CE8,
			0x6FCCE8,
			0x7621E8,
			0x7765E8,
			0x7769E8,
			0x761DE8,
			0x6ECCE8,
			0x7762E8,
			0x7616E8,
			0x6ECBE8,
			0x6FCDE8,
			0x6ECAE8,
			0x7623E8,
			0x6E7BE8,
			0x6EF768,
			0x6F00E8,
			0x761EE8,
			0x6F0B68,
			0x6FD568,
			0x6E79E8,
			0x760EE8,
			0x6ED5E8,
	};
	int locations_Length = *(&locations + 1) - locations; //define max value of randVal to determine location
	int SIZE = 1;
	unsigned char buffer[SIZE];
	
	float new_byte = 0.15; //drop rate of enemies drops
	unsigned char write_item; 
	unsigned char unused_items[] = {
		0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,
		0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 
		0xA6, 0xA7, 0xA8, 0xA9
	};
	int LEN_unused_items = *(&unused_items + 1) - unused_items;
	for(int i = 0; i < locations_Length; i++)
	{
		fseek(fp,locations[i],SEEK_SET);
		fread(buffer,sizeof(buffer),1,fp);
		//printf("buffer[0] 0x%x \n",buffer[0]);
		if(i != (16 + (locations_Length-65)))
		{
			if(buffer[0] == 0x00)
			{
				randVal = rand() % LEN_unused_items;
				fseek(fp,locations[i],SEEK_SET);
				fwrite(&unused_items[randVal],sizeof(unused_items[randVal]),1,fp);
				if(i > locations_Length - 65)
				{
					fseek(fp,locations[i]+4,SEEK_SET);
					fwrite(&new_byte,sizeof(new_byte),1,fp);
				}
			}
		}
	}
}

_Bool validate_item_optainability(FILE* fp, int checking_item, int complexity)
{
	if(complexity >= 12)
	{
		printf("!Item not obtainable! \n");
		return false;
	}
	if(DEBUG)
	{
		printf("searching for 0x%x %s \n",checking_item,itemNames[checking_item]);
	}
	int locked_locations[] = {
		0x0C6C00E0,
		//megingjord
		0x198E0B60,
		//Saisei Incense
		0x0AB0E960,
		//Wolf's Foot
		0x14467A60,
		//Tool Bag
		0x08CFE360,
		//Black Bishop
		0x13A638E0,
		//Draupnir
		0x22EB31E0,
		//Heart Brooch
		0x1233E0E0,
		//Little Hammer
		0x0A0C6B60,
		//White Orb
		0x1EF44160,
		//Dragon Crest
	};
	char* locked_locations_names[] = {
		//0x0C6C00E0,
		"Megingjord",
		//0x198E0B60,
		"Saisei Incense",
		////0x0AB0E960,
		"Wolf's Foot",
		//0x14467A60,
		"Tool Bag",
		//0x08CFE360,
		"Black Bishop",
		//0x13A638E0,
		"Draupnir",
		//0x22EB31E0,
		"Heart Brooch",
		//0x1233E0E0,
		"Little Hammer",
		//0x0A0C6B60,
		"White Orb",
		//0x1EF44160,
		"Dragon Crest",
	};
		int boss_locked_locations[] = {
		0x0044F590, //golem
		0x44F598, //succubus
	};
	int locked_locations_LEN = *(&locked_locations + 1) - locked_locations;
	int restricted_items[] = {
		0x02, 0x03, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x42, 0x86,
	};
	int restricted_items_LEN = *(&restricted_items + 1) - restricted_items;
	int SIZE = 1;
	unsigned char buffer[SIZE];
	int red_address = 0xD7F5785; //0x03
	//GFbT
	int blue_address = 0x141C2585; //0x02
	//HoSR
	int yellow_address = 0x8A98305; //0x05
	//DPoW
	int black_address = 0x108AA885; //0x04
	//Theatre
	int white_address = 0x27915F85; //0x01
	int doors[] = {red_address, blue_address, yellow_address, black_address, white_address};
	_Bool valid_flag = true;
	char further_tests;
	for(int i = 0; i < locked_locations_LEN; i++)
	{
		fseek(fp,locked_locations[i],SEEK_SET);
		fread(buffer,sizeof(buffer),1,fp);
		if(DEBUG)
		{
			printf("buffer[0]:0x%x, location[%d]:0x%x \n",buffer[0],i,locked_locations[i]);
		}
		if(buffer[0] == checking_item)
		{
			if(DEBUG)
			{
				printf("buffer[0]:0x%x \nchecking_item:0x%x \n",buffer[0],checking_item);
			}
			printf("item needed to unlock a key_item locked \n");
			fflush(stdin);
			do
			{
				if(further_tests == 'Y' || further_tests == 'N')
					break;
				printf("Is this item locked also? \n");
				further_tests = 'Y';
				//scanf("%c",&further_tests);
			}while(further_tests != 'Y' || further_tests != 'N');
			if(further_tests == 'Y')
			{
				switch(locked_locations[i])
				{
					case 0x0C6C00E0:
					//megingjord
					
						checking_item = 0x86; //Wolf's Foot
						break;
					case 0x198E0B60:
					//Saisei Incense
						
						checking_item = 0x86; //Wolf's Foot
						break;
					case 0x0AB0E960:
					//Wolf's Foot
						fseek(fp,red_address,SEEK_SET);
						fread(buffer,sizeof(buffer),1,fp);
						switch(buffer[0])
						{
							case 1: //white key
								checking_item = 0x59; //key
								break;
							case 2: //blue key
								checking_item = 0x5A; //key
								break;
							case 3: //red key
								checking_item = 0x5B; //key
								break;
							case 4: //black key
								checking_item = 0x5C; //key
								break;
							case 5: //yellow key
								checking_item = 0x5D; //key
								break;
							default:
								break;
						}
						break;
					case 0x14467A60:
					//Tool Bag
						fseek(fp,blue_address,SEEK_SET);
						fread(buffer,sizeof(buffer),1,fp);
						switch(buffer[0])
						{
							case 1: //white key
								checking_item = 0x59; //key
								break;
							case 2: //blue key
								checking_item = 0x5A; //key
								break;
							case 3: //red key
								checking_item = 0x5B; //key
								break;
							case 4: //black key
								checking_item = 0x5C; //key
								break;
							case 5: //yellow key
								checking_item = 0x5D; //key
								break;
							default:
								break;
						}
						break;
					case 0x08CFE360:
					//Black Bishop
						fseek(fp,yellow_address,SEEK_SET);
						fread(buffer,sizeof(buffer),1,fp);
						switch(buffer[0])
						{
							case 1: //white key
								checking_item = 0x59; //key
								break;
							case 2: //blue key
								checking_item = 0x5A; //key
								break;
							case 3: //red key
								checking_item = 0x5B; //key
								break;
							case 4: //black key
								checking_item = 0x5C; //key
								break;
							case 5: //yellow key
								checking_item = 0x5D; //key
								break;
							default:
								break;
						}
						break;
					case 0x13A638E0:
					//Draupnir
						fseek(fp,black_address,SEEK_SET);
						fread(buffer,sizeof(buffer),1,fp);
						switch(buffer[0])
						{
							case 1: //white key
								checking_item = 0x59; //key
								break;
							case 2: //blue key
								checking_item = 0x5A; //key
								break;
							case 3: //red key
								checking_item = 0x5B; //key
								break;
							case 4: //black key
								checking_item = 0x5C; //key
								break;
							case 5: //yellow key
								checking_item = 0x5D; //key
								break;
							default:
								break;
						}
						break;
					case 0x22EB31E0:
					//Heart Brooch
						fseek(fp,white_address,SEEK_SET);
						fread(buffer,sizeof(buffer),1,fp);
						switch(buffer[0])
						{
							case 1: //white key
								checking_item = 0x59; //key
								break;
							case 2: //blue key
								checking_item = 0x5A; //key
								break;
							case 3: //red key
								checking_item = 0x5B; //key
								break;
							case 4: //black key
								checking_item = 0x5C; //key
								break;
							case 5: //yellow key
								checking_item = 0x5D; //key
								break;
							default:
								break;
						}
						break;
					case 0x1233E0E0:
					//Little Hammer
						checking_item = 0x02; //whip of flames
						for(int n = 0; n < 2; n++)
						{
							fseek(fp,boss_locked_locations[n],SEEK_SET);
							fread(buffer,sizeof(buffer),1,fp);
							if(buffer[0] == checking_item)
							{
								switch(n)
								{
									case 0:
										checking_item = 0x55; //e tablet
										break;
									case 1:
										checking_item = 0x42; //curtain time bell
										break;
									default:
										break;
								}
							}
						}
						break;
					case 0x0A0C6B60:
					//White Orb
						checking_item = 0x02; //whip of flames
						for(int n = 0; n < 2; n++)
						{
							fseek(fp,boss_locked_locations[n],SEEK_SET);
							fread(buffer,sizeof(buffer),1,fp);
							if(buffer[0] == checking_item)
							{
								switch(n)
								{
									case 0:
										checking_item = 0x55; //e tablet
										break;
									case 1:
										checking_item = 0x42; //curtain time bell
										break;
									default:
										break;
								}
							}
						}
						if(complexity < 12)
							valid_flag = validate_item_optainability(fp,checking_item,complexity+1);
						if(valid_flag)
						{
							checking_item = 0x03; //whip of ice
							for(int n = 0; n < 2; n++)
							{
								fseek(fp,boss_locked_locations[n],SEEK_SET);
								fread(buffer,sizeof(buffer),1,fp);
								if(buffer[0] == checking_item)
								{
									switch(n)
									{
										case 0:
											checking_item = 0x55; //e tablet
											break;
										case 1:
											checking_item = 0x42; //curtain time bell
											break;
										default:
											break;
									}
								}
							}
						}
						else
						{
							return false;
						}
						break;
					case 0x1EF44160:
						checking_item = 0x56; //VI tablet
					//Dragon Crest
						break;
					default:
						break;
				}
				if(complexity < 12)
					valid_flag = validate_item_optainability(fp,checking_item,complexity+1);
				if(!valid_flag)
					return false;
				//_Bool valid = validate_item_optainability(fp,
			}
			else
			{
				return false;
			}
		}
		else
		{
		}
	}
	if(complexity < 12)
		printf("obtainable \n");
	if(complexity > 12)
		return false;
	return true;
}
_Bool validate_Forgotten_One(FILE* fp)
{
	//figure out what boss is in PoET
	int Forgotten_One_Loadzone = 0x1F51DD70;
	int SIZE = 1;
	unsigned char buffer[SIZE];
	
	fseek(fp,Forgotten_One_Loadzone,SEEK_SET);
	fread(buffer,sizeof(buffer),1,fp);
	int address;
	switch(buffer[0])
	{
		case 0x00: //undead parasite
			address = 0x44F594;
			break;
		case 0x01: //ASML
			fseek(fp,Forgotten_One_Loadzone+2,SEEK_SET);
			fread(buffer,sizeof(buffer),1,fp);
			switch(buffer[0])
			{
				case 0x16: //Flame Elemental 
					address = 0x44F5B0;
					break;
				case 0x0E: //Golem
					address = 0x44F590;
					break;
				default:
					break;
			}
			break;
		case 0x02: //DPoW
			switch(buffer[0])
			{
				case 0x33: //Joachim 
					address = 0x44F5B0;
					break;
				case 0x2C: //Frost Elemental
					address = 0x44F590;
					break;
				default:
					break;
			}
			break;
		case 0x03: //GFbT
			switch(buffer[0])
			{
				case 0x37: //Medusa
					address = 0x44F5B0;
					break;
				case 0x3C: //Thunder Elemental
					address = 0x44F590;
					break;
				default:
					break;
			}
			break;
		case 0x08: //Succubus
			address = 0x44F598;
			break;
		case 0x06: //ForgottenOne
			address = 0x44F5A8;
			break;
		default:
			break;
	}
	//figure out what item the boss has
	fseek(fp,address,SEEK_SET);
	fread(buffer,sizeof(buffer),1,fp);
	switch(address)
	{
		case 0x44F590:
			if(!validate_item_optainability(fp,0x55,0))
				return false;
			break;
		case 0x44F598:
			if(!validate_item_optainability(fp,0x42,0))
				return false;
			break;
		default:
			break;
	}
	//check for flame and ice whips
	if(buffer[0] == 0x02 || buffer[0] == 0x03)
		return false;
	return true;
}

void show_debug_info(int item, int location, const char *itemName, const char *locationName) {
    GtkWidget *dialog;
    GtkWidget *parent = NULL; // Set this if you have a parent window

    // Create a formatted message
    char message[256];
    snprintf(message, sizeof(message),
             "Item 0x%x is at 0x%x\n%s is on %s",
             item, location, itemName, locationName);

    // Create a message dialog
    dialog = gtk_message_dialog_new(GTK_WINDOW(parent),
                                    GTK_DIALOG_MODAL,
                                    GTK_MESSAGE_INFO,
                                    GTK_BUTTONS_OK,
                                    "%s", message);

    gtk_window_set_title(GTK_WINDOW(dialog), "Debug Info");

    // Run the dialog and wait for user to close it
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog); // Close the dialog
}

void check_seed(FILE* fp)
{
	//Svarog Statue, Wolf's Foot, Red Phoenix Key, Blue Dragon Key, Yellow Dragon Key, Black Turtle Key, White Tiger Key, E tablet, Curtain Time Bell, Unlock Jewel, Dragon Crest, VI Tablet, Lucifer's Sword
	//Golden Knight Common Drop, Golden Knight Rare Drop, Megingjord, Saisei Incense, Wolf's Foot, Tool Bag, Black Bishop, Draupnir, Heart Brooch, Little Hammer, White Orb, Dragon Crest, 
	int locked_locations[] = {
		0x0C6C00E0,
		//megingjord
		0x198E0B60,
		//Saisei Incense
		0x0AB0E960,
		//Wolf's Foot
		0x14467A60,
		//Tool Bag
		0x08CFE360,
		//Black Bishop
		0x13A638E0,
		//Draupnir
		0x22EB31E0,
		//Heart Brooch
		0x1233E0E0,
		//Little Hammer
		0x0A0C6B60,
		//White Orb
		0x1EF44160,
		//Dragon Crest
		
		//bosses
		
	};
	int boss_locked_locations[] = {
		0x0044F590, //golem
		0x44F598, //succubus
	};
	char* locked_locations_names[] = {
		//0x0C6C00E0,
		"Megingjord",
		//0x198E0B60,
		"Saisei Incense",
		////0x0AB0E960,
		"Wolf's Foot",
		//0x14467A60,
		"Tool Bag",
		//0x08CFE360,
		"Black Bishop",
		//0x13A638E0,
		"Draupnir",
		//0x22EB31E0,
		"Heart Brooch",
		//0x1233E0E0,
		"Little Hammer",
		//0x0A0C6B60,
		"White Orb",
		//0x1EF44160,
		"Dragon Crest",
		
		//bosses
	};
	int locked_locations_LEN = *(&locked_locations + 1) - locked_locations;
	int restricted_items[] = {
		0x02, 0x03, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x42, 0x86,
	};
	int restricted_items_LEN = *(&restricted_items + 1) - restricted_items;
	int SIZE = 1;
	unsigned char buffer[SIZE];
	//ASML
	int red_address = 0xD7F5785; //0x03
	//GFbT
	int blue_address = 0x141C2585; //0x02
	//HoSR
	int yellow_address = 0x8A98305; //0x05
	//DPoW
	int black_address = 0x108AA885; //0x04
	//Theatre
	int white_address = 0x27915F85; //0x01
	int doors[] = {red_address, blue_address, yellow_address, black_address, white_address};
	_Bool valid_flag = true;
	int checking_item = 0x00;
	for(int i = 0; i < locked_locations_LEN; i++)
	{
		fseek(fp,locked_locations[i],SEEK_SET);
		fread(buffer,sizeof(buffer),1,fp);
		for(int n = 0; n < restricted_items_LEN; n++)
		{
			if(buffer[0] == restricted_items[n])
			{
				printf("An 'important item' is on a locked_location check \n");
				if(restricted_items[n] == 0x58)
				{
					if(i == 8 || i == 9)
					{
						if(!validate_Forgotten_One(fp))
							printf("can't get to 'important' locked boss \n");
					}
				}
				printf("checking if the item needed to unlock it is locked. \n");
				switch(locked_locations[i])
				{
					case 0x0C6C00E0:
					//megingjord
					
						checking_item = 0x86; //Wolf's Foot
						break;
					case 0x198E0B60:
					//Saisei Incense
						
						checking_item = 0x86; //Wolf's Foot
						break;
					case 0x0AB0E960:
					//Wolf's Foot
						fseek(fp,red_address,SEEK_SET);
						fread(buffer,sizeof(buffer),1,fp);
						switch(buffer[0])
						{
							case 1: //white key
								checking_item = 0x59; //key
								break;
							case 2: //blue key
								checking_item = 0x5A; //key
								break;
							case 3: //red key
								checking_item = 0x5B; //key
								break;
							case 4: //black key
								checking_item = 0x5C; //key
								break;
							case 5: //yellow key
								checking_item = 0x5D; //key
								break;
							default:
								break;
						}
						break;
					case 0x14467A60:
					//Tool Bag
						fseek(fp,blue_address,SEEK_SET);
						fread(buffer,sizeof(buffer),1,fp);
						switch(buffer[0])
						{
							case 1: //white key
								checking_item = 0x59; //key
								break;
							case 2: //blue key
								checking_item = 0x5A; //key
								break;
							case 3: //red key
								checking_item = 0x5B; //key
								break;
							case 4: //black key
								checking_item = 0x5C; //key
								break;
							case 5: //yellow key
								checking_item = 0x5D; //key
								break;
							default:
								break;
						}
						break;
					case 0x08CFE360:
					//Black Bishop
						fseek(fp,yellow_address,SEEK_SET);
						fread(buffer,sizeof(buffer),1,fp);
						switch(buffer[0])
						{
							case 1: //white key
								checking_item = 0x59; //key
								break;
							case 2: //blue key
								checking_item = 0x5A; //key
								break;
							case 3: //red key
								checking_item = 0x5B; //key
								break;
							case 4: //black key
								checking_item = 0x5C; //key
								break;
							case 5: //yellow key
								checking_item = 0x5D; //key
								break;
							default:
								break;
						}
						break;
					case 0x13A638E0:
					//Draupnir
						fseek(fp,black_address,SEEK_SET);
						fread(buffer,sizeof(buffer),1,fp);
						switch(buffer[0])
						{
							case 1: //white key
								checking_item = 0x59; //key
								break;
							case 2: //blue key
								checking_item = 0x5A; //key
								break;
							case 3: //red key
								checking_item = 0x5B; //key
								break;
							case 4: //black key
								checking_item = 0x5C; //key
								break;
							case 5: //yellow key
								checking_item = 0x5D; //key
								break;
							default:
								break;
						}
						break;
					case 0x22EB31E0:
					//Heart Brooch
						fseek(fp,white_address,SEEK_SET);
						fread(buffer,sizeof(buffer),1,fp);
						switch(buffer[0])
						{
							case 1: //white key
								checking_item = 0x59; //key
								break;
							case 2: //blue key
								checking_item = 0x5A; //key
								break;
							case 3: //red key
								checking_item = 0x5B; //key
								break;
							case 4: //black key
								checking_item = 0x5C; //key
								break;
							case 5: //yellow key
								checking_item = 0x5D; //key
								break;
							default:
								break;
						}
						break;
					case 0x1233E0E0:
					//Little Hammer
						checking_item = 0x02; //whip of flames
						for(int n = 0; n < 2; n++)
						{
							fseek(fp,boss_locked_locations[n],SEEK_SET);
							fread(buffer,sizeof(buffer),1,fp);
							if(buffer[0] == checking_item)
							{
								switch(n)
								{
									case 0:
										checking_item = 0x55; //e tablet
										break;
									case 1:
										checking_item = 0x42; //curtain time bell
										break;
									default:
										break;
								}
							}
						}
						break;
					case 0x0A0C6B60:
					//White Orb
						checking_item = 0x02; //whip of flames
						for(int n = 0; n < 2; n++)
						{
							fseek(fp,boss_locked_locations[n],SEEK_SET);
							fread(buffer,sizeof(buffer),1,fp);
							if(buffer[0] == checking_item)
							{
								switch(n)
								{
									case 0:
										checking_item = 0x55; //e tablet
										break;
									case 1:
										checking_item = 0x42; //curtain time bell
										break;
									default:
										break;
								}
							}
						}
						valid_flag = validate_item_optainability(fp,checking_item,0);
						if(valid_flag)
						{
							checking_item = 0x03; //whip of ice
							for(int n = 0; n < 2; n++)
							{
								fseek(fp,boss_locked_locations[n],SEEK_SET);
								fread(buffer,sizeof(buffer),1,fp);
								if(buffer[0] == checking_item)
								{
									switch(n)
									{
										case 0:
											checking_item = 0x55; //e tablet
											break;
										case 1:
											checking_item = 0x42; //curtain time bell
											break;
										default:
											break;
									}
								}
							}
						}	
						break;
					case 0x1EF44160:
						checking_item = 0x56; //VI tablet
					//Dragon Crest
						break;
					default:
						break;
				}
				valid_flag = validate_item_optainability(fp,checking_item,0);
				if(!valid_flag)
				{
					printf("not obtainable \n");
				}
				else
				{
					printf("\n");
				}
				
				GtkWidget *dialog;
				GtkWidget *parent = NULL; // Set this if you have a parent window
				gint response;
				
				// Create a message dialog
				dialog = gtk_message_dialog_new(GTK_WINDOW(parent),
												GTK_DIALOG_MODAL,
												GTK_MESSAGE_QUESTION,
												GTK_BUTTONS_YES_NO,
												"An 'important item' is on a locked_location check \nDo you want to see what it is?\n(This is mainly a debugging feature)");

				gtk_window_set_title(GTK_WINDOW(dialog), "Debugging Feature");

				// Run the dialog and store the response
				response = gtk_dialog_run(GTK_DIALOG(dialog));
				gtk_widget_destroy(dialog); // Close dialog after getting response

				// Check response
				if (response == GTK_RESPONSE_YES) {
					printf("Debugging enabled!\n");
					DEBUG = true;
				} else {
					DEBUG = false;
				}

				// If DEBUG is true, print debug info make this a pop-up as well?
				if (DEBUG) {
					show_debug_info(restricted_items[n], locked_locations[i], 
									itemNames[restricted_items[n]], locked_locations_names[i]);
					DEBUG = false;
				}
			}
		}
	}	
}

void fun(FILE* fp)
{
	int address = 0x1194170;
	char* text = "Useless.";
	for(int i = 0; text[i] != '\0'; i++)
	{
		fseek(fp,address+i,SEEK_SET);
		fwrite(&text[i],sizeof(text[i]),1,fp);
	}
	newByte = 0x00;
	fwrite(&newByte,sizeof(newByte),1,fp);
	
	//TODO: 
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

void create_Log(FILE* fp, FILE* fptr, int hashed_seed,const char text_seed[])
{
	int locations[] =
	{
		//non-Repeatable
			0x20259A60, //Entrance
			0x2044CAE0, //Entrance
			0x2002CF80, //Entrance
			0x09A6A980, //ASML
			0x1745D4E0, //GFbT
			0x05323DE0, //HoSR
			0x23ED6C10, //Theatre
			0x117ECB60, //DPoW
			0x0A0C6B60, //ASML
			//EntrancePotion = 
			0x20259AE0,
			// GFbTPotion = 
			0x14AB1380,
			// DPoWPotion = 
			0x10BD2300,
			// ASMLFlameElementalPotion = 
			0x0B240470,
			// ASMLMegingjordPotion = 
			0x0C556070,
			// HoSR1stPotion = 
			0x0484E010,
			// HoSR2ndPotion = 
			0x049E4D80,
			// Theatre1stPotion = 
			0x23CE4480,
			// Theatre2ndPotion = 
			0x244AE000,
			// HoSRHighPotion = 
			0x02FC9690,
			// PoETHighPotion = 
			0x1F776C60,
			// ASMLHangedManHighPotion = 
			0x0B09D570,
			// ASMLMegingjordHighPotion = 
			0x0C555FF0,
			// TheatreHighPotion = 
			0x21EE6D10,
			// DPoWSuperPotion = 
			0x134CE560,
			// ASMLSuperPotion = 
			0x0C05D210,
			// PotMMSuperPotion = 
			0x1DE6CB60,
			// DPoWHeartRepair = 
			0x12B56990,
			// TheatreHeartRepair = 
			0x21D03C90,
			0x250F8570, //Theatre
			// EntranceSerum = 
			0x2044CB60,
			// HoSRSerum = 
			0x02C297B0,
			// EntranceUncursePotion = 
			0x20804560,
			// HoSRUncursePotion = 
			0x02C29830,
		//Magical Ticket
			0x208045E0,
		//Curtain Time Bell  
			0x233A5CE0,
			// HoSRNeapolitan = 
			0x0319A080,
			// ASMLShortcake = 
			0x0BBD3080,
			// HoSRRamen = 
			0x08017400, //
			// WhiteTigerKey = 
			0x089FB1E0, //HoSR
			// BlueDragonKey = 
			0x124C2060, //DPoW
			// RedPhoenixKey = 
			0x18D2EA60, //GFbT
			// BlackTurtleKey = 
			0x24F26C60, //Theatre
			// YellowDragonKey = 
			0x0A77A3E0, //ASML
			// AncientText2 = 
			0x2245EEE0, //Theatre
			// AncientText1 = 
			0x0BEFB3F0,
			// AncientText3 = 
			0x0C1BC8F0,
			// AncientText4 = 
			0x09D38F00,
			// Map1 = 
			0x2002D000, //Entrance
			// Map2 = 
			0x0D05C770, //ASML
			// Map3 = 
			0x0ECFB510, //DPoW
			// Map4 = 
			0x1599D480, //GFbT
			// Map5 = 
			0x238EC470, //Theatre
		//Event Items
			// ToolBag = 
			0x14467A60,
			// ETablet = 
			0x0B9E9C60,
			// VITablet = 
			0x1EDC0BE0,
			// DragonCrest = 
			0x1EF44160,
			// UnlockJewel = 
			0x1EAE6B60,
			// Svarog Statue = 
			0x209E5880,
			// WolfsFoot = 
			0x0AB0E960,
			// SaiseiIncense = 
			0x198E0B60,
			// BlackBishop = 
			0x08CFE360,
			// LucifersSword = 
			0x231FF460,
			// LittleHammer = 
			0x1233E0E0,
			// MeditativeIncence = 
			0x13337450, //
			// Draupnir = 
			0x13A638E0,
			// AromaEarring = 
			0x22B66E80,
			// RacoonCharm = 
			0x11C532E0,
			// BloodyCape = 
			0x08EA35E0,
			// RingofFire = 
			0x1491E9E0,
			// ArticRing = 
			0x0C34EAE0,
			// RingofThunder = 
			0x18A1DF60,
			// HeartBrooch = 
			0x22EB31E0,
			// JewelCrush = 
			0x172BE1E0,
			// Megingjord = 
			0x0C6C00E0,
			// Brisingamen = 
			0x23539760,
		//Heart Max Up
			// ASMLHPHeartUp = 
			0x0B240370,
			// ASMLFlameElementalHeartUp = 
			0x0B2403F0,
			// DPoWHeartUp1 = 
			0x11C53360, //1st?
			// DPoWHeartUp2 = 
			0x12197CE0, //2nd?
			// GFbTHeartUp = 
			0x19092400,
			// PotMMHeartUp = 
			0x1E1DDA60,
			// TheatreHeartUp = 
			0x22D0CF60,
		//MP Max Up
			// HoSRMPMaxUp = 
			0x088769E0,
			// ASMLMPMaxUp = 
			0x0BD97A80,
			// DPoWMPMaxUp = 
			0x12FD6ED0,
			// TheatreMPMaxUp = 
			0x22646A10,
			// PotMMMPMaxUp = 
			0x1E371160,
		//HP Max Up
			// HoSRHPMaxUp1 = 
			0x02C29730,
			// HoSRHPMaxUp2 = 
			0x086CF560,
			// ASMLHPMaxUp = 
			0x0B3E7EE0,
			// DPoWHPMaxUpBF1 = 
			0x0F9B0880,
			// HoSRHPMaxUpBF2 = 
			0x12004460,
			// TheatreHPMaxUp1 = 
			0x2305A7E0,
			// TheatreHPMaxUp2 = 
			0x23761F00,
			// GFbTHPMaxUp = 
			0x156654E0,
			//// PotMMDoppelHPMaxUp = 0x1C6B9510, //??? could cause problems
			// PotMMHPMaxUp = 
			0x1EC79C60, //
		//$1000
			// HoSR1000 = 
			0x026F22D0, //replace with whip of lightning?
			// ASML1000 = 
			0x0AC7A7E0, //replace with whip of flames?
			// DPoW1000 = 
			0x134CE5E0, //replace with whip of ice?
		//$400
			// GFbT4001 = 
			0x15665560, //replace with red orb?
			// GFbT4002 = 
			0x16343290, //replace with blue orb?
			// GFbT4003 = 
			0x1653B400, //replace with yellow orb?
			// GFbT4004 = 
			0x17120200, //replace with green orb?
			// HoSR400 = 
			0x04B91A10, //replace with purple orb?   

			// ASML4001 = 
			0x09908580, //replace with white bishop?

			// ASML4002 = 
			0x0ADDC6F0, //replace with Sacrificial doll?
			// DPoW4001 = 
			0x12B56890, //replace with Jade Mask?

			// DPoW4002 = 
			0x12B56910, //replace with Diamond?
			
			// Theatre4001 = 
			0x23ED6C90, //replace with earth plate?
			// Theatre4002 = 
			0x240C9D80, //replace with meteor plate?
			// Theatre4003 = 
			0x242B7880, //replace with moonlight plate?
			// Theatre4004 = 
			0x24A24E80, //replace with solar plate?
		//repeatable
		//Torches
		//Knives
			// HoSRKnife = 
			0x0708D580,
			// ASMLKnife = 
			0x0DA607F0,
			// GFbTKnife = 
			0x15FBCB00,
			// PotMMKnife = 
			0x1A6882F0,
			// TheatreKnife = 
			0x26237B70,
		//Axes
			// HoSRAxe = 
			0x0335EA80,
			// ASMLAxe = 
			0x0CED53F0,
			// DPoWFrostElementalAxe = 
			0x10171D80,
			// DPoWBridgeLeverAxe = 
			0x11034C70,
			// GFbTAxe = 
			0x15827A70,
			// PotMMAxe = 
			0x1A688370,
			// TheatreAxe = 
			0x242B5610,
		//Holy Water
			// HoSRHolyWater = 
			0x063C3EF0,
			// GFbTHolyWater =
			0x17778C00,
			// PotMMHolyWater = 
			0x1A6883F0,
			// TheatreHolyWater = 
			0x240C7A90,
		//Crystal
			// HoSRCrystal = 
			0x02DF7310,
			// ASMLCrystal = 
			0x0D05AA70,
			// DPoWCrystal = 
			0x114C3AF0,
			// GFbTCrystal = 
			0x17EC3EF0,
			// PotMMCrystal = 
			0x1A688470,
			// TheatreCrystal = 
			0x265E8270,
		//Cross
			// HoSRCross = 
			0x06703A00,
			// ASMLWhiteOrbCross = 
			0x0D1A24F0,
			// ASML3FCross = 
			0x0DE96970,
			// DPoWCross = 
			0x11636670,
			// PotMMCross = 
			0x1A6884F0,
		//$250
			0x14E46170, //replace with black orb?
		//common drops
			0x6FCAE8,
			0x6E7CE8,
			0x776FE8,
			0x7626E8,
			0x6E7AE8,
			0x6EF868,
			0x776BE8,
			0x760DE8,
			0x6E78E8,
			0x6FC8E8,
			0x7618E8,
			0x6EC7E8,
			0x6F02E8,
			0x6EFDE8,
			0x7763E8,
			0x6EFCE8,
			0x761FE8,
			0x6EC5E8,
			0x7619E8,
			0x6F04E8,
			0x7617E8,
			0x6F01E8,
			0x6F03E8,
			0x6F0D68,
			0x6FD368,
			0x6FC7E8,
			0x761BE8,
			0x6FCBE8,
			0x7622E8,
			0x760FE8,
			0x6FD968,
			0x7608E8,
			0x760CE8,
			0x7620E8,
			0x6FD468,
			0x6EC6E8,
			0x761AE8,
			0x6EFFE8,
			0x7764E8,
			0x6F0968,
			0x6F0A68,
			0x6EFEE8,
			0x776AE8,
			0x761CE8,
			0x6FCCE8,
			0x7621E8,
			0x7765E8,
			0x7769E8,
			0x761DE8,
			0x6ECCE8,
			0x7762E8,
			0x7616E8,
			0x6ECBE8,
			0x6FCDE8,
			0x6ECAE8,
			0x7623E8,
			0x6E7BE8,
			0x6EF768,
			0x6F00E8,
			0x761EE8,
			0x6F0B68,
			0x6FD568,
			0x6E79E8,
			0x760EE8,
			0x6ED5E8,
		//rare drops
			0x6FCAEA,
			0x6E7CEA,
			0x776FEA,
			0x7626EA,
			0x6E7AEA,
			0x6EF86A,
			0x776BEA,
			0x760DEA,
			0x6E78EA,
			0x6FC8EA,
			0x7618EA,
			0x6EC7EA,
			0x6F02EA,
			0x6EFDEA,
			0x7763EA,
			0x6EFCEA,
			0x761FEA,
			0x6EC5EA,
			0x7619EA,
			0x6F04EA,
			0x7617EA,
			0x6F01EA,
			0x6F03EA,
			0x6F0D6A,
			0x6FD36A,
			0x6FC7EA,
			0x761BEA,
			0x6FCBEA,
			0x7622EA,
			0x760FEA,
			0x6FD96A,
			0x7608EA,
			0x760CEA,
			0x7620EA,
			0x6FD46A,
			0x6EC6EA,
			0x761AEA,
			0x6EFFEA,
			0x7764EA,
			0x6F096A,
			0x6F0A6A,
			0x6EFEEA,
			0x776AEA,
			0x761CEA,
			0x6FCCEA,
			0x7621EA,
			0x7765EA,
			0x7769EA,
			0x761DEA,
			0x6ECCEA,
			0x7762EA,
			0x7616EA,
			0x6ECBEA,
			0x6FCDEA,
			0x6ECAEA,
			0x7623EA,
			0x6E7BEA,
			0x6EF76A,
			0x6F00EA,
			0x761EEA,
			0x6F0B6A,
			0x6FD56A,
			0x6E79EA,
			0x760EEA,
			0x6ED5EA,
	};
	int locations_Length = *(&locations + 1) - locations;
	
	char* locations_Names[] =
	{
			//non-Repeatable
			"MarkerStone1", //0x20259A60, //Entrance
			"MarkerStone2", //0x2044CAE0, //Entrance
			"MarkerStone3", //0x2002CF80, //Entrance
			"MarkerStone4", //0x09A6A980, //ASML
			"MarkerStone5", //0x1745D4E0, //GFbT
			"MarkerStone6", //0x05323DE0, //HoSR
			"MarkerStone7", //0x23ED6C10, //Theatre
			"MarkerStone8", //0x117ECB60, //DPoW
			"WhiteOrb", //0x0A0C6B60, //ASML
			"EntrancePotion", 
			//20259AE0,
			"GFbTPotion", 
			//14AB1380,
			"DPoWPotion", 
			//10BD2300,
			"ASMLElementalPotion", 
			//0B240470,
			"ASMLMegingjordPotion", 
			//0C556070,
			"HoSR1stPotion", 
			//0484E010,
			"HoSR2ndPotion", 
			//049E4D80,
			"Theatre1stPotion", 
			//23CE4480,
			"Theatre2ndPotion", 
			//244AE000,
			"HoSRHighPotion", 
			//02FC9690,
			"PoETHighPotion", 
			//1F776C60,
			"ASMLHangedManHighPotion", 
			//0B09D570,
			"MegingjordHighPotion", 
			//0C555FF0,
			"TheatreHighPotion", 
			//21EE6D10,
			"DPoWSuperPotion", 
			//134CE560,
			"ASMLSuperPotion", 
			//0C05D210,
			"PotMMSuperPotion", 
			//1DE6CB60,
			"DPoWHeartRepair", 
			//12B56990,
			"TheatreHeartRepair", 
			//21D03C90,
			"ManaPrism",
			//250F8570, //Theatre
			"EntranceSerum", 
			//2044CB60,
			"HoSRSerum", 
			//02C297B0,
			"EntranceUncursePotion", 
			//20804560,
			"HoSRUncursePotion", 
			//02C29830,
			"MagicalTicket", 
			//208045E0,
			"CurtainTimeBell",  
			//233A5CE0,
			"HoSRNeapolitan", 
			//0319A080,
			"ASMLShortcake", 
			//0BBD3080,
			"HoSRRamen", 
			//08017400, //
			"WhiteTigerKey", 
			//089FB1E0, //HoSR
			"BlueDragonKey", 
			//124C2060, //DPoW
			"RedPhoenixKey", 
			//18D2EA60, //GFbT
			"BlackTurtleKey", 
			//24F26C60, //Theatre
			"YellowDragonKey", 
			//0A77A3E0, //ASML
			"AncientText2", 
			//2245EEE0, //Theatre
			"AncientText1", 
			//0BEFB3F0,
			"AncientText3", 
			//0C1BC8F0,
			"AncientText4", 
			//09D38F00,
			"Map1", 
			//2002D000, //Entrance
			"Map2", 
			//0D05C770, //ASML
			"Map3", 
			//0ECFB510, //DPoW
			"Map4", 
			//1599D480, //GFbT
			"Map5", 
			//238EC470, //Theatre
		//Event Items
			"ToolBag", 
			//14467A60,
			"ETablet", 
			//0B9E9C60,
			"VITablet", 
			//1EDC0BE0,
			"DragonCrest", 
			//1EF44160,
			"UnlockJewel", 
			//1EAE6B60,
			"Svarog Statue", 
			//209E5880,
			"WolfsFoot", 
			//0AB0E960,
			"SaiseiIncense", 
			//198E0B60,
			"BlackBishop", 
			//08CFE360,
			"LucifersSword", 
			//231FF460,
			"LittleHammer", 
			//1233E0E0,
			"MeditativeIncence", 
			//13337450, //
			"Draupnir", 
			//13A638E0,
			"AromaEarring", 
			//22B66E80,
			"RacoonCharm", 
			//11C532E0,
			"BloodyCape", 
			//08EA35E0,
			"RingofFire", 
			//1491E9E0,
			"ArticRing", 
			//0C34EAE0,
			"RingofThunder", 
			//18A1DF60,
			"HeartBrooch", 
			//22EB31E0,
			"JewelCrush", 
			//172BE1E0,
			"Megingjord", 
			//0C6C00E0,
			"Brisingamen", 
			//23539760,
		//Heart Max Up
			"ASMLHPHeartUp", 
			//0B240370,
			"ASMLElementalHeartUp", 
			//0B2403F0,
			"DPoWHeartUp1", 
			//11C53360, //1st?
			"DPoWHeartUp2", 
			//12197CE0, //2nd?
			"GFbTHeartUp", 
			//19092400,
			"PotMMHeartUp", 
			//1E1DDA60,
			"TheatreHeartUp", 
			//22D0CF60,
		//MP Max Up
			"HoSRMPMaxUp", 
			//088769E0,
			"ASMLMPMaxUp", 
			//0BD97A80,
			"DPoWMPMaxUp", 
			//12FD6ED0,
			"TheatreMPMaxUp", 
			//22646A10,
			"PotMMMPMaxUp", 
			//1E371160,
		//HP Max Up
			"HoSRHPMaxUp1", 
			//02C29730,
			"HoSRHPMaxUp2", 
			//086CF560,
			"ASMLHPMaxUp", 
			//0B3E7EE0,
			"DPoWHPMaxUpBF1", 
			//0F9B0880,
			"HoSRHPMaxUpBF2", 
			//12004460,
			"TheatreHPMaxUp1", 
			//2305A7E0,
			"TheatreHPMaxUp2", 
			//23761F00,
			"GFbTHPMaxUp", 
			//156654E0,
			//"PotMMDoppelHPMaxUp", //1C6B9510, //??? could cause problems
			"PotMMHPMaxUp", 
			//1EC79C60, //
		//$1000
			"HoSR1000", 
			//026F22D0, //replace with whip of lightning?
			"ASML1000", 
			//0AC7A7E0, //replace with whip of flames?
			"DPoW1000", 
			//134CE5E0, //replace with whip of ice?
		//$400
			"GFbT4001", 
			//15665560, //replace with red orb?
			"GFbT4002", 
			//16343290, //replace with blue orb?
			"GFbT4003", 
			//1653B400, //replace with yellow orb?
			"GFbT4004", 
			//17120200, //replace with green orb?
			"HoSR400", 
			//04B91A10, //replace with purple orb?   

			"ASML4001", 
			//09908580, //replace with white bishop?

			"ASML4002", 
			//0ADDC6F0, //replace with Sacrificial doll?
			"DPoW4001", 
			//12B56890, //replace with Jade Mask?

			"DPoW4002", 
			//12B56910, //replace with Diamond?
			
			"Theatre4001", 
			//23ED6C90, //replace with earth plate?
			"Theatre4002", 
			//240C9D80, //replace with meteor plate?
			"Theatre4003", 
			//242B7880, //replace with moonlight plate?
			"Theatre4004", 
			//24A24E80, //replace with solar plate?
		//repeatable
		//Torches
		//Knives
			"HoSRKnife", 
			//0708D580,
			"ASMLKnife", 
			//0DA607F0,
			"GFbTKnife", 
			//15FBCB00,
			"PotMMKnife", 
			//1A6882F0,
			"TheatreKnife", 
			//26237B70,
		//Axes
			"HoSRAxe", 
			//0335EA80,
			"ASMLAxe", 
			//0CED53F0,
			"DPoWFrostElementalAxe", 
			//10171D80,
			"DPoWBridgeLeverAxe", 
			//11034C70,
			"GFbTAxe", 
			//15827A70,
			"PotMMAxe", 
			//1A688370,
			"TheatreAxe", 
			//242B5610,
		//Holy Water
			"HoSRHolyWater", 
			//063C3EF0,
			"GFbTHolyWater",
			//17778C00,
			"PotMMHolyWater", 
			//1A6883F0,
			"TheatreHolyWater", 
			//240C7A90,
		//Crystal
			"HoSRCrystal", 
			//02DF7310,
			"ASMLCrystal", 
			//0D05AA70,
			"DPoWCrystal", 
			//114C3AF0,
			"GFbTCrystal", 
			//17EC3EF0,
			"PotMMCrystal", 
			//1A688470,
			"TheatreCrystal", 
			//265E8270,
		//Cross
			"HoSRCross", 
			//06703A00,
			"ASMLWhiteOrbCross", 
			//0D1A24F0,
			"ASML3FCross", 
			//0DE96970,
			"DPoWCross", 
			//11636670,
			"PotMMCross", 
			//1A6884F0,
			"$250Torch",
			//14E46170, //replace with black orb?
		//common drops
	};
	
	int n;
	int SIZE = 1;
	unsigned char buffer[SIZE];
	
	for(int i = 0; i < locations_Length-130; i++)
	{
		if( i < locations_Length - 130)
		{
			fprintf(fptr,locations_Names[i]);
			//printf(locations_Names[i]);
			fprintf(fptr," = ");
			//printf(" = ");
			fseek(fp,locations[i],SEEK_SET);
			n = fread(buffer,sizeof(buffer),1,fp);
			for(int t = 0; t < SIZE; t++)
			{
				fprintf(fptr,itemNames[buffer[t]]);
				//printf(itemNames[buffer[t]]);
			}
			fprintf(fptr," \n" );
			//printf(" \n");
		} 	
	}
	for(int i = 0; i <= 65; i++)
	{
		fprintf(fptr,enemies[i].name);
		//printf(enemies[i].name);
		fprintf(fptr,"\n common: ");
		//printf("\n common: ");
		fseek(fp,enemies[i].common_item_address,SEEK_SET);
		n = fread(buffer,sizeof(buffer),1,fp);
		for(int t = 0; t < SIZE; t++)
		{
			fprintf(fptr,itemNames[buffer[t]]);
			//printf(itemNames[buffer[t]]);
		}
		fprintf(fptr,"\n");
		//printf("\n");
		fprintf(fptr," rare: ");
		//printf(" rare: ");
		fseek(fp,enemies[i].rare_item_address,SEEK_SET);
		n = fread(buffer,sizeof(buffer),1,fp);
		for(int t = 0; t < SIZE; t++)
		{
			fprintf(fptr,itemNames[buffer[t]]);
			//printf(itemNames[buffer[t]]);
		}
		fprintf(fptr," \n");
		//printf(" \n");
	}
	fprintf(fptr,"\n");
	for(int i =  0x0044F590; i < 0x44F5BB; i+=4)
	{
		switch(i)
		{
			case 0x0044F590:
				fprintf(fptr,"Red Orb : ");
				fseek(fp,i,SEEK_SET);
				fread(buffer,sizeof(buffer),1,fp);
				for(int t = 0; t < SIZE; t++)
				{
					fprintf(fptr,itemNames[buffer[t]]);
				}
				fprintf(fptr,"\n");
				break;
			case 0x0044F594:
				fprintf(fptr,"Blue Orb : ");
				fseek(fp,i,SEEK_SET);
				fread(buffer,sizeof(buffer),1,fp);
				for(int t = 0; t < SIZE; t++)
				{
					fprintf(fptr,itemNames[buffer[t]]);
				}
				fprintf(fptr,"\n");
				break;
			case 0x0044F598:
				fprintf(fptr,"Yellow Orb : ");
				fseek(fp,i,SEEK_SET);
				fread(buffer,sizeof(buffer),1,fp);
				for(int t = 0; t < SIZE; t++)
				{
					fprintf(fptr,itemNames[buffer[t]]);
				}
				fprintf(fptr,"\n");
				break;
			case 0x0044F59C:
				fprintf(fptr,"Green Orb : ");
				fseek(fp,i,SEEK_SET);
				fread(buffer,sizeof(buffer),1,fp);
				for(int t = 0; t < SIZE; t++)
				{
					fprintf(fptr,itemNames[buffer[t]]);
				}
				fprintf(fptr,"\n");
				break;
			case 0x0044F5A0:
				fprintf(fptr,"Purple Orb : ");
				fseek(fp,i,SEEK_SET);
				fread(buffer,sizeof(buffer),1,fp);
				for(int t = 0; t < SIZE; t++)
				{
					fprintf(fptr,itemNames[buffer[t]]);
				}
				fprintf(fptr,"\n");
				break;
			case 0x0044F5A8:
				fprintf(fptr,"Black Orb : ");
				fseek(fp,i,SEEK_SET);
				fread(buffer,sizeof(buffer),1,fp);
				for(int t = 0; t < SIZE; t++)
				{
					fprintf(fptr,itemNames[buffer[t]]);
				}
				fprintf(fptr,"\n");
				break;
			case 0x44F5B0:
				fprintf(fptr,"Whip of Flames : ");
				fseek(fp,i,SEEK_SET);
				fread(buffer,sizeof(buffer),1,fp);
				for(int t = 0; t < SIZE; t++)
				{
					fprintf(fptr,itemNames[buffer[t]]);
				}
				fprintf(fptr,"\n");
				break;
			case 0x44F5B4:
				fprintf(fptr,"Whip of Ice : ");
				fseek(fp,i,SEEK_SET);
				fread(buffer,sizeof(buffer),1,fp);
				for(int t = 0; t < SIZE; t++)
				{
					fprintf(fptr,itemNames[buffer[t]]);
				}
				fprintf(fptr,"\n");
				break;
			case 0x44F5B8:
				fprintf(fptr,"Whip of Lightning : ");
				fseek(fp,i,SEEK_SET);
				fread(buffer,sizeof(buffer),1,fp);
				for(int t = 0; t < SIZE; t++)
				{
					fprintf(fptr,itemNames[buffer[t]]);
				}
				fprintf(fptr,"\n");
				break;
			default:
				break;
		}
	}
	
	fprintf(fptr,"\n");
	for(int i = 0; i < torchesLEN; i++)
	{
		fprintf(fptr,"0x%08X:",torches[i]); //torches[i] is a number of the form 0xABCDEFGH; 
		fseek(fp,torches[i],SEEK_SET);
		fread(buffer,sizeof(buffer),1,fp);
		fprintf(fptr," %s\n",itemNames[buffer[0]]);
	}
	
}

void show_message(const char *message) {
    GtkWidget *dialog;
    dialog = gtk_message_dialog_new(NULL,
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_INFO,
                                    GTK_BUTTONS_OK,
                                    "%s", message);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}


void test_UNUSED4D_function(FILE* fp)
{
	/*
	93020000 93020000
	FD010000 FD010000 
	FD010000 FD010000 
	FD010000 FD010000 
	00000000 00000000 
	00000000 6A4A0000 
	00008C42 00000000 
	FFFFFFFF 01000000 
	00000000 00000000 
	00000000 00000000 
	*/
	/*
	AA020000 AA020000 
	13020000 70010000 
	FD010000 FD010000 
	13020000 70010000 
	21000000 3E000000 
	00000000 6A4A0000 
	00008C42 09000000 
	FFFFFFFF 01000000 
	C8000000 C8006400 
	32000000 0000003F
	*/
	int address_potion = 0x3DAC08;
	int address = 0x3DB6F8;
	unsigned char value[] = {
    0xAA, 0x02, 0x00, 0x00, 0xAA, 0x02, 0x00, 0x00,
    0xCF, 0x01, 0x00, 0x00, 0x70, 0x01, 0x00, 0x00,
    0xFD, 0x01, 0x00, 0x00, 0xFD, 0x01, 0x00, 0x00,
    0xCF, 0x01, 0x00, 0x00, 0x70, 0x01, 0x00, 0x00,
    0x21, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x6A, 0x4A, 0x00, 0x00,
    0x00, 0x00, 0x8C, 0x42, 0x09, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00,
    0xC8, 0x00, 0x00, 0x00, 0xC8, 0x00, 0x64, 0x00,
    0xF1, 0xD8, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x3F
	};
	for(int i = 0; i < 80; i++)
	{
		fseek(fp,address+i,SEEK_SET);
		fwrite(&value[i],sizeof(value[i]),1,fp);
	}
	
	fseek(fp,0x1193E10,SEEK_SET);
	char* text = "DEATH";
	for(int j = 0; text[j] != '\0'; j++)
	{
		fseek(fp,0x1193E10+j,SEEK_SET);
		fwrite(&text[j],sizeof(text[j]),1,fp);
	}
	
	fseek(fp,0x6FD868,SEEK_SET);
	char new_byte = 0x4D;
	fwrite(&new_byte,sizeof(new_byte),1,fp);
	
	float newByte = 1.00;
	fseek(fp,0x6FD86C,SEEK_SET);
	fwrite(&newByte,sizeof(newByte),1,fp);
}
void test_UNUSED23_function(FILE* fp)
{
	/* 
	4C020000 4C020000 
	B7010000 15010000 
	FD010000 FD010000 
	B7010000 15010000 
	15000000 18000000 
	00000000 6A4A0000 
	00008C42 09000000 
	FFFFFFFF 00000000 
	00000000 00000000 
	204E0000 00000000 
	00000000 00000000 
	00000000 00000000 
	00000000 00000000 
	00000000 00000000 
	00000000 00000000 
	00000000 00000000 
	00000000 
	*/
	/*
	93020000 93020000 
	FD010000 FD010000 
	FD010000 FD010000 
	FD010000 FD010000 
	00000000 00000000 
	00000000 6A4A0000 
	00008C42 00000000 
	FFFFFFFF 00000000 
	00000000 00000000 
	00000000 00000000 
	00000000 00000000 
	00000000 00000000 
	00000000 00000000 
	00000000 00000000 
	00000000 00000000 
	00000000 00000000 
	00000000
	*/
	
	int address = 0x3DA8F0;
	unsigned char value[] = {
	0x4C, 0x02, 0x00, 0x00, 0x4C, 0x02, 0x00, 0x00,
    0xB7, 0x01, 0x00, 0x00, 0x15, 0x01, 0x00, 0x00,
    0xFD, 0x01, 0x00, 0x00, 0xFD, 0x01, 0x00, 0x00,
    0xB7, 0x01, 0x00, 0x00, 0x15, 0x01, 0x00, 0x00,
    0x15, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x6A, 0x4A, 0x00, 0x00,
    0x00, 0x00, 0x8C, 0x42, 0x09, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x20, 0x4E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	for(int i = 0; i < 120; i++)
	{
		fseek(fp,address+i,SEEK_SET);
		fwrite(&value[i],sizeof(value[i]),1,fp);
	}
	fseek(fp,0x6EFAE8,SEEK_SET);
	char new_byte = 0x24;
	fwrite(&new_byte,sizeof(new_byte),1,fp);
	
	fseek(fp,0x6EFAEC,SEEK_SET);
	float newByte = 1.00;
	fwrite(&newByte,sizeof(newByte),1,fp);
}

/*
int main()
{
	fflush(stdin);
	char choose;
	
	printf("WARNING: This will overwrite the iso file \n");
	do 
	{
		if(choose == 'N' || choose == 'Y')
			break;
		printf("continue Y/N ? \n");
		scanf("%c",&choose);
	} while (choose != 'Y' || choose != 'N');
	if(choose == 'N')
	{
		printf("bye \n");
		exit(0);
	}
	fflush(stdin);
	//int randVal = 0;
	unsigned int xseed = 0;
	FILE* fp;
	FILE* fptr;
	char seed_string[128];
	printf("Type a seed (number or string): \n"); 
	scanf("%127s",seed_string);
	if(strspn(seed_string, "0123456789") == strlen(seed_string)) {
		//If the input is purely numeric use it directly
		xseed = atoi(seed_string);
		printf("Numeric seed will be: %u\n", xseed);
	} else {
		xseed = hash_string(seed_string);
		printf("String seed hashed to: %u\n", xseed);
	}
	srand(xseed);
	fflush(stdin);
	printf("Type the filepath to the NTSC-U iso version of Castlevania: Lament of Innocence \n(Spaces are NOT allowed; including the filename)\n *If your filename has spaces in it you will need to rename the file to not have spaces before using it here* \n(This is case sensitive) \n");
	char choose2[128];
	scanf("%127s",&choose2);
	fp = fopen(choose2, "rb+");
	//check if file opened successfully 
	if(fp == NULL)
	{
		printf("The file is not opened. The program will now exit. \n");
		exit(1); //failed to open file
	}
	else
	{
		printf("The file opened successfully. \n");
	}
	char log_choose;	
	do
	{
		if(log_choose == 'Y' || log_choose == 'N')
			break;
		printf("do you want to make a spoiler log? Y/N \n!!!THIS IS HIGHLY RECOMMENDED!!!\n");
		scanf("%s",&log_choose);
	}while(log_choose != 'Y' ||	log_choose == 'N');
	if(log_choose == 'Y')
	{
		char log_file[128];
		printf("Where do you want the log saved? \n(Spaces are NOT allowed) \n(This MUST be a .txt file) \n");
		scanf("%s",&log_file);
		fptr = fopen(log_file,"w");
		if(fptr == NULL)
		{
			printf("could not generate log file \n");
		}
	}
	
	call_setup(fp);
	call_randomizer(fp,fptr);
	call_QoL(fp,fptr);
	
		fun(fp);

	
	if(log_choose == 'Y')
	{
		if(fptr != NULL)
		{
			create_Log(fp,fptr,xseed,seed_string);
		}
		else
		{
			printf("could not generate log file \n");
		}
	}
	
	//test_UNUSED4D_function(fp); //Death Potion
	//test_UNUSED23_function(fp); //fake talisman
	
	char check_confirm;
	fflush(stdin);
	do
	{
		if(check_confirm == 'Y' || check_confirm == 'N')
			break;
		printf("Do you want to see if there are any 'important items' on the locked_locations? Y/N \n");
		scanf("%c",&check_confirm);
	}while(check_confirm != 'Y' || check_confirm != 'N');
	if(check_confirm == 'Y')
	{
		check_seed(fp); 
		if(fptr != NULL)
			fprintf(fptr,"\n seed was checked \n");
	}
	fclose(fptr);
	fclose(fp);
	
	char close_input;
	fflush(stdin);
	do
	{
		if(close_input == 'c')
			break;
		printf("Press \' c \' to close \n");
		scanf(" %c",&close_input);
	}while(close_input != 'c');
	
	
	return 0;
}
*/

_Bool betterValidate(FILE* fp, int item_ID)
{
	int locked_locations[] = {
		0x0C6C00E0,
		//megingjord
		0x198E0B60,
		//Saisei Incense
		0x0AB0E960,
		//Wolf's Foot
		0x14467A60,
		//Tool Bag
		0x08CFE360,
		//Black Bishop
		0x13A638E0,
		//Draupnir
		0x22EB31E0,
		//Heart Brooch
		0x1233E0E0,
		//Little Hammer
		0x0A0C6B60,
		//White Orb
		0x1EF44160,
		//Dragon Crest
		
		//bosses	
	};
	int locked_location_LEN = *(&locked_locations + 1) - locked_locations;
	int SIZE = 1;
	unsigned char buffer[SIZE];
	for(int i = 0; i < locked_location_LEN; i++)
	{
		fseek(fp,locked_locations[i],SEEK_SET);
		fread(buffer,sizeof(buffer),1,fp);
		if(buffer[0] == item_ID)
		{
			return false;
		}
	}
	
	return true;
}
//InProgress
void betterChecker(FILE* fp)
{
	int locked_locations[] = {
		0x0C6C00E0,
		//megingjord
		0x198E0B60,
		//Saisei Incense
		0x0AB0E960,
		//Wolf's Foot
		0x14467A60,
		//Tool Bag
		0x08CFE360,
		//Black Bishop
		0x13A638E0,
		//Draupnir
		0x22EB31E0,
		//Heart Brooch
		0x1233E0E0,
		//Little Hammer
		0x0A0C6B60,
		//White Orb
		0x1EF44160,
		//Dragon Crest
		
		//bosses	
	};
	int locked_location_LEN = *(&locked_locations + 1) - locked_locations;
	int boss_locked_locations[] = {
		0x0044F590, //golem
		0x44F598, //succubus
	};
	//ASML
	int red_address = 0xD7F5785; //0x03
	//GFbT
	int blue_address = 0x141C2585; //0x02
	//HoSR
	int yellow_address = 0x8A98305; //0x05
	//DPoW
	int black_address = 0x108AA885; //0x04
	//Theatre
	int white_address = 0x27915F85; //0x01
	int doors[] = {red_address, blue_address, yellow_address, black_address, white_address};
	int SIZE = 1;
	unsigned char buffer[SIZE];
	_Bool obtainable = true;
	for(int i = 0; i < locked_location_LEN; i++)
	{
		fseek(fp,locked_locations[i],SEEK_SET);
		fread(buffer,sizeof(buffer),1,fp);
		if(buffer[0] == 0x05 || buffer[0] == 0x7F || buffer[0] == 0x80 || buffer[0] == 0x81 || buffer[0] == 0x82 || buffer[0] == 0x83 || buffer[0] == 0x5E || buffer[0] == 0x58)
		{
			//printf("required item behind locked location! \n");
			switch(i)
			{
				case 0: 
					obtainable = betterValidate(fp,0x86);
					break;
				case 1:
					obtainable = betterValidate(fp,0x86);
					break;
				case 2: 
					fseek(fp,red_address,SEEK_SET);
					fread(buffer,sizeof(buffer),1,fp);
					switch(buffer[0])
					{
						case 0x03:
							obtainable = betterValidate(fp,0x5B);
							break;
						case 0x02:
							obtainable = betterValidate(fp,0x5A);
							break;
						case 0x05:
							obtainable = betterValidate(fp,0x5D);
							break;
						case 0x04:
							obtainable = betterValidate(fp,0x5C);
							break;
						case 0x01:
							obtainable = betterValidate(fp,0x59);
							break;
						default: 
							break;
					}
					break;
				case 3: 
					fseek(fp,blue_address,SEEK_SET);
					fread(buffer,sizeof(buffer),1,fp);
					switch(buffer[0])
					{
						case 0x03:
							obtainable = betterValidate(fp,0x5B);
							break;
						case 0x02:
							obtainable = betterValidate(fp,0x5A);
							break;
						case 0x05:
							obtainable = betterValidate(fp,0x5D);
							break;
						case 0x04:
							obtainable = betterValidate(fp,0x5C);
							break;
						case 0x01:
							obtainable = betterValidate(fp,0x59);
							break;
						default: 
							break;
					}
					break;
				case 4: 
					fseek(fp,yellow_address,SEEK_SET);
					fread(buffer,sizeof(buffer),1,fp);
					switch(buffer[0])
					{
						case 0x03:
							obtainable = betterValidate(fp,0x5B);
							break;
						case 0x02:
							obtainable = betterValidate(fp,0x5A);
							break;
						case 0x05:
							obtainable = betterValidate(fp,0x5D);
							break;
						case 0x04:
							obtainable = betterValidate(fp,0x5C);
							break;
						case 0x01:
							obtainable = betterValidate(fp,0x59);
							break;
						default: 
							break;
					}
					break;
				case 5: 
					fseek(fp,black_address,SEEK_SET);
					fread(buffer,sizeof(buffer),1,fp);
					switch(buffer[0])
					{
						case 0x03:
							obtainable = betterValidate(fp,0x5B);
							break;
						case 0x02:
							obtainable = betterValidate(fp,0x5A);
							break;
						case 0x05:
							obtainable = betterValidate(fp,0x5D);
							break;
						case 0x04:
							obtainable = betterValidate(fp,0x5C);
							break;
						case 0x01:
							obtainable = betterValidate(fp,0x59);
							break;
						default: 
							break;
					}
					break;
				case 6: 
					fseek(fp,white_address,SEEK_SET);
					fread(buffer,sizeof(buffer),1,fp);
					switch(buffer[0])
					{
						case 0x03:
							obtainable = betterValidate(fp,0x5B);
							break;
						case 0x02:
							obtainable = betterValidate(fp,0x5A);
							break;
						case 0x05:
							obtainable = betterValidate(fp,0x5D);
							break;
						case 0x04:
							obtainable = betterValidate(fp,0x5C);
							break;
						case 0x01:
							obtainable = betterValidate(fp,0x59);
							break;
						default: 
							break;
					}
					break;
				case 7:
					obtainable = betterValidate(fp,0x02);
					if(obtainable)
					{
						for(int j = 0; j < 2; j++)
						{
							fseek(fp,boss_locked_locations[j],SEEK_SET);
							fread(buffer,sizeof(buffer),1,fp);
							if(buffer[0] == 0x02)
							{
								switch(j)
								{
									case 0:
										obtainable = betterValidate(fp,0x59);
										break;
									case 1:
										obtainable = betterValidate(fp,0x42);
										break;
									default:
										break;
								}
							}
						}
					}
					break;
				case 8:
					obtainable = betterValidate(fp,0x02);
					if(obtainable)
					{
						for(int j = 0; j < 2; j++)
						{
							fseek(fp,boss_locked_locations[j],SEEK_SET);
							fread(buffer,sizeof(buffer),1,fp);
							if(buffer[0] == 0x02)
							{
								switch(j)
								{
									case 0:
										obtainable = betterValidate(fp,0x59);
										break;
									case 1:
										obtainable = betterValidate(fp,0x42);
										break;
									default:
										break;
								}
							}
						}
					}
					if(obtainable)
						obtainable = betterValidate(fp,0x03);
					if(obtainable)
					{
						for(int j = 0; j < 2; j++)
						{
							fseek(fp,boss_locked_locations[j],SEEK_SET);
							fread(buffer,sizeof(buffer),1,fp);
							if(buffer[0] == 0x03)
							{
								switch(j)
								{
									case 0:
										obtainable = betterValidate(fp,0x59);
										break;
									case 1:
										obtainable = betterValidate(fp,0x42);
										break;
									default:
										break;
								}
							}
						}
					}
					break;
				default:
					break;
			}
		}
		if(!obtainable)
			break;
	}
	if(obtainable)
	{
		for(int i = 0; i < 2; i++)
		{
			fseek(fp,boss_locked_locations[i],SEEK_SET);
			fread(buffer,sizeof(buffer),1,fp);
			if(buffer[0] == 0x7F || buffer[0] == 0x80 || buffer[0] == 0x81 || buffer[0] == 0x82 || buffer[0] == 0x83)
			{
				switch(i)
				{
					case 0:
						obtainable = betterValidate(fp,0x59);
						break;
					case 1:
						obtainable = betterValidate(fp,0x42);
						break;
					default:
						break;
				}
			}
			if(!obtainable)
				break;
		}
	}
	if(!obtainable)
	{
		show_message("Seed has over sphere 1 complexity, and it might not be completable");
	}
	else
	{
		show_message("Seed completable");
	}
}
int helper_QoL_item_limit_call(GtkWidget *widget, gpointer data, FILE* fp,  GtkWidget **checkboxes)
{
	unsigned char user_input;
	char item_limit_change = 'N';
	CheckBoxData *cb_data = (CheckBoxData *)data;
	
	do {
		GtkWidget *dialog, *content_area, *entry, *label;
		GtkEntryBuffer *buffer;

		dialog = gtk_dialog_new_with_buttons("Enter Quantity",
											 NULL, // No parent window needed
											 GTK_DIALOG_MODAL,
											 "_OK", GTK_RESPONSE_OK,
											 "_Cancel", GTK_RESPONSE_CANCEL,
											 NULL);
		content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

		// Create label and entry field
		label = gtk_label_new("Quantity? (1-255)");
		entry = gtk_entry_new();
		buffer = gtk_entry_get_buffer(GTK_ENTRY(entry));

		// Allow only numerical input
		gtk_entry_set_input_purpose(GTK_ENTRY(entry), GTK_INPUT_PURPOSE_DIGITS);

		// Add widgets to dialog
		gtk_box_pack_start(GTK_BOX(content_area), label, FALSE, FALSE, 5);
		gtk_box_pack_start(GTK_BOX(content_area), entry, FALSE, FALSE, 5);

		gtk_widget_show_all(dialog);

		// Wait for user response
		gint response = gtk_dialog_run(GTK_DIALOG(dialog));

		if (response == GTK_RESPONSE_OK) {
			const gchar *text = gtk_entry_buffer_get_text(buffer);
			user_input = atoi(text); // Convert input to integer

			// Validate input range (1-255)
			if (user_input >= 1 && user_input <= 255) {
				QoL_change_item_limits(fp, (char)user_input);
				item_limit_change = 'Y';
			} else {
				GtkWidget *error_dialog = gtk_message_dialog_new(NULL,
																 GTK_DIALOG_MODAL,
																 GTK_MESSAGE_ERROR,
																 GTK_BUTTONS_OK,
																 "Invalid quantity! Enter a number between 1 and 255.");
				gtk_dialog_run(GTK_DIALOG(error_dialog));
				gtk_widget_destroy(error_dialog);
			}
		} else {
			// If user cancels, uncheck checkbox 22
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkboxes[22]), FALSE);
			user_input = 9; // Set a valid value to exit the loop (9 is the Default)
		}

		gtk_widget_destroy(dialog); // Destroy input dialog

	} while (user_input < 1 || user_input > 255);

	return user_input;
}
static void on_submit(GtkWidget *widget, gpointer data) {
	//g_print("Submitted \n");
    unsigned int xseed = 0;
	char player_chooses[] = {'N','N','N','N','N','N','N','N','N','N','N','N','N','N','N','N','N','N','N','N','N','N','N','N','N','N','N','N','N','N','N','N','N','N','N','N','N','N','N','N','N','N'};
    CheckBoxData *cb_data = (CheckBoxData *)data;

    // Get text from entry fields
    const gchar *fp_FilePath = gtk_entry_get_text(GTK_ENTRY(cb_data->entries[0])); //TODO: allow user to drag-and-drop file for filePath
    const gchar *seed_string = gtk_entry_get_text(GTK_ENTRY(cb_data->entries[1]));
    const gchar *fptr_FilePath = gtk_entry_get_text(GTK_ENTRY(cb_data->entries[2])); //TODO: allow user to drag & drop file for filePath.

    // Open first file
    FILE* fp = fopen(fp_FilePath, "rb+");
	if (fp == NULL) {
		g_print("Error: Could not open file %s\n", fp_FilePath);

		GtkWidget *dialog = gtk_message_dialog_new(
			NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
			"The file '%s' could not be opened.", fp_FilePath
		);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);

		return;
    } else {
        printf("The file '%s' opened successfully.\n", fp_FilePath);
    }

    // Open second file for writing
    FILE* fptr = fopen(fptr_FilePath, "w");
    if (fptr == NULL) {
        printf("Could not generate log file '%s'.\n", fptr_FilePath);
    }

    // Handle seed input
	if (strlen(seed_string) == 0) {
		// Generate a random seed if the field is empty
		xseed = (unsigned int)time(NULL);
		g_print("No seed entered. Generated random seed: %u\n", xseed);
	} else if (strspn(seed_string, "0123456789") == strlen(seed_string)) {
		// If the input is purely numeric, use it directly
		xseed = (unsigned int)atoi(seed_string);
		g_print("Numeric seed will be: %u\n", xseed);
	} else {
		// If input is a string, hash it
		xseed = hash_string(seed_string); // Ensure hash_string() is defined
		g_print("String seed hashed to: %u\n", xseed);
	}

    srand(xseed);

    // Print entry texts
    g_print("File Path 1: %s\n", fp_FilePath);
    g_print("Seed: %s\n", seed_string);
    g_print("File Path 2: %s\n", fptr_FilePath);

	call_setup(fp);

	// Loop through checkboxes and execute relevant actions
	for(int i = 0; i < TOTAL_USED_CHECKBOXES; i++) {
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb_data->checkboxes[i]))) {
			const gchar *label = gtk_button_get_label(GTK_BUTTON(cb_data->checkboxes[i]));
			//g_print("- Selected: %s\n", label);

			// Perform different actions based on checkbox index
			if (i == 0) {
				randomize_boss_loadzone(fp);
				player_chooses[0] = 'Y';
			}
		}
	}

	fun(fp);

	// Ensure this **always** runs
	randomize_orbs_and_whips(fp);
	printf("Items from bosses randomized\n");
	int start_sub_weapon = 0xAA;
	
	char fake_trap_items = 'N';
	
	// Loop again to apply remaining effects (ensuring proper order)
	for(int i = 0; i < TOTAL_CHECKBOXES; i++) {
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb_data->checkboxes[i]))) {
			const gchar *label = gtk_button_get_label(GTK_BUTTON(cb_data->checkboxes[i]));
			//g_print("- Selected: %s\n", label);
			if (i == 1) {
				randomize_enemy_HP(fp);
				player_chooses[1] = 'Y';
			}
			if (i == 2) {
				randomize_enemy_tolerance_and_or_weaknesses(fp);
				player_chooses[5] = 'Y';
			}
			if (i == 3) {
				randomize_item_locations(fp); //REQUIRE
				player_chooses[10] = 'Y';
			}
			if (i == 4) {
				randomize_relic_MP(fp);
				player_chooses[2] = 'Y';
			}
			if (i == 5) {
				randomize_key_doors(fp);
				player_chooses[14] = 'Y';
			}
			if (i == 6) {
				randomize_doppel_fight(fp);
				player_chooses[3] = 'Y';
			}
			if (i == 7) {
				randomize_sub_weapon_attacks(fp);
				player_chooses[4] = 'Y';
			}
			if (i == 8) {
				start_sub_weapon = randomize_start_sub_weapon(fp,fptr);
				player_chooses[6] = 'Y';
			}
			if (i == 9) {
				randomize_area_warp_rooms(fp);
				player_chooses[7] = 'Y';
			}
			if (i == 10) {
				randomize_armor_def(fp);
				player_chooses[8] = 'Y';
			}
			if (i == 11) {
				randomize_switch_rooms(fp);
				player_chooses[15] = 'Y';
			}
			if (i == 12) {
				randomize_sub_weapon_costs(fp);
				player_chooses[9] = 'Y';
			}
			if (i == 13) {
				randomize_start_DEF(fp);
				player_chooses[11] = 'Y';
			}
			if (i == 14) {
				randomize_orbs_model_and_sprites(fp);
				randomize_sub_weapon_model_and_sprites(fp);
				player_chooses[13] = 'Y';
			}
		}
	}
	for(int i = 0; i < TOTAL_CHECKBOXES; i++){
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb_data->checkboxes[i]))) {
			const gchar *label = gtk_button_get_label(GTK_BUTTON(cb_data->checkboxes[i]));
			if(i == 43){
				randomize_orbs_and_whips_to_anywhere(fp);
				player_chooses[19] = 'Y';
			}
			if(i == 44){
				randomize_boss_music(fp);
				player_chooses[20] = 'Y';
			}
			if(i == 46){
				randomize_area_locking(fp);
				player_chooses[22] = 'Y';
			}
		}
	}
	for(int i = 0; i < TOTAL_CHECKBOXES; i++){
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb_data->checkboxes[i]))) {
			
			if (i == 15) {
				hints_write(fp);
				player_chooses[12] = 'Y';
			}
			if (i == 40) {
				//printf("does this work? \n");
				extra_unused_items_in_pool(fp);
				fake_trap_items = 'Y';
				player_chooses[16] = 'Y';
			}
			if( i == 41 ) {
				extra_pumpkin_sub_weapons(fp);
				player_chooses[17] = 'Y';
			}
			if( i == 42 ) {
				randomize_save_rooms(fp);
				player_chooses[18] = 'Y';
			}
			if(i == 45){
				randomize_torchsanity(fp);
				player_chooses[21] = 'Y';
			}
		}
	}
	if(fptr != NULL)
	{
		//printf("write log part 1 \n");
		fprintf(fptr,"Castlevania: Lament of Innocence Randomizer \n \n");
		//printf("Castlevania: Lament of Innocence Randomizer \n \n");
				
		fprintf(fptr,"seed: ");fprintf(fptr,"%s",seed_string);fprintf(fptr,"\n");
		//printf("seed: ");printf("%s",text_seed);printf("\n");
		fprintf(fptr,"seed hashed to: ");fprintf(fptr,"%d",xseed);fprintf(fptr,"\n");
		//printf("seed hashed to: ");printf("%d",hashed_seed);printf("\n");
		fprintf(fptr,"\n\n");
		//printf("\n\n");
		
		fprintf(fptr,"Settings: \n");
		fprintf(fptr,"boss loadzones: %c  \n",(char)player_chooses[0]);
		fprintf(fptr,"enemy HP: %c  \n",(char)player_chooses[1]);
		fprintf(fptr,"relic MP: %c  \n",(char)player_chooses[2]);
		fprintf(fptr,"doppelganger loadzones: %c  \n",(char)player_chooses[3]);
		fprintf(fptr,"sub-weapon attacks: %c  \n",(char)player_chooses[4]);
		fprintf(fptr,"enemy tolerance/weakness: ");fprintf(fptr,"%c",player_chooses[5]);fprintf(fptr,"\n");
		fprintf(fptr,"random start sub-weapon: %c  \n",(char)player_chooses[6]);
		fprintf(fptr," starting sub-weapon is: %s \n",itemNames[start_sub_weapon]);
		fprintf(fptr,"warp room: %c  \n",(char)player_chooses[7]);
		fprintf(fptr,"armor DEF: %c  \n",(char)player_chooses[8]);
		fprintf(fptr,"sub-weapon heart costs: %c  \n",(char)player_chooses[9]);
		fprintf(fptr,"default shuffle: ");fprintf(fptr,"%c \n",(char)player_chooses[10]);
		fprintf(fptr,"decrease start DEF: %c  \n",(char)player_chooses[11]);
		fprintf(fptr,"hints: %c  \n",(char)player_chooses[12]);
		fprintf(fptr,"important models/sprites:"); fprintf(fptr,"%c  \n",(char)player_chooses[13]);
		fprintf(fptr,"key doors: %c  \n",(char)player_chooses[14]);
		fprintf(fptr,"switch rooms: %c  \n",(char)player_chooses[15]);
		fprintf(fptr,"fake/trap items: %c  \n",(char)player_chooses[16]);
		fprintf(fptr,"pumpkin subweapons: %c  \n",(char)player_chooses[17]);
		fprintf(fptr,"Random Save Rooms: %c  \n",(char)player_chooses[18]);
		fprintf(fptr,"Orbs/Whips anywhere: %c \n",(char)player_chooses[19]);
		fprintf(fptr,"music rando: %c \n",(char)player_chooses[20]);
		fprintf(fptr,"torch-sanity: %c \n",(char)player_chooses[21]);
		fprintf(fptr,"area_locking: %c \n",(char)player_chooses[22]);
		//printf("end of log part 1 \n");
	}
	
	char wolfFootoption = 'N';
	char shopOption = 'N';
	char foodOption = 'N';
	char startOption = 'N';
	char startSkills = 'N';
	char choose_key_items_not_drops = 'N';
	int skill_listA = 0x00;
	int skill_listB = 0x00;
	char checked_seed = 'N';
	char item_limit_change = 'N';
	int item_limit = 9;
	int crazy_item_limit = 5;
	
	for (int i = 0; i < TOTAL_USED_CHECKBOXES; i++) {
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb_data->checkboxes[i]))) {
			const gchar *label = gtk_button_get_label(GTK_BUTTON(cb_data->checkboxes[i]));
			g_print("- Selected: %s\n", label);

			// Perform different actions based on checkbox index
			
			if (i == 17) {
				QoL_WolfFoot(fp);
				wolfFootoption = 'Y';
			}
			if (i == 18) {
				QoL_shop(fp);
				shopOption = 'Y';
			}
			if (i == 19) {
				QoL_food_change(fp);
				foodOption = 'Y';
			}
			if (i == 20) {
				QoL_Start(fp);
				startOption = 'Y';
			}
			if (i == 16) {
				if(fptr != NULL)
				{
					fprintf(fptr,"Power Up settings: \n");
				}
				QoL_change_power_ups(fp,fptr);
			}
			if (i == 21) {
				QoL_compatibility_items_not_drops(fp,player_chooses[12]);
				choose_key_items_not_drops = 'Y';
			}
			if (i == 22) {
				int user_input = 0; // Declare outside loop
				item_limit = helper_QoL_item_limit_call(widget, data, fp, cb_data->checkboxes);
				crazy_item_limit = item_limit;
				item_limit_change = 'Y';
			}

			if (i == 23) {
				for (int j = 0; j < TOTAL_USED_CHECKBOXES; j++) {
					if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb_data->checkboxes[j]))) {
						const gchar *label = gtk_button_get_label(GTK_BUTTON(cb_data->checkboxes[j]));
						//g_print("- Selected: %s\n", label);

						// Perform different actions based on checkbox index
						if (j == 24) {
							skill_listA += 0x01;
						}
						if (j == 25) {
							skill_listA += 0x02;
						}
						if (j == 26) {
							skill_listA += 0x04;
						}
						if (j == 27) {
							skill_listA += 0x08;
						}
						if (j == 28) {
							skill_listA += 0x10;
						}
						if (j == 29) {
							skill_listA += 0x20;
						}
						if (j == 30) {
							skill_listA += 0x40;
						}
						if (j == 31) {
							skill_listA += 0x80;
						}
						if (j == 32) {
							skill_listB += 0x01;
						}
						if (j == 33) {
							skill_listB += 0x02;
						}
						if (j == 34) {
							skill_listB += 0x04;
						}
						if (j == 35) {
							skill_listB += 0x08;
						}
						if (j == 36) {
							skill_listB += 0x10;
						}
						if (j == 37) {
							skill_listB += 0x20;
						}
						if (j == 38) {
							skill_listB += 0x40;
						}
					}
				}
				QoL_start_skills(fp,skill_listA,skill_listB);
				startSkills = 'Y';
			}
			if (i == 39) {
				betterChecker(fp); 
				checked_seed == 'Y';
			}
		}
	}
	if(fptr != NULL)
	{
		//printf("Castlevania: Lament of Innocence Randomizer \n \n");
		fprintf(fptr,"Settings (cont.): \n");
		fprintf(fptr,"QoL Wolf Foot: %c \n",wolfFootoption);
		fprintf(fptr,"QoL shop: %c \n",shopOption);
		fprintf(fptr,"QoL food: %c \n",foodOption);
		fprintf(fptr,"QoL starting Gold,MP,Hearts: %c \n",startOption);
		fprintf(fptr,"QoL starting skills: %c \n",startSkills);
		fprintf(fptr,"QoL key items not drops: %c \n",choose_key_items_not_drops);
		fprintf(fptr,"QoL item limit change: %c : limit-%d/crazy-%d\n",item_limit_change,item_limit,crazy_item_limit);
		if(checked_seed == 'Y')
			fprintf(fptr,"\nseed checked \n\n");
		fprintf(fptr,"\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
	}
	if(fptr != NULL)
	{
		create_Log(fp,fptr,xseed,seed_string);
	}
	else
	{
		printf("could not generate log file \n");
	}
    // Close files to avoid leaks
    fclose(fp);
    if (fptr != NULL) fclose(fptr);

    gtk_main_quit(); // Close the window after submission
}

static void on_browse_iso_clicked(GtkButton *button, gpointer user_data) {
    GtkEntry *entry = GTK_ENTRY(user_data);

    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Select Lament of Innocence NTSC-U ISO",
        NULL,
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        NULL
    );

    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, "*.iso");
    gtk_file_filter_set_name(filter, "ISO Files");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        gtk_entry_set_text(entry, filename);
        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}
// Callback function for browsing Spoiler Log (.txt)
static void on_browse_spoiler_clicked(GtkButton *button, gpointer user_data) {
    GtkEntry *entry = GTK_ENTRY(user_data);

    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Select Spoiler Log (.txt)",
        NULL,
        GTK_FILE_CHOOSER_ACTION_SAVE,  // Using SAVE because it’s for output file
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Save", GTK_RESPONSE_ACCEPT,
        NULL
    );

    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, "*.txt");
    gtk_file_filter_set_name(filter, "Text Files");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        gtk_entry_set_text(entry, filename);
        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

int main(int argc, char *argv[]) {  
	gtk_init(&argc, &argv);
	srand(time(NULL));

    // Load CSS style
	GtkCssProvider *css_provider = gtk_css_provider_new();
	GdkDisplay *display = gdk_display_get_default(); // Use the default display
	GdkScreen *screen = gdk_screen_get_default();    // Get the default screen

	gtk_css_provider_load_from_path(css_provider, "00style.css", NULL);
	gtk_style_context_add_provider_for_screen(screen,
		GTK_STYLE_PROVIDER(css_provider),
		GTK_STYLE_PROVIDER_PRIORITY_USER);

    
    // Create the main window  
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);  
    gtk_window_set_title(GTK_WINDOW(window), "Castlevania: Lament of Innocence Randomizer");  
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);  
    gtk_widget_set_size_request(window, 1300, 600);  
    gtk_window_set_resizable(GTK_WINDOW(window), TRUE);  

    // Create a vertical box layout  
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);  
    gtk_container_add(GTK_CONTAINER(window), vbox);  

   // --- Row: ISO File Path ---
	GtkWidget *hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox1, FALSE, FALSE, 0);

	GtkWidget *label1 = gtk_label_new("Filepath to Castlevania: Lament of Innocence NTSC-U ISO");
	gtk_box_pack_start(GTK_BOX(hbox1), label1, FALSE, FALSE, 0);

	GtkWidget *entry1 = gtk_entry_new();
	gtk_widget_set_size_request(entry1, 600, 25);
	gtk_box_pack_start(GTK_BOX(hbox1), entry1, TRUE, TRUE, 0);
	enable_drag_drop(entry1);

	// Create Browse button
	GtkWidget *browse1 = gtk_button_new_with_label("Browse…");
	gtk_box_pack_start(GTK_BOX(hbox1), browse1, FALSE, FALSE, 0);

	// Connect button to handler
	g_signal_connect(browse1, "clicked", G_CALLBACK(on_browse_iso_clicked), entry1);

    GtkWidget *label2 = gtk_label_new("Seed (leave blank for random)");  
    gtk_box_pack_start(GTK_BOX(vbox), label2, FALSE, FALSE, 0);  

    GtkWidget *entry2 = gtk_entry_new();
	gtk_widget_set_size_request(entry2, -1, 25);
	gtk_box_pack_start(GTK_BOX(vbox), entry2, FALSE, FALSE, 0);

    GtkWidget *label3 = gtk_label_new("Filepath for Spoiler Log (.txt document)");  
    gtk_box_pack_start(GTK_BOX(vbox), label3, FALSE, FALSE, 0);  

		// --- Row: Spoiler Log File Path ---
	GtkWidget *hbox3 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox3, FALSE, FALSE, 0);

	GtkWidget *entry3 = gtk_entry_new();
	gtk_widget_set_size_request(entry3, 600, 25);
	gtk_box_pack_start(GTK_BOX(hbox3), entry3, TRUE, TRUE, 0);
	enable_drag_drop(entry3);

	GtkWidget *browse3 = gtk_button_new_with_label("Browse…");
	gtk_box_pack_start(GTK_BOX(hbox3), browse3, FALSE, FALSE, 0);
	g_signal_connect(browse3, "clicked", G_CALLBACK(on_browse_spoiler_clicked), entry3);

   // Create a scrolled window for the checkboxes  
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);  
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);  

    // Create a grid for checkboxes  
    GtkWidget *grid = gtk_grid_new();  
    gtk_container_add(GTK_CONTAINER(scrolled_window), grid);  

    // Initialize CheckBoxData structure  
    CheckBoxData cb_data;  

    const gchar *unique_labels[TOTAL_CHECKBOXES] = {
        "boss loadzones random", "enemy HP random", "enemy tolerance/\nweakness random", "default items shuffle", "relic MP drain\n to be random",
        "key doors potentially not\n require their default key", "doppelganger loadzones random", "sub-weapon\n attacks random", "start with a\n random sub-weapon", "warp room random",
        "Armor DEF random", "random Switch rooms", "random sub-weapon\n heart costs", "decrease starting DEF", "randomize some Important\n models and sprites",
        "hints", "modify the power_ups", "QoL Wolf Foot", "QoL Shop modified", "QoL foods to heal HP, MP,\n and Hearts randomly",
        "QoL start with Gold,\n MP, and Hearts", "QoL make key items\n not be on drops", "QoL item limits", "Start with Skills", "Extension", "Draw Up",
        "Vertical High", "Rising Shot", "Fast Rising", "Spinning Blast", "Energy Blast",
        "Sonic Edge", "A Extension 1", "A Extension 2", "Step Attack", "Falcon Claw",
        "Quick Step", "Quick Step 2", "Perfect Guard", "check_seed", "fake/trap items", "pumpkin subweapons", "randomize \nSave Rooms", "orbs/whips \nanywhere","music rando",
		"Torch-sanity","AreaLocking",
		};
	
	const gchar *checkbox_tooltips[TOTAL_CHECKBOXES] = {
		"Randomize What boss door goes to which boss", "Randomize enemy HP", "Randomize enemy tolerance and weakness", "", "Randomize relic MP drain between the vanilla values",
		"Key doors could require any of the 5 keys (all 5 still used)", "Randomize doppelganger loadzones (2/3 chance of changing)", "Randomize sub-weapon attacks", "Start with a random sub-weapon", "Randomize warp room",
		"Randomize Armor DEF", "Randomize Switch rooms", "Randomize sub-weapon heart costs", "Decrease starting DEF", "Randomize some important models and sprites",
		"Enable hints", "Modify the power-ups", "Put Wolf's Foot in shop for $-1,\nAlso has Wolf's Foot MP Drain VERY slowly", "Modify Shop items", "Make foods randomly heal HP, MP, and Hearts",
		"Start with Gold, MP, and Hearts", "Make key items not be on drops", "Change item limits upto 255","Enable starting skills", "Enable Extension", "Enable Draw Up",
		"Enable Vertical High", "Enable Rising Shot", "Enable Fast Rising", "Enable Spinning Blast", "Enable Energy Blast",
		"Enable Sonic Edge", "Enable A Extension 1", "Enable A Extension 2", "Enable Step Attack", "Enable Falcon Claw",
		"Enable Quick Step", "Enable Quick Step 2", "Enable Perfect Guard", "Check seed for completability",
		"Enable fake/trap items\n Items named 'Sylph's Feather won't do\n what the model would imply", "pumpkin subweapons instead of normal for some", "Randomize Save Rooms", "removes the orb/whip from the boss \nand places them anywhere", "randomizes some 'mostly' boss music",
		"Lets 'most' torches in the game have item drops in them\nTHIS IS STILL IN DEVELOPMENT","Uses the colored keys to lock the areas\nThis likely breaks the standard functionality\nThe keys are set to be in 'random' torches"
	};
	
	// Create checkboxes and apply the 'checkbox-label' class from CSS
	for (int i = 0; i < TOTAL_CHECKBOXES; i++) {  
		cb_data.checkboxes[i] = gtk_check_button_new_with_label(unique_labels[i]);  
		
		if (i >= 24 && i < 40) {
			gtk_widget_hide(cb_data.checkboxes[i]); // Hide options 25 to 40 initially
		}
		
		// Set specified checkboxes to checked
        if (i == 0 || i == 3 || i == 6 || i == 8 || i == 9 || i == 11 || i == 15 || i == 17 || i == 18 ||
            i == 19 || i == 20 || i == 21 || i == 23 || i == 31 || i == 38 || i == 39) {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb_data.checkboxes[i]), TRUE);
        }

        // Make default item shuffle unselectable
        if (i == 3) {
            gtk_widget_set_sensitive(cb_data.checkboxes[i], FALSE); // Disable this checkbox
        }
		
		//gtk_grid_attach(GTK_GRID(grid), cb_data.checkboxes[i], i % COLS, i / COLS, 1, 1); // Attach checkbox to grid
		
		gtk_widget_set_tooltip_text(cb_data.checkboxes[i], checkbox_tooltips[i]);
		
		// Apply the CSS class to the checkbox label
		GtkWidget *checkbox_label = gtk_bin_get_child(GTK_BIN(cb_data.checkboxes[i]));
		gtk_style_context_add_class(gtk_widget_get_style_context(checkbox_label), "checkbox-label");

		gtk_grid_attach(GTK_GRID(grid), cb_data.checkboxes[i], i % COLS, i / COLS, 1, 1);  
	}
	

    // Set the "Show More Options" checkbox at position (4, 0) and set it to show options 25-40
    //gtk_button_set_label(GTK_BUTTON(cb_data.checkboxes[23]), "start with skills");
    g_signal_connect(cb_data.checkboxes[23], "toggled", G_CALLBACK(on_show_more_toggled), &cb_data);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb_data.checkboxes[23]), TRUE); // Start checked


	cb_data.random_seed_button = gtk_button_new_with_label("Random seed");  
    gtk_box_pack_start(GTK_BOX(vbox), cb_data.random_seed_button, FALSE, FALSE, 5);  
	g_signal_connect(cb_data.random_seed_button, "clicked", G_CALLBACK(random_seed), &cb_data);
	
    // Save Preset Button  
	cb_data.save_button = gtk_button_new_with_label("Save Preset");  
	gtk_box_pack_start(GTK_BOX(vbox), cb_data.save_button, FALSE, FALSE, 5);  
	g_signal_connect(cb_data.save_button, "clicked", G_CALLBACK(save_preset), &cb_data);

	// Submit Button  
	cb_data.submit_button = gtk_button_new_with_label("Submit");  
	gtk_box_pack_start(GTK_BOX(vbox), cb_data.submit_button, FALSE, FALSE, 10);  
	//Still not working
	g_signal_connect(cb_data.submit_button, "clicked", G_CALLBACK(on_submit), &cb_data);



	// Scale buttons at startup
	scale_text(cb_data.save_button, 14);
	scale_text(cb_data.submit_button, 14); 

    // Store entry widgets in CheckBoxData  
    cb_data.entries[0] = GTK_ENTRY(entry1);  
    cb_data.entries[1] = GTK_ENTRY(entry2);  
    cb_data.entries[2] = GTK_ENTRY(entry3);  

    // Set initial font size  
    scale_text(label1, 14);  
    scale_text(label2, 14);  
    scale_text(label3, 14);  
    scale_text(cb_data.save_button, 14);  
    scale_text(cb_data.submit_button, 14);  
	scale_text(cb_data.random_seed_button, 14);
	
	give_information_about_everything(window);
	load_preset(&cb_data);
	
    // Connect resize event  
    g_signal_connect(window, "size-allocate", G_CALLBACK(on_window_resize), &cb_data);  

    // Close window event  
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);  

    // Show all widgets  
    gtk_widget_show_all(window);  

    // Run GTK main loop  
    gtk_main();  

    return 0;  
}  
