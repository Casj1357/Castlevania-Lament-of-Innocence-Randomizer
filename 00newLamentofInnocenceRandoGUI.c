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
#include <stdbool.h>
#include <glib.h>

#define ROWS 6
#define COLS 8
#define TOTAL_CHECKBOXES 48
#define TOTAL_USED_CHECKBOXES 48

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
	GtkWidget *golden_knight_entry;
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
	
	// Load Golden Knight chance
	json_value = json_object_object_get(root, "golden_knight_chance");
	if (json_value) {
		gtk_entry_set_text(GTK_ENTRY(cb_data->golden_knight_entry), json_object_get_string(json_value));
	} else {
		gtk_entry_set_text(GTK_ENTRY(cb_data->golden_knight_entry), "85"); // default
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

	// Save Golden Knight chance
	json_object_object_add(
		root,
		"golden_knight_chance",
		json_object_new_string(gtk_entry_get_text(GTK_ENTRY(cb_data->golden_knight_entry)))
	);


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
                                    "you will NOT be able to fight him again.\n"
									"\nTorch and enemy randomization still need tested!\n");
									
	gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

}

static void on_show_more_toggled(GtkToggleButton *toggle_button, gpointer data) {
    CheckBoxData *cb_data = (CheckBoxData *)data;

    // Show or hide checkboxes 25 to 40 based on the toggle state
    gboolean active = gtk_toggle_button_get_active(toggle_button);
    for (int i = 24; i < 39; i++) {
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
	
	enemy_randomization to all enemies available
	
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
	setup_destroy_joachim_mode(fp); //REQUIRE ... might be able to use enemy randomization?
	setup_destroy_pumpkin_mode(fp); //REQUIRE ... should technically work with standard randomization (torches just 'harder')?
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
void randomize_Golden_Knight_to_Boss(FILE* fp,int chance)
{
	//75% chance to happen
	int randVal;
	unsigned char newByte = 0x4F;
	
	randVal = rand() % 100;
	if(randVal <= chance) //choose your own?
	{
		randVal = rand() % 6;
		switch(randVal)
		{
			case 0: 
				newByte = 0x64;
				break;
			case 1:
				newByte = 0x12;
				break;
			case 2:
				newByte = 0x63;
				break;
			case 3:
				newByte = 0x3D;
				break;
			case 4:
				newByte = 0x59;
				break;
			case 5:
				newByte = 0x1C;
				break;
			default:
				break;
		}
		
		fseek(fp,0x209E56B2,SEEK_SET);
		fwrite(&newByte,sizeof(newByte),1,fp);
		fseek(fp,0x209E56BC,SEEK_SET);
		fwrite(&newByte,sizeof(newByte),1,fp);
		fseek(fp,0x209E56F2,SEEK_SET);
		fwrite(&newByte,sizeof(newByte),1,fp);
	}
	//Flame Elemental, Frost Elemental, Thunder Elemental, Doppelganger (red), Doppelganger (blue), Golem
	
}
unsigned char enemy_type(FILE* fp,unsigned char enemy_ID)
{
	unsigned char type = 0x00;
	int randVal;
	switch(enemy_ID)
	{
		case 0x01: case 0x28: //spirit & Rune Spirit
			randVal = rand() % 2;
			if(randVal == 0)
				type = 0x01;
			if(randVal == 1)
				type = 0x28;
			break;
		case 0x04: case 0x35: //Mad Diver & Evil Stabber
			randVal = rand() % 2;
			if(randVal == 0)
				type = 0x04;
			if(randVal == 1)
				type = 0x35;
			break;
		case 0x05: case 0x29: //Fish Man & Merman
			randVal = rand() % 2;
			if(randVal == 0)
				type = 0x05;
			if(randVal == 1)
				type = 0x29;
			break;
		case 0x07: case 0x65: case 0x66: case 0x25: //Zombie & Hanged man & Frost Zombie & Flame Zombie
			randVal = rand() % 4;
			if(randVal == 0)
				type = 0x07;
			if(randVal == 1)
				type = 0x65;
			if(randVal == 2)
				type = 0x66;
			if(randVal == 3)
				type = 0x25;
			break;
		case 0x09: case 0x13: case 0x0E: case 0x39: case 0x0B: //Evil Sword & Chaos Sword & Thunder Sword & Flame Sword & Frost Sword
			randVal = rand() % 5;
			if(randVal == 0)
				type = 0x09;
			if(randVal == 1)
				type = 0x13;
			if(randVal == 2)
				type = 0x0E;
			if(randVal == 3)
				type = 0x39;
			if(randVal == 4)
				type = 0x0B;
			break;
		case 0x0A: case 0x15: case 0x20: case 0x2E: 
			randVal = rand() % 4;
			if(randVal == 0)
				type = 0x0A;
			if(randVal == 1)
				type = 0x15;
			if(randVal == 2)
				type = 0x20;
			if(randVal == 3)
				type = 0x2E;
			break;
		case 0x0D: case 0x0F: case 0x10: case 0x11:
			randVal = rand() % 4;
			if(randVal == 0)
				type = 0x0D;
			if(randVal == 1)
				type = 0x0F;
			if(randVal == 2)
				type = 0x10;
			if(randVal == 3)
				type = 0x11;
			break;
		case 0x17: case 0x30: 
			randVal = rand() % 2;
			if(randVal == 0)
				type = 0x17;
			if(randVal == 1)
				type = 0x30;
			break;
		case 0x1E: case 0x1F: case 0x47: case 0x27:
			randVal = rand() % 4;
			if(randVal == 0)
				type = 0x1E;
			if(randVal == 1)
				type = 0x1F;
			if(randVal == 2)
				type = 0x47;
			if(randVal == 3)
				type = 0x27;
			break;
		case 0x22: case 0x23: case 0x24:
			randVal = rand() % 3;
			if(randVal == 0)
				type = 0x22;
			if(randVal == 1)
				type = 0x23;
			if(randVal == 2)
				type = 0x24;
			break;
		case 0x4C: case 0x4E: case 0x61:
			randVal = rand() % 3;
			if(randVal == 0)
				type = 0x4C;
			if(randVal == 1)
				type = 0x4E;
			if(randVal == 2)
				type = 0x61;
			break;
		case 0x53: case 0x57: case 0x58:
			randVal = rand() % 3;
			if(randVal == 0)
				type = 0x53;
			if(randVal == 1)
				type = 0x57;
			if(randVal == 2)
				type = 0x58;
			break;
		case 0x62: case 0x69:
			randVal = rand() % 2;
			if(randVal == 0)
				type = 0x62;
			if(randVal == 1)
				type = 0x69;
			break;
		case 0x6A: case 0x6B: case 0x6C: case 0x6D:
			randVal = rand() % 4;
			if(randVal == 0)
				type = 0x6A;
			if(randVal == 1)
				type = 0x6B;
			if(randVal == 2)
				type = 0x6C;
			if(randVal == 3)
				type = 0x6D;
			break;
		case 0x6F: case 0x70: case 0x71:
			randVal = rand() % 3;
			if(randVal == 0)
				type = 0x6F;
			if(randVal == 1)
				type = 0x70;
			if(randVal == 2)
				type = 0x71;
			break;
		default:
			type = enemy_ID;
			break;
	}
	return type;
}

void randomize_enemy_spawns(FILE* fp,int chance)
{
	/* This is going to be a massive function...*/
	/* The arrays of tables per each room are going to use several thousand lines... */

//rooms must be excluded: 0x23761BB2, 0x08016CB2 , TODO: there are a few more (but we will just exclude the enemies for now)

int enemy_room_1[] = {
    //num enemy types: 2, num enemies 4,
    //0x0226D884, 0x0226D888,
    0x02,
    //room spawners:,
    0x0226D8B2,
    //values: 01 00 03 00 0F 00  //0x03 - Red Skeleton,
    //0x0226D8BC, //+0x0A address,
    //room spawners:,
    0x0226D8C6,
    //values: 01 00 61 00 0F 00  //0x61 - Wolf Skeleton,
    //0x0226D8D0, //+0x0A address,
    //1: 01 00 03 00 04 00,
    0x0226D8E2,
    //2: 01 00 03 00 04 00,
    0x0226D962,
    //3: 01 00 61 00 04 00,
    0x0226D9E2,
    //4: 01 00 61 00 04 00,
    0x0226DA62
};

int enemy_room_2[] = {
    //num enemy types: 3, num enemies 7,
    //0x02A3C404, 0x02A3C408,
    0x03,
    //room spawners:,
    0x02A3C432,
    //values: 01 00 42 00 0F 00  //0x42 - ???,
    //0x02A3C43C, //+0x0A address,
    //room spawners:,
    0x02A3C446,
    //values: 01 00 67 00 02 00  //0x67 - Spartacus,
    //0x02A3C450, //+0x0A address,
    //room spawners:,
    0x02A3C45A,
    //values: 01 00 0C 00 0F 00  //0x0C - Skeleton,
    //0x02A3C464, //+0x0A address,
    //1: 01 00 42 00 04 00,
    0x02A3C472,
    //2: 01 00 0C 00 04 00,
    0x02A3C4F2,
    //3: 01 00 0C 00 04 00,
    0x02A3C572,
    //4: 01 00 0C 00 04 00,
    0x02A3C5F2,
    //5: 01 00 0C 00 04 00,
    0x02A3C672,
    //6: 01 00 67 00 04 00,
    0x02A3C6F2,
    //7: 01 00 67 00 04 00,
    0x02A3C772
};

int enemy_room_3[] = {
    //num enemy types: 5, num enemies 17,
    //0x02C28F84, 0x02C28F88,
    0x05,
    //room spawners:,
    0x02C28FB2,
    //values: 01 00 2F 00 02 00  //0x2F - Phantom,
    //0x02C28FBC, //+0x0A address,
    //room spawners:,
    0x02C28FC6,
    //values: 01 00 0C 00 0D 00  //0x0C - Skeleton,
    //0x02C28FD0, //+0x0A address,
    //room spawners:,
    0x02C28FDA,
    //values: 01 00 1A 00 0F 00  //0x1A - Heavy Armor,
    //0x02C28FE4, //+0x0A address,
    //room spawners:,
    0x02C28FEE,
    //values: 01 00 4B 00 00 00  //0x4B - ???,
    //0x02C28FF8, //+0x0A address,
    //room spawners:,
    0x02C29002,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x02C2900C, //+0x0A address,
    //1: 01 00 1A 00 07 00,
    0x02C29022,
    //2: 01 00 1A 00 07 00,
    0x02C290A2,
    //3: 01 00 0C 00 07 00,
    0x02C29122,
    //4: 01 00 0C 00 07 00,
    0x02C291A2,
    //5: 01 00 0C 00 07 00,
    0x02C29222,
    //6: 01 00 0C 00 07 00,
    0x02C292A2,
    //7: 01 00 0C 00 07 00,
    0x02C29322,
    //8: 01 00 0C 00 07 00,
    0x02C293A2,
    //9: 01 00 0C 00 07 00,
    0x02C29422,
    //10: 01 00 0C 00 07 00,
    0x02C294A2,
    //11: 01 00 0C 00 07 00,
    0x02C29522,
    //12: 01 00 0C 00 07 00,
    0x02C295A2,
    //13: 01 00 2F 00 14 00,
    0x02C29622,
    //14: 01 00 4B 00 06 00,
    0x02C296A2,
    //15: 01 00 54 00 04 00,
    0x02C29722,
    //16: 01 00 54 00 04 00,
    0x02C297A2,
    //17: 01 00 54 00 04 00,
    0x02C29822
};

int enemy_room_4[] = {
    //num enemy types: 4, num enemies 5,
    //0x02DF8704, 0x02DF8708,
    0x04,
    //room spawners:,
    0x02DF8732,
    //values: 01 00 1E 00 0F 00  //0x1E - Ghost Warrior,
    //0x02DF873C, //+0x0A address,
    //room spawners:,
    0x02DF8746,
    //values: 01 00 2A 00 02 00  //0x2A - Vassago,
    //0x02DF8750, //+0x0A address,
    //room spawners:,
    0x02DF875A,
    //values: 01 00 0C 00 0D 00  //0x0C - Skeleton,
    //0x02DF8764, //+0x0A address,
    //room spawners:,
    0x02DF876E,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x02DF8778, //+0x0A address,
    //1: 01 00 1E 00 14 00,
    0x02DF8782,
    //2: 01 00 0C 00 14 00,
    0x02DF8802,
    //3: 01 00 2A 00 05 00,
    0x02DF8882,
    //4: 01 00 2A 00 05 00,
    0x02DF8902,
    //5: 01 00 54 00 04 00,
    0x02DF8982
};

int enemy_room_5[] = {
    //num enemy types: 4, num enemies 9,
    //0x02FC9204, 0x02FC9208,
    0x04,
    //room spawners:,
    0x02FC9232,
    //values: 01 00 1E 00 0D 00  //0x1E - Ghost Warrior,
    //0x02FC923C, //+0x0A address,
    //room spawners:,
    0x02FC9246,
    //values: 01 00 2A 00 02 00  //0x2A - Vassago,
    //0x02FC9250, //+0x0A address,
    //room spawners:,
    0x02FC925A,
    //values: 01 00 11 00 0F 00  //0x11 - Skeleton Swordsman,
    //0x02FC9264, //+0x0A address,
    //room spawners:,
    0x02FC926E,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x02FC9278, //+0x0A address,
    //1: 01 00 1E 00 04 00,
    0x02FC9282,
    //2: 01 00 11 00 04 00,
    0x02FC9302,
    //3: 01 00 11 00 04 00,
    0x02FC9382,
    //4: 01 00 11 00 04 00,
    0x02FC9402,
    //5: 01 00 1E 00 04 00,
    0x02FC9482,
    //6: 01 00 2A 00 14 00,
    0x02FC9502,
    //7: 01 00 54 00 04 00,
    0x02FC9582,
    //8: 01 00 54 00 04 00,
    0x02FC9602,
    //9: 01 00 54 00 04 00,
    0x02FC9682
};

int enemy_room_6[] = {
    //num enemy types: 3, num enemies 9,
    //0x03199D04, 0x03199D08,
    0x03,
    //room spawners:,
    0x03199D32,
    //values: 01 00 11 00 0F 00  //0x11 - Skeleton Swordsman,
    //0x03199D3C, //+0x0A address,
    //room spawners:,
    0x03199D46,
    //values: 01 00 28 00 0F 00  //0x28 - Rune Spirit,
    //0x03199D50, //+0x0A address,
    //room spawners:,
    0x03199D5A,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x03199D64, //+0x0A address,
    //1: 01 00 28 00 04 00,
    0x03199D72,
    //2: 01 00 11 00 04 00,
    0x03199DF2,
    //3: 01 00 11 00 04 00,
    0x03199E72,
    //4: 01 00 11 00 04 00,
    0x03199EF2,
    //5: 01 00 11 00 04 00,
    0x03199F72,
    //6: 01 00 11 00 04 00,
    0x03199FF2,
    //7: 01 00 54 00 04 00,
    0x0319A072,
    //8: 01 00 54 00 04 00,
    0x0319A0F2,
    //9: 01 00 54 00 04 00,
    0x0319A172
};

int enemy_room_7[] = {
    //num enemy types: 4, num enemies 5,
    //0x0335FD84, 0x0335FD88,
    0x04,
    //room spawners:,
    0x0335FDB2,
    //values: 01 00 2A 00 02 00  //0x2A - Vassago,
    //0x0335FDBC, //+0x0A address,
    //room spawners:,
    0x0335FDC6,
    //values: 01 00 28 00 0F 00  //0x28 - Rune Spirit,
    //0x0335FDD0, //+0x0A address,
    //room spawners:,
    0x0335FDDA,
    //values: 01 00 0C 00 0D 00  //0x0C - Skeleton,
    //0x0335FDE4, //+0x0A address,
    //room spawners:,
    0x0335FDEE,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x0335FDF8, //+0x0A address,
    //1: 01 00 28 00 14 00,
    0x0335FE02,
    //2: 01 00 0C 00 14 00,
    0x0335FE82,
    //3: 01 00 2A 00 05 00,
    0x0335FF02,
    //4: 01 00 2A 00 05 00,
    0x0335FF82,
    //5: 01 00 54 00 04 00,
    0x03360002
};

int enemy_room_8[] = {
    //num enemy types: 4, num enemies 9,
    //0x03534F84, 0x03534F88,
    0x04,
    //room spawners:,
    0x03534FB2,
    //values: 01 00 61 00 0F 00  //0x61 - Wolf Skeleton,
    //0x03534FBC, //+0x0A address,
    //room spawners:,
    0x03534FC6,
    //values: 01 00 07 00 0D 00  //0x07 - Zombie,
    //0x03534FD0, //+0x0A address,
    //room spawners:,
    0x03534FDA,
    //values: 01 00 65 00 02 00  //0x65 - Hanged Man,
    //0x03534FE4, //+0x0A address,
    //room spawners:,
    0x03534FEE,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x03534FF8, //+0x0A address,
    //1: 01 00 61 00 04 00,
    0x03535002,
    //2: 01 00 61 00 04 00,
    0x03535082,
    //3: 01 00 61 00 04 00,
    0x03535102,
    //4: 01 00 07 00 04 00,
    0x03535182,
    //5: 01 00 07 00 04 00,
    0x03535202,
    //6: 01 00 65 00 04 00,
    0x03535282,
    //7: 01 00 65 00 04 00,
    0x03535302,
    //8: 01 00 54 00 04 00,
    0x03535382,
    //9: 01 00 54 00 04 00,
    0x03535402
};

int enemy_room_9[] = {
    //num enemy types: 4, num enemies 11,
    //0x0370D384, 0x0370D388,
    0x04,
    //room spawners:,
    0x0370D3B2,
    //values: 01 00 1A 00 0F 00  //0x1A - Heavy Armor,
    //0x0370D3BC, //+0x0A address,
    //room spawners:,
    0x0370D3C6,
    //values: 01 00 03 00 0D 00  //0x03 - Red Skeleton,
    //0x0370D3D0, //+0x0A address,
    //room spawners:,
    0x0370D3DA,
    //values: 01 00 4F 00 02 00  //0x4F - Golden Knight,
    //0x0370D3E4, //+0x0A address,
    //room spawners:,
    0x0370D3EE,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x0370D3F8, //+0x0A address,
    //1: 01 00 1A 00 07 00,
    0x0370D402,
    //2: 01 00 1A 00 07 00,
    0x0370D482,
    //3: 01 00 03 00 07 00,
    0x0370D502,
    //4: 01 00 03 00 07 00,
    0x0370D582,
    //5: 01 00 03 00 07 00,
    0x0370D602,
    //6: 01 00 03 00 07 00,
    0x0370D682,
    //7: 01 00 03 00 07 00,
    0x0370D702,
    //8: 01 00 03 00 07 00,
    0x0370D782,
    //9: 01 00 4F 00 14 00,
    0x0370D802,
    //10: 01 00 54 00 04 00,
    0x0370D882,
    //11: 01 00 54 00 04 00,
    0x0370D902
};

int enemy_room_10[] = {
    //num enemy types: 3, num enemies 5,
    //0x038DF084, 0x038DF088,
    0x03,
    //room spawners:,
    0x038DF0B2,
    //values: 01 00 0C 00 0F 00  //0x0C - Skeleton,
    //0x038DF0BC, //+0x0A address,
    //room spawners:,
    0x038DF0C6,
    //values: 01 00 65 00 02 00  //0x65 - Hanged Man,
    //0x038DF0D0, //+0x0A address,
    //room spawners:,
    0x038DF0DA,
    //values: 01 00 17 00 0D 00  //0x17 - Skeleton Archer,
    //0x038DF0E4, //+0x0A address,
    //1: 01 00 0C 00 14 00,
    0x038DF0F2,
    //2: 01 00 17 00 14 00,
    0x038DF172,
    //3: 01 00 65 00 04 00,
    0x038DF1F2,
    //4: 01 00 65 00 04 00,
    0x038DF272,
    //5: 01 00 65 00 04 00,
    0x038DF2F2
};

int enemy_room_11[] = {
    //num enemy types: 4, num enemies 6,
    //0x03AD6504, 0x03AD6508,
    0x04,
    //room spawners:,
    0x03AD6532,
    //values: 01 00 65 00 02 00  //0x65 - Hanged Man,
    //0x03AD653C, //+0x0A address,
    //room spawners:,
    0x03AD6546,
    //values: 01 00 17 00 02 00  //0x17 - Skeleton Archer,
    //0x03AD6550, //+0x0A address,
    //room spawners:,
    0x03AD655A,
    //values: 01 00 07 00 0D 00  //0x07 - Zombie,
    //0x03AD6564, //+0x0A address,
    //room spawners:,
    0x03AD656E,
    //values: 01 00 4B 00 00 00  //0x4B - ???,
    //0x03AD6578, //+0x0A address,
    //1: 01 00 17 00 04 00,
    0x03AD6582,
    //2: 01 00 17 00 04 00,
    0x03AD6602,
    //3: 01 00 17 00 04 00,
    0x03AD6682,
    //4: 01 00 07 00 14 00,
    0x03AD6702,
    //5: 01 00 65 00 14 00,
    0x03AD6782,
    //6: 01 00 4B 00 06 00,
    0x03AD6802
};

int enemy_room_12[] = {
    //num enemy types: 3, num enemies 8,
    //0x03C77284, 0x03C77288,
    0x03,
    //room spawners:,
    0x03C772B2,
    //values: 01 00 07 00 0D 00  //0x07 - Zombie,
    //0x03C772BC, //+0x0A address,
    //room spawners:,
    0x03C772C6,
    //values: 01 00 10 00 0F 00  //0x10 - Skeleton Knight,
    //0x03C772D0, //+0x0A address,
    //room spawners:,
    0x03C772DA,
    //values: 01 00 0A 00 02 00  //0x0A - Axe Armor,
    //0x03C772E4, //+0x0A address,
    //1: 01 00 07 00 04 00,
    0x03C772F2,
    //2: 01 00 10 00 04 00,
    0x03C77372,
    //3: 01 00 10 00 04 00,
    0x03C773F2,
    //4: 01 00 07 00 04 00,
    0x03C77472,
    //5: 01 00 10 00 04 00,
    0x03C774F2,
    //6: 01 00 10 00 04 00,
    0x03C77572,
    //7: 01 00 0A 00 04 00,
    0x03C775F2,
    //8: 01 00 0A 00 04 00,
    0x03C77672
};

int enemy_room_13[] = {
    //num enemy types: 2, num enemies 2,
    //0x03E77704, 0x03E77708,
    0x02,
    //room spawners:,
    0x03E77732,
    //values: 01 00 10 00 0F 00  //0x10 - Skeleton Knight,
    //0x03E7773C, //+0x0A address,
    //room spawners:,
    0x03E77746,
    //values: 01 00 07 00 0F 00  //0x07 - Zombie,
    //0x03E77750, //+0x0A address,
    //1: 01 00 10 00 14 00,
    0x03E77762,
    //2: 01 00 07 00 14 00,
    0x03E777E2
};

int enemy_room_14[] = {
    //num enemy types: 3, num enemies 5,
    //0x0401CB84, 0x0401CB88,
    0x03,
    //room spawners:,
    0x0401CBB2,
    //values: 01 00 10 00 0D 00  //0x10 - Skeleton Knight,
    //0x0401CBBC, //+0x0A address,
    //room spawners:,
    0x0401CBC6,
    //values: 01 00 67 00 02 00  //0x67 - Spartacus,
    //0x0401CBD0, //+0x0A address,
    //room spawners:,
    0x0401CBDA,
    //values: 01 00 1E 00 0F 00  //0x1E - Ghost Warrior,
    //0x0401CBE4, //+0x0A address,
    //1: 01 00 10 00 14 00,
    0x0401CBF2,
    //2: 01 00 1E 00 14 00,
    0x0401CC72,
    //3: 01 00 67 00 04 00,
    0x0401CCF2,
    //4: 01 00 67 00 04 00,
    0x0401CD72,
    //5: 01 00 67 00 04 00,
    0x0401CDF2
};

int enemy_room_15[] = {
    //num enemy types: 2, num enemies 6,
    //0x041D4D84, 0x041D4D88,
    0x02,
    //room spawners:,
    0x041D4DB2,
    //values: 01 00 07 00 0F 00  //0x07 - Zombie,
    //0x041D4DBC, //+0x0A address,
    //room spawners:,
    0x041D4DC6,
    //values: 01 00 28 00 0F 00  //0x28 - Rune Spirit,
    //0x041D4DD0, //+0x0A address,
    //1: 01 00 28 00 04 00,
    0x041D4DE2,
    //2: 01 00 28 00 04 00,
    0x041D4E62,
    //3: 01 00 07 00 04 00,
    0x041D4EE2,
    //4: 01 00 07 00 04 00,
    0x041D4F62,
    //5: 01 00 28 00 04 00,
    0x041D4FE2,
    //6: 01 00 28 00 04 00,
    0x041D5062
};

int enemy_room_16[] = {
    //num enemy types: 2, num enemies 6,
    //0x043CE704, 0x043CE708,
    0x02,
    //room spawners:,
    0x043CE732,
    //values: 01 00 28 00 0F 00  //0x28 - Rune Spirit,
    //0x043CE73C, //+0x0A address,
    //room spawners:,
    0x043CE746,
    //values: 01 00 10 00 0F 00  //0x10 - Skeleton Knight,
    //0x043CE750, //+0x0A address,
    //1: 01 00 28 00 04 00,
    0x043CE762,
    //2: 01 00 28 00 04 00,
    0x043CE7E2,
    //3: 01 00 28 00 04 00,
    0x043CE862,
    //4: 01 00 28 00 04 00,
    0x043CE8E2,
    //5: 01 00 10 00 04 00,
    0x043CE962,
    //6: 01 00 10 00 04 00,
    0x043CE9E2
};

int enemy_room_17[] = {
    //num enemy types: 4, num enemies 7,
    //0x0484DC84, 0x0484DC88,
    0x04,
    //room spawners:,
    0x0484DCB2,
    //values: 01 00 67 00 02 00  //0x67 - Spartacus,
    //0x0484DCBC, //+0x0A address,
    //room spawners:,
    0x0484DCC6,
    //values: 01 00 10 00 0F 00  //0x10 - Skeleton Knight,
    //0x0484DCD0, //+0x0A address,
    //room spawners:,
    0x0484DCDA,
    //values: 01 00 17 00 0F 00  //0x17 - Skeleton Archer,
    //0x0484DCE4, //+0x0A address,
    //room spawners:,
    0x0484DCEE,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x0484DCF8, //+0x0A address,
    //1: 01 00 10 00 16 00,
    0x0484DD02,
    //2: 01 00 17 00 14 00,
    0x0484DD82,
    //3: 01 00 67 00 06 00,
    0x0484DE02,
    //4: 01 00 67 00 06 00,
    0x0484DE82,
    //5: 01 00 67 00 06 00,
    0x0484DF02,
    //6: 01 00 67 00 06 00,
    0x0484DF82,
    //7: 01 00 54 00 04 00,
    0x0484E002
};

int enemy_room_18[] = {
    //num enemy types: 3, num enemies 4,
    //0x049E4B84, 0x049E4B88,
    0x03,
    //room spawners:,
    0x049E4BB2,
    //values: 01 00 67 00 0F 00  //0x67 - Spartacus,
    //0x049E4BBC, //+0x0A address,
    //room spawners:,
    0x049E4BC6,
    //values: 01 00 61 00 0F 00  //0x61 - Wolf Skeleton,
    //0x049E4BD0, //+0x0A address,
    //room spawners:,
    0x049E4BDA,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x049E4BE4, //+0x0A address,
    //1: 01 00 67 00 14 00,
    0x049E4BF2,
    //2: 01 00 61 00 14 00,
    0x049E4C72,
    //3: 01 00 67 00 04 00,
    0x049E4CF2,
    //4: 01 00 54 00 04 00,
    0x049E4D72
};

int enemy_room_19[] = {
    //num enemy types: 4, num enemies 5,
    //0x04B91784, 0x04B91788,
    0x04,
    //room spawners:,
    0x04B917B2,
    //values: 01 00 07 00 0F 00  //0x07 - Zombie,
    //0x04B917BC, //+0x0A address,
    //room spawners:,
    0x04B917C6,
    //values: 01 00 22 00 02 00  //0x22 - Astral Fighter,
    //0x04B917D0, //+0x0A address,
    //room spawners:,
    0x04B917DA,
    //values: 01 00 2A 00 0F 00  //0x2A - Vassago,
    //0x04B917E4, //+0x0A address,
    //room spawners:,
    0x04B917EE,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x04B917F8, //+0x0A address,
    //1: 01 00 07 00 16 00,
    0x04B91802,
    //2: 01 00 2A 00 14 00,
    0x04B91882,
    //3: 01 00 22 00 06 00,
    0x04B91902,
    //4: 01 00 22 00 06 00,
    0x04B91982,
    //5: 01 00 54 00 04 00,
    0x04B91A02
};

int enemy_room_20[] = {
    //num enemy types: 2, num enemies 2,
    //0x04D06C84, 0x04D06C88,
    0x02,
    //room spawners:,
    0x04D06CB2,
    //values: 01 00 21 00 0F 00  //0x21 - Poison Zombie,
    //0x04D06CBC, //+0x0A address,
    //room spawners:,
    0x04D06CC6,
    //values: 01 00 22 00 0F 00  //0x22 - Astral Fighter,
    //0x04D06CD0, //+0x0A address,
    //1: 01 00 21 00 14 00,
    0x04D06CE2,
    //2: 01 00 22 00 14 00,
    0x04D06D62
};

int enemy_room_21[] = {
    //num enemy types: 2, num enemies 3,
    //0x04E9DD84, 0x04E9DD88,
    0x02,
    //room spawners:,
    0x04E9DDB2,
    //values: 01 00 58 00 0F 00  //0x58 - Executioner,
    //0x04E9DDBC, //+0x0A address,
    //room spawners:,
    0x04E9DDC6,
    //values: 01 00 2A 00 0F 00  //0x2A - Vassago,
    //0x04E9DDD0, //+0x0A address,
    //1: 01 00 58 00 04 00,
    0x04E9DDE2,
    //2: 01 00 2A 00 14 00,
    0x04E9DE62,
    //3: 01 00 2A 00 14 00,
    0x04E9DEE2
};

int enemy_room_22[] = {
    //num enemy types: 2, num enemies 4,
    //0x0502BC84, 0x0502BC88,
    0x02,
    //room spawners:,
    0x0502BCB2,
    //values: 01 00 22 00 0F 00  //0x22 - Astral Fighter,
    //0x0502BCBC, //+0x0A address,
    //room spawners:,
    0x0502BCC6,
    //values: 01 00 2A 00 0F 00  //0x2A - Vassago,
    //0x0502BCD0, //+0x0A address,
    //1: 01 00 2A 00 14 00,
    0x0502BCE2,
    //2: 01 00 22 00 04 00,
    0x0502BD62,
    //3: 01 00 22 00 04 00,
    0x0502BDE2,
    //4: 01 00 22 00 04 00,
    0x0502BE62
};

int enemy_room_23[] = {
    //num enemy types: 1, num enemies 1,
    //0x05323D84, 0x05323D88,
    0x01,
    //room spawners:,
    0x05323DB2,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x05323DBC, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x05323DD2
};

int enemy_room_24[] = {
    //num enemy types: 3, num enemies 5,
    //0x05D24C04, 0x05D24C08,
    0x03,
    //room spawners:,
    0x05D24C32,
    //values: 01 00 3A 00 0F 00  //0x3A - Buckbaird,
    //0x05D24C3C, //+0x0A address,
    //room spawners:,
    0x05D24C46,
    //values: 01 00 03 00 0F 00  //0x03 - Red Skeleton,
    //0x05D24C50, //+0x0A address,
    //room spawners:,
    0x05D24C5A,
    //values: 01 00 4B 00 00 00  //0x4B - ???,
    //0x05D24C64, //+0x0A address,
    //1: 01 00 3A 00 07 00,
    0x05D24C72,
    //2: 01 00 03 00 05 00,
    0x05D24CF2,
    //3: 01 00 03 00 05 00,
    0x05D24D72,
    //4: 01 00 03 00 05 00,
    0x05D24DF2,
    //5: 01 00 4B 00 06 00,
    0x05D24E72
};

int enemy_room_25[] = {
    //num enemy types: 2, num enemies 6,
    //0x05E7C504, 0x05E7C508,
    0x02,
    //room spawners:,
    0x05E7C532,
    //values: 01 00 08 00 0F 00  //0x08 - Bat,
    //0x05E7C53C, //+0x0A address,
    //room spawners:,
    0x05E7C546,
    //values: 01 00 3A 00 0F 00  //0x3A - Buckbaird,
    //0x05E7C550, //+0x0A address,
    //1: 01 00 08 00 05 00,
    0x05E7C562,
    //2: 01 00 08 00 05 00,
    0x05E7C5E2,
    //3: 01 00 08 00 05 00,
    0x05E7C662,
    //4: 01 00 08 00 05 00,
    0x05E7C6E2,
    //5: 01 00 08 00 05 00,
    0x05E7C762,
    //6: 01 00 3A 00 05 00,
    0x05E7C7E2
};

int enemy_room_26[] = {
    //num enemy types: 2, num enemies 6,
    //0x05FD7104, 0x05FD7108,
    0x02,
    //room spawners:,
    0x05FD7132,
    //values: 01 00 03 00 0F 00  //0x03 - Red Skeleton,
    //0x05FD713C, //+0x0A address,
    //room spawners:,
    0x05FD7146,
    //values: 01 00 08 00 0F 00  //0x08 - Bat,
    //0x05FD7150, //+0x0A address,
    //1: 01 00 03 00 05 00,
    0x05FD7162,
    //2: 01 00 08 00 05 00,
    0x05FD71E2,
    //3: 01 00 08 00 05 00,
    0x05FD7262,
    //4: 01 00 08 00 05 00,
    0x05FD72E2,
    //5: 01 00 08 00 05 00,
    0x05FD7362,
    //6: 01 00 08 00 05 00,
    0x05FD73E2
};

int enemy_room_27[] = {
    //num enemy types: 3, num enemies 6,
    //0x06131404, 0x06131408,
    0x03,
    //room spawners:,
    0x06131432,
    //values: 01 00 30 00 0F 00  //0x30 - Skeleton Hunter,
    //0x0613143C, //+0x0A address,
    //room spawners:,
    0x06131446,
    //values: 01 00 03 00 0F 00  //0x03 - Red Skeleton,
    //0x06131450, //+0x0A address,
    //room spawners:,
    0x0613145A,
    //values: 01 00 4B 00 00 00  //0x4B - ???,
    //0x06131464, //+0x0A address,
    //1: 01 00 30 00 05 00,
    0x06131472,
    //2: 01 00 03 00 05 00,
    0x061314F2,
    //3: 01 00 03 00 05 00,
    0x06131572,
    //4: 01 00 30 00 05 00,
    0x061315F2,
    //5: 01 00 30 00 05 00,
    0x06131672,
    //6: 01 00 4B 00 06 00,
    0x061316F2
};

int enemy_room_28[] = {
    //num enemy types: 1, num enemies 2,
    //0x063C5584, 0x063C5588,
    0x01,
    //room spawners:,
    0x063C55B2,
    //values: 01 00 11 00 0F 00  //0x11 - Skeleton Swordsman,
    //0x063C55BC, //+0x0A address,
    //1: 01 00 11 00 05 00,
    0x063C55D2,
    //2: 01 00 11 00 05 00,
    0x063C5652
};

int enemy_room_29[] = {
    //num enemy types: 1, num enemies 11,
    //0x06565D84, 0x06565D88,
    0x01,
    //room spawners:,
    0x06565DB2,
    //values: 01 00 62 00 0F 00  //0x62 - Flea Man,
    //0x06565DBC, //+0x0A address,
    //1: 01 00 62 00 07 00,
    0x06565DD2,
    //2: 01 00 62 00 07 00,
    0x06565E52,
    //3: 01 00 62 00 07 00,
    0x06565ED2,
    //4: 01 00 62 00 07 00,
    0x06565F52,
    //5: 01 00 62 00 07 00,
    0x06565FD2,
    //6: 01 00 62 00 07 00,
    0x06566052,
    //7: 01 00 62 00 07 00,
    0x065660D2,
    //8: 01 00 62 00 07 00,
    0x06566152,
    //9: 01 00 62 00 07 00,
    0x065661D2,
    //10: 01 00 62 00 07 00,
    0x06566252,
    //11: 01 00 62 00 16 00,
    0x065662D2
};

int enemy_room_30[] = {
    //num enemy types: 1, num enemies 1,
    //0x06704D04, 0x06704D08,
    0x01,
    //room spawners:,
    0x06704D32,
    //values: 01 00 3A 00 02 00  //0x3A - Buckbaird,
    //0x06704D3C, //+0x0A address,
    //1: 01 00 3A 00 14 00,
    0x06704D52
};

int enemy_room_31[] = {
    //num enemy types: 3, num enemies 7,
    //0x068FB884, 0x068FB888,
    0x03,
    //room spawners:,
    0x068FB8B2,
    //values: 01 00 19 00 02 00  //0x19 - Mist,
    //0x068FB8BC, //+0x0A address,
    //room spawners:,
    0x068FB8C6,
    //values: 01 00 17 00 0F 00  //0x17 - Skeleton Archer,
    //0x068FB8D0, //+0x0A address,
    //room spawners:,
    0x068FB8DA,
    //values: 01 00 4B 00 00 00  //0x4B - ???,
    //0x068FB8E4, //+0x0A address,
    //1: 01 00 19 00 05 00,
    0x068FB8F2,
    //2: 01 00 17 00 07 00,
    0x068FB972,
    //3: 01 00 17 00 07 00,
    0x068FB9F2,
    //4: 01 00 17 00 07 00,
    0x068FBA72,
    //5: 01 00 17 00 05 00,
    0x068FBAF2,
    //6: 01 00 17 00 05 00,
    0x068FBB72,
    //7: 01 00 4B 00 06 00,
    0x068FBBF2
};

int enemy_room_32[] = {
    //num enemy types: 2, num enemies 3,
    //0x06A53804, 0x06A53808,
    0x02,
    //room spawners:,
    0x06A53832,
    //values: 01 00 0C 00 0F 00  //0x0C - Skeleton,
    //0x06A5383C, //+0x0A address,
    //room spawners:,
    0x06A53846,
    //values: 01 00 58 00 02 00  //0x58 - Executioner,
    //0x06A53850, //+0x0A address,
    //1: 01 00 0C 00 04 00,
    0x06A53862,
    //2: 01 00 0C 00 04 00,
    0x06A538E2,
    //3: 01 00 58 00 05 00,
    0x06A53962
};

int enemy_room_33[] = {
    //num enemy types: 2, num enemies 5,
    //0x06B95E84, 0x06B95E88,
    0x02,
    //room spawners:,
    0x06B95EB2,
    //values: 01 00 58 00 02 00  //0x58 - Executioner,
    //0x06B95EBC, //+0x0A address,
    //room spawners:,
    0x06B95EC6,
    //values: 01 00 0C 00 0F 00  //0x0C - Skeleton,
    //0x06B95ED0, //+0x0A address,
    //1: 01 00 0C 00 05 00,
    0x06B95EE2,
    //2: 01 00 0C 00 05 00,
    0x06B95F62,
    //3: 01 00 0C 00 05 00,
    0x06B95FE2,
    //4: 01 00 0C 00 05 00,
    0x06B96062,
    //5: 01 00 58 00 04 00,
    0x06B960E2
};

int enemy_room_34[] = {
    //num enemy types: 1, num enemies 6,
    //0x06D08804, 0x06D08808,
    0x01,
    //room spawners:,
    0x06D08832,
    //values: 01 00 30 00 02 00  //0x30 - Skeleton Hunter,
    //0x06D0883C, //+0x0A address,
    //1: 01 00 30 00 04 00,
    0x06D08852,
    //2: 01 00 30 00 04 00,
    0x06D088D2,
    //3: 01 00 30 00 04 00,
    0x06D08952,
    //4: 01 00 30 00 04 00,
    0x06D089D2,
    //5: 01 00 30 00 04 00,
    0x06D08A52,
    //6: 01 00 30 00 04 00,
    0x06D08AD2
};

int enemy_room_35[] = {
    //num enemy types: 1, num enemies 3,
    //0x06ED5A84, 0x06ED5A88,
    0x01,
    //room spawners:,
    0x06ED5AB2,
    //values: 01 00 09 00 0F 00  //0x09 - Evil Sword,
    //0x06ED5ABC, //+0x0A address,
    //1: 01 00 09 00 05 00,
    0x06ED5AD2,
    //2: 01 00 09 00 05 00,
    0x06ED5B52,
    //3: 01 00 09 00 05 00,
    0x06ED5BD2
};

int enemy_room_36[] = {
    //num enemy types: 2, num enemies 6,
    //0x07256A84, 0x07256A88,
    0x02,
    //room spawners:,
    0x07256AB2,
    //values: 01 00 67 00 02 00  //0x67 - Spartacus,
    //0x07256ABC, //+0x0A address,
    //room spawners:,
    0x07256AC6,
    //values: 01 00 30 00 02 00  //0x30 - Skeleton Hunter,
    //0x07256AD0, //+0x0A address,
    //1: 01 00 67 00 04 00,
    0x07256AE2,
    //2: 01 00 67 00 04 00,
    0x07256B62,
    //3: 01 00 30 00 04 00,
    0x07256BE2,
    //4: 01 00 67 00 04 00,
    0x07256C62,
    //5: 01 00 30 00 04 00,
    0x07256CE2,
    //6: 01 00 67 00 04 00,
    0x07256D62
};

int enemy_room_37[] = {
    //num enemy types: 2, num enemies 6,
    //0x07426004, 0x07426008,
    0x02,
    //room spawners:,
    0x07426032,
    //values: 01 00 03 00 02 00  //0x03 - Red Skeleton,
    //0x0742603C, //+0x0A address,
    //room spawners:,
    0x07426046,
    //values: 01 00 22 00 02 00  //0x22 - Astral Fighter,
    //0x07426050, //+0x0A address,
    //1: 01 00 22 00 04 00,
    0x07426062,
    //2: 01 00 22 00 04 00,
    0x074260E2,
    //3: 01 00 22 00 04 00,
    0x07426162,
    //4: 01 00 03 00 04 00,
    0x074261E2,
    //5: 01 00 03 00 04 00,
    0x07426262,
    //6: 01 00 03 00 04 00,
    0x074262E2
};

int enemy_room_38[] = {
    //num enemy types: 3, num enemies 7,
    //0x07564404, 0x07564408,
    0x03,
    //room spawners:,
    0x07564432,
    //values: 01 00 07 00 02 00  //0x07 - Zombie,
    //0x0756443C, //+0x0A address,
    //room spawners:,
    0x07564446,
    //values: 01 00 65 00 02 00  //0x65 - Hanged Man,
    //0x07564450, //+0x0A address,
    //room spawners:,
    0x0756445A,
    //values: 01 00 4B 00 00 00  //0x4B - ???,
    //0x07564464, //+0x0A address,
    //1: 01 00 07 00 05 00,
    0x07564472,
    //2: 01 00 07 00 05 00,
    0x075644F2,
    //3: 01 00 07 00 05 00,
    0x07564572,
    //4: 01 00 65 00 05 00,
    0x075645F2,
    //5: 01 00 65 00 05 00,
    0x07564672,
    //6: 01 00 65 00 05 00,
    0x075646F2,
    //7: 01 00 4B 00 06 00,
    0x07564772
};

int enemy_room_39[] = {
    //num enemy types: 3, num enemies 7,
    //0x076AB284, 0x076AB288,
    0x03,
    //room spawners:,
    0x076AB2B2,
    //values: 01 00 65 00 02 00  //0x65 - Hanged Man,
    //0x076AB2BC, //+0x0A address,
    //room spawners:,
    0x076AB2C6,
    //values: 01 00 07 00 02 00  //0x07 - Zombie,
    //0x076AB2D0, //+0x0A address,
    //room spawners:,
    0x076AB2DA,
    //values: 01 00 4B 00 00 00  //0x4B - ???,
    //0x076AB2E4, //+0x0A address,
    //1: 01 00 07 00 05 00,
    0x076AB2F2,
    //2: 01 00 07 00 07 00,
    0x076AB372,
    //3: 01 00 07 00 05 00,
    0x076AB3F2,
    //4: 01 00 65 00 05 00,
    0x076AB472,
    //5: 01 00 65 00 05 00,
    0x076AB4F2,
    //6: 01 00 65 00 05 00,
    0x076AB572,
    //7: 01 00 4B 00 06 00,
    0x076AB5F2
};

int enemy_room_40[] = {
    //num enemy types: 2, num enemies 6,
    //0x077F3484, 0x077F3488,
    0x02,
    //room spawners:,
    0x077F34B2,
    //values: 01 00 65 00 02 00  //0x65 - Hanged Man,
    //0x077F34BC, //+0x0A address,
    //room spawners:,
    0x077F34C6,
    //values: 01 00 1E 00 02 00  //0x1E - Ghost Warrior,
    //0x077F34D0, //+0x0A address,
    //1: 01 00 1E 00 05 00,
    0x077F34E2,
    //2: 01 00 1E 00 05 00,
    0x077F3562,
    //3: 01 00 1E 00 05 00,
    0x077F35E2,
    //4: 01 00 65 00 05 00,
    0x077F3662,
    //5: 01 00 65 00 05 00,
    0x077F36E2,
    //6: 01 00 65 00 05 00,
    0x077F3762
};

int enemy_room_41[] = {
    //num enemy types: 2, num enemies 6,
    //0x07943104, 0x07943108,
    0x02,
    //room spawners:,
    0x07943132,
    //values: 01 00 28 00 02 00  //0x28 - Rune Spirit,
    //0x0794313C, //+0x0A address,
    //room spawners:,
    0x07943146,
    //values: 01 00 65 00 02 00  //0x65 - Hanged Man,
    //0x07943150, //+0x0A address,
    //1: 01 00 28 00 05 00,
    0x07943162,
    //2: 01 00 28 00 05 00,
    0x079431E2,
    //3: 01 00 28 00 05 00,
    0x07943262,
    //4: 01 00 65 00 05 00,
    0x079432E2,
    //5: 01 00 65 00 05 00,
    0x07943362,
    //6: 01 00 65 00 05 00,
    0x079433E2
};

int enemy_room_42[] = {
    //num enemy types: 2, num enemies 5,
    //0x07A9CC04, 0x07A9CC08,
    0x02,
    //room spawners:,
    0x07A9CC32,
    //values: 01 00 07 00 02 00  //0x07 - Zombie,
    //0x07A9CC3C, //+0x0A address,
    //room spawners:,
    0x07A9CC46,
    //values: 01 00 3A 00 02 00  //0x3A - Buckbaird,
    //0x07A9CC50, //+0x0A address,
    //1: 01 00 07 00 05 00,
    0x07A9CC62,
    //2: 01 00 07 00 05 00,
    0x07A9CCE2,
    //3: 01 00 07 00 05 00,
    0x07A9CD62,
    //4: 01 00 3A 00 05 00,
    0x07A9CDE2,
    //5: 01 00 3A 00 05 00,
    0x07A9CE62
};

int enemy_room_43[] = {
    //num enemy types: 2, num enemies 6,
    //0x07BF3104, 0x07BF3108,
    0x02,
    //room spawners:,
    0x07BF3132,
    //values: 01 00 2A 00 02 00  //0x2A - Vassago,
    //0x07BF313C, //+0x0A address,
    //room spawners:,
    0x07BF3146,
    //values: 01 00 65 00 02 00  //0x65 - Hanged Man,
    //0x07BF3150, //+0x0A address,
    //1: 01 00 2A 00 04 00,
    0x07BF3162,
    //2: 01 00 2A 00 04 00,
    0x07BF31E2,
    //3: 01 00 2A 00 04 00,
    0x07BF3262,
    //4: 01 00 65 00 04 00,
    0x07BF32E2,
    //5: 01 00 65 00 04 00,
    0x07BF3362,
    //6: 01 00 65 00 04 00,
    0x07BF33E2
};

int enemy_room_44[] = {
    //num enemy types: 3, num enemies 3,
    //0x07D49B04, 0x07D49B08,
    0x03,
    //room spawners:,
    0x07D49B32,
    //values: 01 00 30 00 02 00  //0x30 - Skeleton Hunter,
    //0x07D49B3C, //+0x0A address,
    //room spawners:,
    0x07D49B46,
    //values: 01 00 08 00 02 00  //0x08 - Bat,
    //0x07D49B50, //+0x0A address,
    //room spawners:,
    0x07D49B5A,
    //values: 01 00 4B 00 00 00  //0x4B - ???,
    //0x07D49B64, //+0x0A address,
    //1: 01 00 30 00 04 00,
    0x07D49B72,
    //2: 01 00 08 00 14 00,
    0x07D49BF2,
    //3: 01 00 4B 00 06 00,
    0x07D49C72
};

int enemy_room_45[] = {
    //num enemy types: 3, num enemies 4,
    //0x07EB0284, 0x07EB0288,
    0x03,
    //room spawners:,
    0x07EB02B2,
    //values: 01 00 22 00 02 00  //0x22 - Astral Fighter,
    //0x07EB02BC, //+0x0A address,
    //room spawners:,
    0x07EB02C6,
    //values: 01 00 1A 00 0F 00  //0x1A - Heavy Armor,
    //0x07EB02D0, //+0x0A address,
    //room spawners:,
    0x07EB02DA,
    //values: 01 00 4B 00 00 00  //0x4B - ???,
    //0x07EB02E4, //+0x0A address,
    //1: 01 00 22 00 04 00,
    0x07EB02F2,
    //2: 01 00 1A 00 04 00,
    0x07EB0372,
    //3: 01 00 22 00 04 00,
    0x07EB03F2,
    //4: 01 00 4B 00 06 00,
    0x07EB0472
};

/*
int enemy_room_46[] = {
    //num enemy types: 3, num enemies 15,
    //0x08016C84, 0x08016C88,
    0x03,
    //room spawners:,
    0x08016CB2,
    //values: 01 00 38 00 0F 00  //0x38 - Peeping Eye,
    //0x08016CBC, //+0x0A address,
    //room spawners:,
    0x08016CC6,
    //values: 01 00 54 00 0F 00  //0x54 - ???,
    //0x08016CD0, //+0x0A address,
    //room spawners:,
    0x08016CDA,
    //values: 01 00 0C 00 0F 00  //0x0C - Skeleton,
    //0x08016CE4, //+0x0A address,
    //1: 01 00 38 00 05 00,
    0x08016CF2,
    //2: 01 00 0C 00 07 00,
    0x08016D72,
    //3: 01 00 0C 00 07 00,
    0x08016DF2,
    //4: 01 00 0C 00 07 00,
    0x08016E72,
    //5: 01 00 0C 00 07 00,
    0x08016EF2,
    //6: 01 00 0C 00 07 00,
    0x08016F72,
    //7: 01 00 0C 00 07 00,
    0x08016FF2,
    //8: 01 00 0C 00 06 00,
    0x08017072,
    //9: 01 00 0C 00 06 00,
    0x080170F2,
    //10: 01 00 0C 00 06 00,
    0x08017172,
    //11: 01 00 0C 00 06 00,
    0x080171F2,
    //12: 01 00 0C 00 06 00,
    0x08017272,
    //13: 01 00 0C 00 06 00,
    0x080172F2,
    //14: 01 00 0C 00 06 00,
    0x08017372,
    //15: 01 00 54 00 06 00,
    0x080173F2
};
*/

int enemy_room_47[] = {
    //num enemy types: 1, num enemies 3,
    //0x083C3C84, 0x083C3C88,
    0x01,
    //room spawners:,
    0x083C3CB2,
    //values: 01 00 58 00 0F 00  //0x58 - Executioner,
    //0x083C3CBC, //+0x0A address,
    //1: 01 00 58 00 05 00,
    0x083C3CD2,
    //2: 01 00 58 00 05 00,
    0x083C3D52,
    //3: 01 00 58 00 05 00,
    0x083C3DD2
};

int enemy_room_48[] = {
    //num enemy types: 2, num enemies 6,
    //0x0852A484, 0x0852A488,
    0x02,
    //room spawners:,
    0x0852A4B2,
    //values: 01 00 17 00 02 00  //0x17 - Skeleton Archer,
    //0x0852A4BC, //+0x0A address,
    //room spawners:,
    0x0852A4C6,
    //values: 01 00 22 00 02 00  //0x22 - Astral Fighter,
    //0x0852A4D0, //+0x0A address,
    //1: 01 00 22 00 04 00,
    0x0852A4E2,
    //2: 01 00 22 00 04 00,
    0x0852A562,
    //3: 01 00 22 00 04 00,
    0x0852A5E2,
    //4: 01 00 17 00 04 00,
    0x0852A662,
    //5: 01 00 17 00 04 00,
    0x0852A6E2,
    //6: 01 00 17 00 04 00,
    0x0852A762
};

int enemy_room_49[] = {
    //num enemy types: 1, num enemies 1,
    //0x086CF504, 0x086CF508,
    0x01,
    //room spawners:,
    0x086CF532,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x086CF53C, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x086CF552
};

int enemy_room_50[] = {
    //num enemy types: 1, num enemies 1,
    //0x08876984, 0x08876988,
    0x01,
    //room spawners:,
    0x088769B2,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x088769BC, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x088769D2
};

int enemy_room_51[] = {
    //num enemy types: 1, num enemies 2,
    //0x089FB184, 0x089FB188,
    0x01,
    //room spawners:,
    0x089FB1B2,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x089FB1BC, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x089FB1D2,
    //2: 01 00 54 00 04 00,
    0x089FB252
};

int enemy_room_52[] = {
    //num enemy types: 2, num enemies 4,
    //0x08B57084, 0x08B57088,
    0x02,
    //room spawners:,
    0x08B570B2,
    //values: 01 00 2A 00 02 00  //0x2A - Vassago,
    //0x08B570BC, //+0x0A address,
    //room spawners:,
    0x08B570C6,
    //values: 01 00 61 00 0F 00  //0x61 - Wolf Skeleton,
    //0x08B570D0, //+0x0A address,
    //1: 01 00 61 00 04 00,
    0x08B570E2,
    //2: 01 00 61 00 04 00,
    0x08B57162,
    //3: 01 00 2A 00 04 00,
    0x08B571E2,
    //4: 01 00 2A 00 04 00,
    0x08B57262
};

int enemy_room_53[] = {
    //num enemy types: 1, num enemies 2,
    //0x08CFE304, 0x08CFE308,
    0x01,
    //room spawners:,
    0x08CFE332,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x08CFE33C, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x08CFE352,
    //2: 01 00 54 00 04 00,
    0x08CFE3D2
};

int enemy_room_54[] = {
    //num enemy types: 1, num enemies 2,
    //0x08EA3584, 0x08EA3588,
    0x01,
    //room spawners:,
    0x08EA35B2,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x08EA35BC, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x08EA35D2,
    //2: 01 00 54 00 04 00,
    0x08EA3652
};

int enemy_room_55[] = {
    //num enemy types: 1, num enemies 1,
    //0x09085104, 0x09085108,
    0x01,
    //room spawners:,
    0x09085132,
    //values: 01 00 73 00 0F 00  //0x73 - ???,
    //0x0908513C, //+0x0A address,
    //1: 01 00 73 00 04 00,
    0x09085152
};

int enemy_room_56[] = {
    //num enemy types: 3, num enemies 4,
    //0x091E3384, 0x091E3388,
    0x03,
    //room spawners:,
    0x091E33B2,
    //values: 01 00 41 00 0F 00  //0x41 - ???,
    //0x091E33BC, //+0x0A address,
    //room spawners:,
    0x091E33C6,
    //values: 01 00 0A 00 02 00  //0x0A - Axe Armor,
    //0x091E33D0, //+0x0A address,
    //room spawners:,
    0x091E33DA,
    //values: 01 00 0C 00 0F 00  //0x0C - Skeleton,
    //0x091E33E4, //+0x0A address,
    //1: 01 00 41 00 06 00,
    0x091E33F2,
    //2: 01 00 0C 00 04 00,
    0x091E3472,
    //3: 01 00 0C 00 04 00,
    0x091E34F2,
    //4: 01 00 0A 00 04 00,
    0x091E3572
};

int enemy_room_57[] = {
    //num enemy types: 3, num enemies 4,
    //0x09349784, 0x09349788,
    0x03,
    //room spawners:,
    0x093497B2,
    //values: 01 00 1F 00 0F 00  //0x1F - Ghost Knight,
    //0x093497BC, //+0x0A address,
    //room spawners:,
    0x093497C6,
    //values: 01 00 23 00 02 00  //0x23 - Astral Warrior,
    //0x093497D0, //+0x0A address,
    //room spawners:,
    0x093497DA,
    //values: 01 00 11 00 0D 00  //0x11 - Skeleton Swordsman,
    //0x093497E4, //+0x0A address,
    //1: 01 00 1F 00 14 00,
    0x093497F2,
    //2: 01 00 11 00 14 00,
    0x09349872,
    //3: 01 00 23 00 04 00,
    0x093498F2,
    //4: 01 00 23 00 04 00,
    0x09349972
};

int enemy_room_58[] = {
    //num enemy types: 3, num enemies 6,
    //0x0956D804, 0x0956D808,
    0x03,
    //room spawners:,
    0x0956D832,
    //values: 01 00 66 00 0D 00  //0x66 - Frost Zombie,
    //0x0956D83C, //+0x0A address,
    //room spawners:,
    0x0956D846,
    //values: 01 00 67 00 02 00  //0x67 - Spartacus,
    //0x0956D850, //+0x0A address,
    //room spawners:,
    0x0956D85A,
    //values: 01 00 11 00 0F 00  //0x11 - Skeleton Swordsman,
    //0x0956D864, //+0x0A address,
    //1: 01 00 66 00 14 00,
    0x0956D872,
    //2: 01 00 11 00 14 00,
    0x0956D8F2,
    //3: 01 00 67 00 04 00,
    0x0956D972,
    //4: 01 00 67 00 04 00,
    0x0956D9F2,
    //5: 01 00 67 00 04 00,
    0x0956DA72,
    //6: 01 00 67 00 04 00,
    0x0956DAF2
};

int enemy_room_59[] = {
    //num enemy types: 3, num enemies 4,
    //0x0979BD84, 0x0979BD88,
    0x03,
    //room spawners:,
    0x0979BDB2,
    //values: 01 00 0A 00 0F 00  //0x0A - Axe Armor,
    //0x0979BDBC, //+0x0A address,
    //room spawners:,
    0x0979BDC6,
    //values: 01 00 04 00 02 00  //0x04 - Mad Diver,
    //0x0979BDD0, //+0x0A address,
    //room spawners:,
    0x0979BDDA,
    //values: 01 00 11 00 0D 00  //0x11 - Skeleton Swordsman,
    //0x0979BDE4, //+0x0A address,
    //1: 01 00 0A 00 04 00,
    0x0979BDF2,
    //2: 01 00 11 00 14 00,
    0x0979BE72,
    //3: 01 00 0A 00 04 00,
    0x0979BEF2,
    //4: 01 00 04 00 14 00,
    0x0979BF72
};

int enemy_room_60[] = {
    //num enemy types: 3, num enemies 4,
    //0x09908384, 0x09908388,
    0x03,
    //room spawners:,
    0x099083B2,
    //values: 01 00 38 00 0F 00  //0x38 - Peeping Eye,
    //0x099083BC, //+0x0A address,
    //room spawners:,
    0x099083C6,
    //values: 01 00 23 00 0F 00  //0x23 - Astral Warrior,
    //0x099083D0, //+0x0A address,
    //room spawners:,
    0x099083DA,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x099083E4, //+0x0A address,
    //1: 01 00 38 00 04 00,
    0x099083F2,
    //2: 01 00 38 00 04 00,
    0x09908472,
    //3: 01 00 23 00 04 00,
    0x099084F2,
    //4: 01 00 54 00 04 00,
    0x09908572
};

int enemy_room_61[] = {
    //num enemy types: 3, num enemies 4,
    //0x09A6A784, 0x09A6A788,
    0x03,
    //room spawners:,
    0x09A6A7B2,
    //values: 01 00 20 00 0F 00  //0x20 - Axe Knight,
    //0x09A6A7BC, //+0x0A address,
    //room spawners:,
    0x09A6A7C6,
    //values: 01 00 03 00 0F 00  //0x03 - Red Skeleton,
    //0x09A6A7D0, //+0x0A address,
    //room spawners:,
    0x09A6A7DA,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x09A6A7E4, //+0x0A address,
    //1: 01 00 20 00 04 00,
    0x09A6A7F2,
    //2: 01 00 03 00 04 00,
    0x09A6A872,
    //3: 01 00 03 00 04 00,
    0x09A6A8F2,
    //4: 01 00 54 00 04 00,
    0x09A6A972
};

int enemy_room_62[] = {
    //num enemy types: 2, num enemies 3,
    //0x09BD8E84, 0x09BD8E88,
    0x02,
    //room spawners:,
    0x09BD8EB2,
    //values: 01 00 6A 00 02 00  //0x6A - Flame Demon,
    //0x09BD8EBC, //+0x0A address,
    //room spawners:,
    0x09BD8EC6,
    //values: 01 00 1F 00 02 00  //0x1F - Ghost Knight,
    //0x09BD8ED0, //+0x0A address,
    //1: 01 00 6A 00 04 00,
    0x09BD8EE2,
    //2: 01 00 6A 00 04 00,
    0x09BD8F62,
    //3: 01 00 1F 00 14 00,
    0x09BD8FE2
};

int enemy_room_63[] = {
    //num enemy types: 3, num enemies 3,
    //0x09D38D84, 0x09D38D88,
    0x03,
    //room spawners:,
    0x09D38DB2,
    //values: 01 00 04 00 0F 00  //0x04 - Mad Diver,
    //0x09D38DBC, //+0x0A address,
    //room spawners:,
    0x09D38DC6,
    //values: 01 00 25 00 0F 00  //0x25 - Flame Zombie,
    //0x09D38DD0, //+0x0A address,
    //room spawners:,
    0x09D38DDA,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x09D38DE4, //+0x0A address,
    //1: 01 00 04 00 04 00,
    0x09D38DF2,
    //2: 01 00 25 00 14 00,
    0x09D38E72,
    //3: 01 00 54 00 04 00,
    0x09D38EF2
};

int enemy_room_64[] = {
    //num enemy types: 3, num enemies 4,
    //0x09F5E484, 0x09F5E488,
    0x03,
    //room spawners:,
    0x09F5E4B2,
    //values: 01 00 0A 00 0F 00  //0x0A - Axe Armor,
    //0x09F5E4BC, //+0x0A address,
    //room spawners:,
    0x09F5E4C6,
    //values: 01 00 35 00 02 00  //0x35 - Evil Stabber,
    //0x09F5E4D0, //+0x0A address,
    //room spawners:,
    0x09F5E4DA,
    //values: 01 00 1F 00 0D 00  //0x1F - Ghost Knight,
    //0x09F5E4E4, //+0x0A address,
    //1: 01 00 0A 00 04 00,
    0x09F5E4F2,
    //2: 01 00 0A 00 04 00,
    0x09F5E572,
    //3: 01 00 1F 00 14 00,
    0x09F5E5F2,
    //4: 01 00 35 00 14 00,
    0x09F5E672
};

int enemy_room_65[] = {
    //num enemy types: 1, num enemies 2,
    //0x0A0C6B04, 0x0A0C6B08,
    0x01,
    //room spawners:,
    0x0A0C6B32,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x0A0C6B3C, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x0A0C6B52,
    //2: 01 00 54 00 04 00,
    0x0A0C6BD2
};

int enemy_room_66[] = {
    //num enemy types: 2, num enemies 6,
    //0x0A2FCC04, 0x0A2FCC08,
    0x02,
    //room spawners:,
    0x0A2FCC32,
    //values: 01 00 66 00 0F 00  //0x66 - Frost Zombie,
    //0x0A2FCC3C, //+0x0A address,
    //room spawners:,
    0x0A2FCC46,
    //values: 01 00 25 00 0F 00  //0x25 - Flame Zombie,
    //0x0A2FCC50, //+0x0A address,
    //1: 01 00 66 00 14 00,
    0x0A2FCC62,
    //2: 01 00 25 00 14 00,
    0x0A2FCCE2,
    //3: 01 00 66 00 04 00,
    0x0A2FCD62,
    //4: 01 00 66 00 04 00,
    0x0A2FCDE2,
    //5: 01 00 25 00 04 00,
    0x0A2FCE62,
    //6: 01 00 25 00 04 00,
    0x0A2FCEE2
};

int enemy_room_67[] = {
    //num enemy types: 3, num enemies 6,
    //0x0A487604, 0x0A487608,
    0x03,
    //room spawners:,
    0x0A487632,
    //values: 01 00 1F 00 0D 00  //0x1F - Ghost Knight,
    //0x0A48763C, //+0x0A address,
    //room spawners:,
    0x0A487646,
    //values: 01 00 65 00 02 00  //0x65 - Hanged Man,
    //0x0A487650, //+0x0A address,
    //room spawners:,
    0x0A48765A,
    //values: 01 00 25 00 0F 00  //0x25 - Flame Zombie,
    //0x0A487664, //+0x0A address,
    //1: 01 00 1F 00 14 00,
    0x0A487672,
    //2: 01 00 25 00 14 00,
    0x0A4876F2,
    //3: 01 00 65 00 04 00,
    0x0A487772,
    //4: 01 00 65 00 04 00,
    0x0A4877F2,
    //5: 01 00 65 00 04 00,
    0x0A487872,
    //6: 01 00 65 00 04 00,
    0x0A4878F2
};

int enemy_room_68[] = {
    //num enemy types: 2, num enemies 5,
    //0x0A5F5984, 0x0A5F5988,
    0x02,
    //room spawners:,
    0x0A5F59B2,
    //values: 01 00 15 00 0F 00  //0x15 - Armor Knight,
    //0x0A5F59BC, //+0x0A address,
    //room spawners:,
    0x0A5F59C6,
    //values: 01 00 4E 00 0F 00  //0x4E - Hellhound,
    //0x0A5F59D0, //+0x0A address,
    //1: 01 00 15 00 04 00,
    0x0A5F59E2,
    //2: 01 00 4E 00 04 00,
    0x0A5F5A62,
    //3: 01 00 4E 00 04 00,
    0x0A5F5AE2,
    //4: 01 00 15 00 04 00,
    0x0A5F5B62,
    //5: 01 00 15 00 04 00,
    0x0A5F5BE2
};

int enemy_room_69[] = {
    //num enemy types: 1, num enemies 2,
    //0x0A77A384, 0x0A77A388,
    0x01,
    //room spawners:,
    0x0A77A3B2,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x0A77A3BC, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x0A77A3D2,
    //2: 01 00 54 00 04 00,
    0x0A77A452
};

int enemy_room_70[] = {
    //num enemy types: 4, num enemies 12,
    //0x0A97B904, 0x0A97B908,
    0x04,
    //room spawners:,
    0x0A97B932,
    //values: 01 00 1C 00 0F 00  //0x1C - Golem,
    //0x0A97B93C, //+0x0A address,
    //room spawners:,
    0x0A97B946,
    //values: 01 00 72 00 0F 00  //0x72 - ???,
    //0x0A97B950, //+0x0A address,
    //room spawners:,
    0x0A97B95A,
    //values: 01 00 73 00 0F 00  //0x73 - ???,
    //0x0A97B964, //+0x0A address,
    //room spawners:,
    0x0A97B96E,
    //values: 01 00 74 00 0F 00  //0x74 - ???,
    //0x0A97B978, //+0x0A address,
    //1: 01 00 1C 00 06 00,
    0x0A97B982,
    //2: 01 00 74 00 06 00,
    0x0A97BA02,
    //3: 01 00 73 00 06 00,
    0x0A97BA82,
    //4: 01 00 72 00 06 00,
    0x0A97BB02,
    //5: 01 00 73 00 06 00,
    0x0A97BB82,
    //6: 01 00 72 00 06 00,
    0x0A97BC02,
    //7: 01 00 72 00 06 00,
    0x0A97BC82,
    //8: 01 00 72 00 06 00,
    0x0A97BD02,
    //9: 01 00 72 00 06 00,
    0x0A97BD82,
    //10: 01 00 72 00 06 00,
    0x0A97BE02,
    //11: 01 00 72 00 06 00,
    0x0A97BE82,
    //12: 01 00 1C 00 06 00,
    0x0A97BF02
};

int enemy_room_71[] = {
    //num enemy types: 1, num enemies 2,
    //0x0AB0E904, 0x0AB0E908,
    0x01,
    //room spawners:,
    0x0AB0E932,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x0AB0E93C, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x0AB0E952,
    //2: 01 00 54 00 04 00,
    0x0AB0E9D2
};

int enemy_room_72[] = {
    //num enemy types: 1, num enemies 2,
    //0x0AC7A784, 0x0AC7A788,
    0x01,
    //room spawners:,
    0x0AC7A7B2,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x0AC7A7BC, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x0AC7A7D2,
    //2: 01 00 54 00 04 00,
    0x0AC7A852
};

int enemy_room_73[] = {
    //num enemy types: 2, num enemies 5,
    //0x0ADDC484, 0x0ADDC488,
    0x02,
    //room spawners:,
    0x0ADDC4B2,
    //values: 01 00 23 00 0F 00  //0x23 - Astral Warrior,
    //0x0ADDC4BC, //+0x0A address,
    //room spawners:,
    0x0ADDC4C6,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x0ADDC4D0, //+0x0A address,
    //1: 01 00 23 00 05 00,
    0x0ADDC4E2,
    //2: 01 00 23 00 05 00,
    0x0ADDC562,
    //3: 01 00 23 00 05 00,
    0x0ADDC5E2,
    //4: 01 00 23 00 05 00,
    0x0ADDC662,
    //5: 01 00 54 00 04 00,
    0x0ADDC6E2
};

int enemy_room_74[] = {
    //num enemy types: 1, num enemies 1,
    //0x0AF3EC84, 0x0AF3EC88,
    0x01,
    //room spawners:,
    0x0AF3ECB2,
    //values: 01 00 62 00 02 00  //0x62 - Flea Man,
    //0x0AF3ECBC, //+0x0A address,
    //1: 01 00 62 00 14 00,
    0x0AF3ECD2
};

int enemy_room_75[] = {
    //num enemy types: 2, num enemies 2,
    //0x0B09D484, 0x0B09D488,
    0x02,
    //room spawners:,
    0x0B09D4B2,
    //values: 01 00 65 00 0F 00  //0x65 - Hanged Man,
    //0x0B09D4BC, //+0x0A address,
    //room spawners:,
    0x0B09D4C6,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x0B09D4D0, //+0x0A address,
    //1: 01 00 65 00 14 00,
    0x0B09D4E2,
    //2: 01 00 54 00 04 00,
    0x0B09D562
};

int enemy_room_76[] = {
    //num enemy types: 2, num enemies 5,
    //0x0B240204, 0x0B240208,
    0x02,
    //room spawners:,
    0x0B240232,
    //values: 01 00 14 00 0F 00  //0x14 - Skeleton Flower,
    //0x0B24023C, //+0x0A address,
    //room spawners:,
    0x0B240246,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x0B240250, //+0x0A address,
    //1: 01 00 14 00 04 00,
    0x0B240262,
    //2: 01 00 14 00 04 00,
    0x0B2402E2,
    //3: 01 00 54 00 04 00,
    0x0B240362,
    //4: 01 00 54 00 04 00,
    0x0B2403E2,
    //5: 01 00 54 00 04 00,
    0x0B240462
};

int enemy_room_77[] = {
    //num enemy types: 1, num enemies 2,
    //0x0B3E7E84, 0x0B3E7E88,
    0x01,
    //room spawners:,
    0x0B3E7EB2,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x0B3E7EBC, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x0B3E7ED2,
    //2: 01 00 54 00 04 00,
    0x0B3E7F52
};

int enemy_room_78[] = {
    //num enemy types: 4, num enemies 11,
    //0x0B59E884, 0x0B59E888,
    0x04,
    //room spawners:,
    0x0B59E8B2,
    //values: 01 00 64 00 0F 00  //0x64 - Flame Elemental,
    //0x0B59E8BC, //+0x0A address,
    //room spawners:,
    0x0B59E8C6,
    //values: 01 00 73 00 0F 00  //0x73 - ???,
    //0x0B59E8D0, //+0x0A address,
    //room spawners:,
    0x0B59E8DA,
    //values: 01 00 72 00 0F 00  //0x72 - ???,
    //0x0B59E8E4, //+0x0A address,
    //room spawners:,
    0x0B59E8EE,
    //values: 01 00 74 00 0F 00  //0x74 - ???,
    //0x0B59E8F8, //+0x0A address,
    //1: 01 00 64 00 06 00,
    0x0B59E902,
    //2: 01 00 74 00 06 00,
    0x0B59E982,
    //3: 01 00 72 00 06 00,
    0x0B59EA02,
    //4: 01 00 73 00 06 00,
    0x0B59EA82,
    //5: 01 00 72 00 06 00,
    0x0B59EB02,
    //6: 01 00 72 00 06 00,
    0x0B59EB82,
    //7: 01 00 64 00 06 00,
    0x0B59EC02,
    //8: 01 00 72 00 06 00,
    0x0B59EC82,
    //9: 01 00 72 00 06 00,
    0x0B59ED02,
    //10: 01 00 72 00 06 00,
    0x0B59ED82,
    //11: 01 00 72 00 06 00,
    0x0B59EE02
};

int enemy_room_79[] = {
    //num enemy types: 2, num enemies 5,
    //0x0B701904, 0x0B701908,
    0x02,
    //room spawners:,
    0x0B701932,
    //values: 01 00 57 00 0F 00  //0x57 - Red Ogre,
    //0x0B70193C, //+0x0A address,
    //room spawners:,
    0x0B701946,
    //values: 01 00 03 00 0F 00  //0x03 - Red Skeleton,
    //0x0B701950, //+0x0A address,
    //1: 01 00 57 00 04 00,
    0x0B701962,
    //2: 01 00 03 00 04 00,
    0x0B7019E2,
    //3: 01 00 03 00 04 00,
    0x0B701A62,
    //4: 01 00 03 00 04 00,
    0x0B701AE2,
    //5: 01 00 03 00 04 00,
    0x0B701B62
};

int enemy_room_80[] = {
    //num enemy types: 1, num enemies 2,
    //0x0B9E9C04, 0x0B9E9C08,
    0x01,
    //room spawners:,
    0x0B9E9C32,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x0B9E9C3C, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x0B9E9C52,
    //2: 01 00 54 00 04 00,
    0x0B9E9CD2
};

int enemy_room_81[] = {
    //num enemy types: 3, num enemies 5,
    //0x0BBD2E04, 0x0BBD2E08,
    0x03,
    //room spawners:,
    0x0BBD2E32,
    //values: 01 00 30 00 02 00  //0x30 - Skeleton Hunter,
    //0x0BBD2E3C, //+0x0A address,
    //room spawners:,
    0x0BBD2E46,
    //values: 01 00 4B 00 00 00  //0x4B - ???,
    //0x0BBD2E50, //+0x0A address,
    //room spawners:,
    0x0BBD2E5A,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x0BBD2E64, //+0x0A address,
    //1: 01 00 30 00 07 00,
    0x0BBD2E72,
    //2: 01 00 30 00 07 00,
    0x0BBD2EF2,
    //3: 01 00 30 00 07 00,
    0x0BBD2F72,
    //4: 01 00 4B 00 06 00,
    0x0BBD2FF2,
    //5: 01 00 54 00 04 00,
    0x0BBD3072
};

int enemy_room_82[] = {
    //num enemy types: 3, num enemies 5,
    //0x0BD97804, 0x0BD97808,
    0x03,
    //room spawners:,
    0x0BD97832,
    //values: 01 00 6A 00 0F 00  //0x6A - Flame Demon,
    //0x0BD9783C, //+0x0A address,
    //room spawners:,
    0x0BD97846,
    //values: 01 00 03 00 0F 00  //0x03 - Red Skeleton,
    //0x0BD97850, //+0x0A address,
    //room spawners:,
    0x0BD9785A,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x0BD97864, //+0x0A address,
    //1: 01 00 6A 00 04 00,
    0x0BD97872,
    //2: 01 00 03 00 04 00,
    0x0BD978F2,
    //3: 01 00 03 00 04 00,
    0x0BD97972,
    //4: 01 00 03 00 04 00,
    0x0BD979F2,
    //5: 01 00 54 00 04 00,
    0x0BD97A72
};

int enemy_room_83[] = {
    //num enemy types: 2, num enemies 5,
    //0x0BEFB184, 0x0BEFB188,
    0x02,
    //room spawners:,
    0x0BEFB1B2,
    //values: 01 00 4E 00 0F 00  //0x4E - Hellhound,
    //0x0BEFB1BC, //+0x0A address,
    //room spawners:,
    0x0BEFB1C6,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x0BEFB1D0, //+0x0A address,
    //1: 01 00 4E 00 04 00,
    0x0BEFB1E2,
    //2: 01 00 4E 00 04 00,
    0x0BEFB262,
    //3: 01 00 4E 00 04 00,
    0x0BEFB2E2,
    //4: 01 00 4E 00 04 00,
    0x0BEFB362,
    //5: 01 00 54 00 04 00,
    0x0BEFB3E2
};

int enemy_room_84[] = {
    //num enemy types: 4, num enemies 5,
    //0x0C05CF84, 0x0C05CF88,
    0x04,
    //room spawners:,
    0x0C05CFB2,
    //values: 01 00 20 00 0F 00  //0x20 - Axe Knight,
    //0x0C05CFBC, //+0x0A address,
    //room spawners:,
    0x0C05CFC6,
    //values: 01 00 0A 00 0F 00  //0x0A - Axe Armor,
    //0x0C05CFD0, //+0x0A address,
    //room spawners:,
    0x0C05CFDA,
    //values: 01 00 4B 00 00 00  //0x4B - ???,
    //0x0C05CFE4, //+0x0A address,
    //room spawners:,
    0x0C05CFEE,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x0C05CFF8, //+0x0A address,
    //1: 01 00 20 00 05 00,
    0x0C05D002,
    //2: 01 00 0A 00 05 00,
    0x0C05D082,
    //3: 01 00 0A 00 05 00,
    0x0C05D102,
    //4: 01 00 4B 00 06 00,
    0x0C05D182,
    //5: 01 00 54 00 04 00,
    0x0C05D202
};

int enemy_room_85[] = {
    //num enemy types: 2, num enemies 4,
    //0x0C1BC704, 0x0C1BC708,
    0x02,
    //room spawners:,
    0x0C1BC732,
    //values: 01 00 04 00 0F 00  //0x04 - Mad Diver,
    //0x0C1BC73C, //+0x0A address,
    //room spawners:,
    0x0C1BC746,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x0C1BC750, //+0x0A address,
    //1: 01 00 04 00 04 00,
    0x0C1BC762,
    //2: 01 00 04 00 04 00,
    0x0C1BC7E2,
    //3: 01 00 04 00 04 00,
    0x0C1BC862,
    //4: 01 00 54 00 04 00,
    0x0C1BC8E2
};

int enemy_room_86[] = {
    //num enemy types: 1, num enemies 2,
    //0x0C34EA84, 0x0C34EA88,
    0x01,
    //room spawners:,
    0x0C34EAB2,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x0C34EABC, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x0C34EAD2,
    //2: 01 00 54 00 04 00,
    0x0C34EB52
};

int enemy_room_87[] = {
    //num enemy types: 2, num enemies 4,
    //0x0C555E84, 0x0C555E88,
    0x02,
    //room spawners:,
    0x0C555EB2,
    //values: 01 00 65 00 0F 00  //0x65 - Hanged Man,
    //0x0C555EBC, //+0x0A address,
    //room spawners:,
    0x0C555EC6,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x0C555ED0, //+0x0A address,
    //1: 01 00 65 00 14 00,
    0x0C555EE2,
    //2: 01 00 65 00 14 00,
    0x0C555F62,
    //3: 01 00 54 00 04 00,
    0x0C555FE2,
    //4: 01 00 54 00 04 00,
    0x0C556062
};

int enemy_room_88[] = {
    //num enemy types: 1, num enemies 2,
    //0x0C6C0084, 0x0C6C0088,
    0x01,
    //room spawners:,
    0x0C6C00B2,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x0C6C00BC, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x0C6C00D2,
    //2: 01 00 54 00 04 00,
    0x0C6C0152
};

int enemy_room_89[] = {
    //num enemy types: 2, num enemies 6,
    //0x0C8E9684, 0x0C8E9688,
    0x02,
    //room spawners:,
    0x0C8E96B2,
    //values: 01 00 4E 00 0F 00  //0x4E - Hellhound,
    //0x0C8E96BC, //+0x0A address,
    //room spawners:,
    0x0C8E96C6,
    //values: 01 00 6A 00 0F 00  //0x6A - Flame Demon,
    //0x0C8E96D0, //+0x0A address,
    //1: 01 00 6A 00 04 00,
    0x0C8E96E2,
    //2: 01 00 4E 00 04 00,
    0x0C8E9762,
    //3: 01 00 4E 00 04 00,
    0x0C8E97E2,
    //4: 01 00 4E 00 04 00,
    0x0C8E9862,
    //5: 01 00 4E 00 04 00,
    0x0C8E98E2,
    //6: 01 00 6A 00 04 00,
    0x0C8E9962
};

int enemy_room_90[] = {
    //num enemy types: 2, num enemies 5,
    //0x0CA5F284, 0x0CA5F288,
    0x02,
    //room spawners:,
    0x0CA5F2B2,
    //values: 01 00 23 00 0F 00  //0x23 - Astral Warrior,
    //0x0CA5F2BC, //+0x0A address,
    //room spawners:,
    0x0CA5F2C6,
    //values: 01 00 4B 00 00 00  //0x4B - ???,
    //0x0CA5F2D0, //+0x0A address,
    //1: 01 00 23 00 07 00,
    0x0CA5F2E2,
    //2: 01 00 23 00 07 00,
    0x0CA5F362,
    //3: 01 00 23 00 07 00,
    0x0CA5F3E2,
    //4: 01 00 23 00 07 00,
    0x0CA5F462,
    //5: 01 00 4B 00 06 00,
    0x0CA5F4E2
};

int enemy_room_91[] = {
    //num enemy types: 1, num enemies 2,
    //0x0CC00D84, 0x0CC00D88,
    0x01,
    //room spawners:,
    0x0CC00DB2,
    //values: 01 00 57 00 02 00  //0x57 - Red Ogre,
    //0x0CC00DBC, //+0x0A address,
    //1: 01 00 57 00 05 00,
    0x0CC00DD2,
    //2: 01 00 57 00 05 00,
    0x0CC00E52
};

int enemy_room_92[] = {
    //num enemy types: 3, num enemies 10,
    //0x0CD90184, 0x0CD90188,
    0x03,
    //room spawners:,
    0x0CD901B2,
    //values: 01 00 25 00 0F 00  //0x25 - Flame Zombie,
    //0x0CD901BC, //+0x0A address,
    //room spawners:,
    0x0CD901C6,
    //values: 01 00 2E 00 02 00  //0x2E - Dullahan,
    //0x0CD901D0, //+0x0A address,
    //room spawners:,
    0x0CD901DA,
    //values: 01 00 4B 00 00 00  //0x4B - ???,
    //0x0CD901E4, //+0x0A address,
    //1: 01 00 25 00 05 00,
    0x0CD901F2,
    //2: 01 00 25 00 05 00,
    0x0CD90272,
    //3: 01 00 25 00 05 00,
    0x0CD902F2,
    //4: 01 00 2E 00 05 00,
    0x0CD90372,
    //5: 01 00 2E 00 05 00,
    0x0CD903F2,
    //6: 01 00 2E 00 05 00,
    0x0CD90472,
    //7: 01 00 2E 00 05 00,
    0x0CD904F2,
    //8: 01 00 2E 00 05 00,
    0x0CD90572,
    //9: 01 00 2E 00 05 00,
    0x0CD905F2,
    //10: 01 00 4B 00 06 00,
    0x0CD90672
};

int enemy_room_93[] = {
    //num enemy types: 1, num enemies 6,
    //0x0CED6A84, 0x0CED6A88,
    0x01,
    //room spawners:,
    0x0CED6AB2,
    //values: 01 00 39 00 0F 00  //0x39 - Flame Sword,
    //0x0CED6ABC, //+0x0A address,
    //1: 01 00 39 00 05 00,
    0x0CED6AD2,
    //2: 01 00 39 00 05 00,
    0x0CED6B52,
    //3: 01 00 39 00 05 00,
    0x0CED6BD2,
    //4: 01 00 39 00 05 00,
    0x0CED6C52,
    //5: 01 00 39 00 05 00,
    0x0CED6CD2,
    //6: 01 00 39 00 05 00,
    0x0CED6D52
};

int enemy_room_94[] = {
    //num enemy types: 2, num enemies 7,
    //0x0D05C404, 0x0D05C408,
    0x02,
    //room spawners:,
    0x0D05C432,
    //values: 01 00 4E 00 02 00  //0x4E - Hellhound,
    //0x0D05C43C, //+0x0A address,
    //room spawners:,
    0x0D05C446,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x0D05C450, //+0x0A address,
    //1: 01 00 4E 00 04 00,
    0x0D05C462,
    //2: 01 00 4E 00 04 00,
    0x0D05C4E2,
    //3: 01 00 4E 00 04 00,
    0x0D05C562,
    //4: 01 00 4E 00 04 00,
    0x0D05C5E2,
    //5: 01 00 4E 00 04 00,
    0x0D05C662,
    //6: 01 00 4E 00 04 00,
    0x0D05C6E2,
    //7: 01 00 54 00 04 00,
    0x0D05C762
};

int enemy_room_95[] = {
    //num enemy types: 1, num enemies 6,
    //0x0D1A3984, 0x0D1A3988,
    0x01,
    //room spawners:,
    0x0D1A39B2,
    //values: 01 00 25 00 0F 00  //0x25 - Flame Zombie,
    //0x0D1A39BC, //+0x0A address,
    //1: 01 00 25 00 05 00,
    0x0D1A39D2,
    //2: 01 00 25 00 05 00,
    0x0D1A3A52,
    //3: 01 00 25 00 05 00,
    0x0D1A3AD2,
    //4: 01 00 25 00 05 00,
    0x0D1A3B52,
    //5: 01 00 25 00 05 00,
    0x0D1A3BD2,
    //6: 01 00 25 00 05 00,
    0x0D1A3C52
};

int enemy_room_96[] = {
    //num enemy types: 3, num enemies 13,
    //0x0D32B804, 0x0D32B808,
    0x03,
    //room spawners:,
    0x0D32B832,
    //values: 01 00 2E 00 02 00  //0x2E - Dullahan,
    //0x0D32B83C, //+0x0A address,
    //room spawners:,
    0x0D32B846,
    //values: 01 00 25 00 0F 00  //0x25 - Flame Zombie,
    //0x0D32B850, //+0x0A address,
    //room spawners:,
    0x0D32B85A,
    //values: 01 00 4B 00 00 00  //0x4B - ???,
    //0x0D32B864, //+0x0A address,
    //1: 01 00 25 00 05 00,
    0x0D32B872,
    //2: 01 00 25 00 05 00,
    0x0D32B8F2,
    //3: 01 00 25 00 05 00,
    0x0D32B972,
    //4: 01 00 25 00 05 00,
    0x0D32B9F2,
    //5: 01 00 25 00 05 00,
    0x0D32BA72,
    //6: 01 00 25 00 05 00,
    0x0D32BAF2,
    //7: 01 00 2E 00 05 00,
    0x0D32BB72,
    //8: 01 00 2E 00 05 00,
    0x0D32BBF2,
    //9: 01 00 2E 00 05 00,
    0x0D32BC72,
    //10: 01 00 2E 00 05 00,
    0x0D32BCF2,
    //11: 01 00 2E 00 05 00,
    0x0D32BD72,
    //12: 01 00 2E 00 05 00,
    0x0D32BDF2,
    //13: 01 00 4B 00 06 00,
    0x0D32BE72
};

int enemy_room_97[] = {
    //num enemy types: 1, num enemies 2,
    //0x0D4CC284, 0x0D4CC288,
    0x01,
    //room spawners:,
    0x0D4CC2B2,
    //values: 01 00 57 00 02 00  //0x57 - Red Ogre,
    //0x0D4CC2BC, //+0x0A address,
    //1: 01 00 57 00 05 00,
    0x0D4CC2D2,
    //2: 01 00 57 00 05 00,
    0x0D4CC352
};

int enemy_room_98[] = {
    //num enemy types: 2, num enemies 6,
    //0x0D62B484, 0x0D62B488,
    0x02,
    //room spawners:,
    0x0D62B4B2,
    //values: 01 00 2E 00 02 00  //0x2E - Dullahan,
    //0x0D62B4BC, //+0x0A address,
    //room spawners:,
    0x0D62B4C6,
    //values: 01 00 6A 00 02 00  //0x6A - Flame Demon,
    //0x0D62B4D0, //+0x0A address,
    //1: 01 00 6A 00 04 00,
    0x0D62B4E2,
    //2: 01 00 2E 00 04 00,
    0x0D62B562,
    //3: 01 00 2E 00 04 00,
    0x0D62B5E2,
    //4: 01 00 6A 00 04 00,
    0x0D62B662,
    //5: 01 00 2E 00 04 00,
    0x0D62B6E2,
    //6: 01 00 2E 00 04 00,
    0x0D62B762
};

int enemy_room_99[] = {
    //num enemy types: 2, num enemies 6,
    //0x0D783284, 0x0D783288,
    0x02,
    //room spawners:,
    0x0D7832B2,
    //values: 01 00 38 00 0F 00  //0x38 - Peeping Eye,
    //0x0D7832BC, //+0x0A address,
    //room spawners:,
    0x0D7832C6,
    //values: 01 00 62 00 02 00  //0x62 - Flea Man,
    //0x0D7832D0, //+0x0A address,
    //1: 01 00 38 00 05 00,
    0x0D7832E2,
    //2: 01 00 38 00 04 00,
    0x0D783362,
    //3: 01 00 38 00 04 00,
    0x0D7833E2,
    //4: 01 00 62 00 04 00,
    0x0D783462,
    //5: 01 00 62 00 04 00,
    0x0D7834E2,
    //6: 01 00 62 00 04 00,
    0x0D783562
};

int enemy_room_100[] = {
    //num enemy types: 2, num enemies 3,
    //0x0D8DDB04, 0x0D8DDB08,
    0x02,
    //room spawners:,
    0x0D8DDB32,
    //values: 01 00 20 00 0F 00  //0x20 - Axe Knight,
    //0x0D8DDB3C, //+0x0A address,
    //room spawners:,
    0x0D8DDB46,
    //values: 01 00 4E 00 0F 00  //0x4E - Hellhound,
    //0x0D8DDB50, //+0x0A address,
    //1: 01 00 20 00 05 00,
    0x0D8DDB62,
    //2: 01 00 4E 00 05 00,
    0x0D8DDBE2,
    //3: 01 00 4E 00 05 00,
    0x0D8DDC62
};

int enemy_room_101[] = {
    //num enemy types: 2, num enemies 6,
    //0x0DA62184, 0x0DA62188,
    0x02,
    //room spawners:,
    0x0DA621B2,
    //values: 01 00 04 00 02 00  //0x04 - Mad Diver,
    //0x0DA621BC, //+0x0A address,
    //room spawners:,
    0x0DA621C6,
    //values: 01 00 65 00 02 00  //0x65 - Hanged Man,
    //0x0DA621D0, //+0x0A address,
    //1: 01 00 65 00 04 00,
    0x0DA621E2,
    //2: 01 00 65 00 04 00,
    0x0DA62262,
    //3: 01 00 65 00 04 00,
    0x0DA622E2,
    //4: 01 00 65 00 04 00,
    0x0DA62362,
    //5: 01 00 04 00 04 00,
    0x0DA623E2,
    //6: 01 00 04 00 04 00,
    0x0DA62462
};

int enemy_room_102[] = {
    //num enemy types: 2, num enemies 6,
    //0x0DBAF004, 0x0DBAF008,
    0x02,
    //room spawners:,
    0x0DBAF032,
    //values: 01 00 38 00 0F 00  //0x38 - Peeping Eye,
    //0x0DBAF03C, //+0x0A address,
    //room spawners:,
    0x0DBAF046,
    //values: 01 00 30 00 02 00  //0x30 - Skeleton Hunter,
    //0x0DBAF050, //+0x0A address,
    //1: 01 00 38 00 05 00,
    0x0DBAF062,
    //2: 01 00 38 00 04 00,
    0x0DBAF0E2,
    //3: 01 00 38 00 04 00,
    0x0DBAF162,
    //4: 01 00 30 00 04 00,
    0x0DBAF1E2,
    //5: 01 00 30 00 04 00,
    0x0DBAF262,
    //6: 01 00 30 00 04 00,
    0x0DBAF2E2
};

int enemy_room_103[] = {
    //num enemy types: 1, num enemies 2,
    //0x0DD0C384, 0x0DD0C388,
    0x01,
    //room spawners:,
    0x0DD0C3B2,
    //values: 01 00 6A 00 02 00  //0x6A - Flame Demon,
    //0x0DD0C3BC, //+0x0A address,
    //1: 01 00 6A 00 04 00,
    0x0DD0C3D2,
    //2: 01 00 6A 00 04 00,
    0x0DD0C452
};

int enemy_room_104[] = {
    //num enemy types: 1, num enemies 1,
    //0x0DE97D84, 0x0DE97D88,
    0x01,
    //room spawners:,
    0x0DE97DB2,
    //values: 01 00 23 00 02 00  //0x23 - Astral Warrior,
    //0x0DE97DBC, //+0x0A address,
    //1: 01 00 23 00 14 00,
    0x0DE97DD2
};

int enemy_room_105[] = {
    //num enemy types: 2, num enemies 4,
    //0x0DFF6A04, 0x0DFF6A08,
    0x02,
    //room spawners:,
    0x0DFF6A32,
    //values: 01 00 25 00 00 00  //0x25 - Flame Zombie,
    //0x0DFF6A3C, //+0x0A address,
    //room spawners:,
    0x0DFF6A46,
    //values: 01 00 39 00 00 00  //0x39 - Flame Sword,
    //0x0DFF6A50, //+0x0A address,
    //1: 01 00 25 00 04 00,
    0x0DFF6A62,
    //2: 01 00 25 00 04 00,
    0x0DFF6AE2,
    //3: 01 00 39 00 04 00,
    0x0DFF6B62,
    //4: 01 00 39 00 04 00,
    0x0DFF6BE2
};

int enemy_room_106[] = {
    //num enemy types: 3, num enemies 7,
    //0x0E38A104, 0x0E38A108,
    0x03,
    //room spawners:,
    0x0E38A132,
    //values: 01 00 40 00 0F 00  //0x40 - ???,
    //0x0E38A13C, //+0x0A address,
    //room spawners:,
    0x0E38A146,
    //values: 01 00 71 00 02 00  //0x71 - Lizard Knight,
    //0x0E38A150, //+0x0A address,
    //room spawners:,
    0x0E38A15A,
    //values: 01 00 0C 00 0F 00  //0x0C - Skeleton,
    //0x0E38A164, //+0x0A address,
    //1: 01 00 40 00 04 00,
    0x0E38A172,
    //2: 01 00 71 00 06 00,
    0x0E38A1F2,
    //3: 01 00 71 00 06 00,
    0x0E38A272,
    //4: 01 00 0C 00 06 00,
    0x0E38A2F2,
    //5: 01 00 0C 00 06 00,
    0x0E38A372,
    //6: 01 00 0C 00 06 00,
    0x0E38A3F2,
    //7: 01 00 0C 00 06 00,
    0x0E38A472
};

int enemy_room_107[] = {
    //num enemy types: 3, num enemies 7,
    //0x0E51A804, 0x0E51A808,
    0x03,
    //room spawners:,
    0x0E51A832,
    //values: 01 00 11 00 0F 00  //0x11 - Skeleton Swordsman,
    //0x0E51A83C, //+0x0A address,
    //room spawners:,
    0x0E51A846,
    //values: 01 00 62 00 02 00  //0x62 - Flea Man,
    //0x0E51A850, //+0x0A address,
    //room spawners:,
    0x0E51A85A,
    //values: 01 00 05 00 0F 00  //0x05 - Fish Man,
    //0x0E51A864, //+0x0A address,
    //1: 01 00 11 00 16 00,
    0x0E51A872,
    //2: 01 00 05 00 16 00,
    0x0E51A8F2,
    //3: 01 00 62 00 06 00,
    0x0E51A972,
    //4: 01 00 62 00 06 00,
    0x0E51A9F2,
    //5: 01 00 62 00 06 00,
    0x0E51AA72,
    //6: 01 00 62 00 06 00,
    0x0E51AAF2,
    //7: 01 00 62 00 06 00,
    0x0E51AB72
};

int enemy_room_108[] = {
    //num enemy types: 2, num enemies 5,
    //0x0E6ADE04, 0x0E6ADE08,
    0x02,
    //room spawners:,
    0x0E6ADE32,
    //values: 01 00 6B 00 0F 00  //0x6B - Frost Demon,
    //0x0E6ADE3C, //+0x0A address,
    //room spawners:,
    0x0E6ADE46,
    //values: 01 00 0B 00 0F 00  //0x0B - Frost Sword,
    //0x0E6ADE50, //+0x0A address,
    //1: 01 00 6B 00 04 00,
    0x0E6ADE62,
    //2: 01 00 6B 00 04 00,
    0x0E6ADEE2,
    //3: 01 00 0B 00 04 00,
    0x0E6ADF62,
    //4: 01 00 0B 00 04 00,
    0x0E6ADFE2,
    //5: 01 00 0B 00 04 00,
    0x0E6AE062
};

int enemy_room_109[] = {
    //num enemy types: 2, num enemies 6,
    //0x0E83B484, 0x0E83B488,
    0x02,
    //room spawners:,
    0x0E83B4B2,
    //values: 01 00 29 00 0F 00  //0x29 - Merman,
    //0x0E83B4BC, //+0x0A address,
    //room spawners:,
    0x0E83B4C6,
    //values: 01 00 05 00 0F 00  //0x05 - Fish Man,
    //0x0E83B4D0, //+0x0A address,
    //1: 01 00 29 00 06 00,
    0x0E83B4E2,
    //2: 01 00 05 00 06 00,
    0x0E83B562,
    //3: 01 00 05 00 06 00,
    0x0E83B5E2,
    //4: 01 00 05 00 06 00,
    0x0E83B662,
    //5: 01 00 05 00 06 00,
    0x0E83B6E2,
    //6: 01 00 29 00 16 00,
    0x0E83B762
};

int enemy_room_110[] = {
    //num enemy types: 2, num enemies 7,
    //0x0E9D4204, 0x0E9D4208,
    0x02,
    //room spawners:,
    0x0E9D4232,
    //values: 01 00 0B 00 0F 00  //0x0B - Frost Sword,
    //0x0E9D423C, //+0x0A address,
    //room spawners:,
    0x0E9D4246,
    //values: 01 00 05 00 0F 00  //0x05 - Fish Man,
    //0x0E9D4250, //+0x0A address,
    //1: 01 00 0B 00 04 00,
    0x0E9D4262,
    //2: 01 00 05 00 04 00,
    0x0E9D42E2,
    //3: 01 00 05 00 04 00,
    0x0E9D4362,
    //4: 01 00 05 00 04 00,
    0x0E9D43E2,
    //5: 01 00 05 00 04 00,
    0x0E9D4462,
    //6: 01 00 0B 00 04 00,
    0x0E9D44E2,
    //7: 01 00 0B 00 04 00,
    0x0E9D4562
};

int enemy_room_111[] = {
    //num enemy types: 2, num enemies 5,
    //0x0EB64904, 0x0EB64908,
    0x02,
    //room spawners:,
    0x0EB64932,
    //values: 01 00 62 00 0F 00  //0x62 - Flea Man,
    //0x0EB6493C, //+0x0A address,
    //room spawners:,
    0x0EB64946,
    //values: 01 00 2F 00 02 00  //0x2F - Phantom,
    //0x0EB64950, //+0x0A address,
    //1: 01 00 62 00 06 00,
    0x0EB64962,
    //2: 01 00 62 00 16 00,
    0x0EB649E2,
    //3: 01 00 62 00 16 00,
    0x0EB64A62,
    //4: 01 00 2F 00 06 00,
    0x0EB64AE2,
    //5: 01 00 2F 00 06 00,
    0x0EB64B62
};

int enemy_room_112[] = {
    //num enemy types: 4, num enemies 9,
    //0x0ECFB084, 0x0ECFB088,
    0x04,
    //room spawners:,
    0x0ECFB0B2,
    //values: 01 00 1A 00 0F 00  //0x1A - Heavy Armor,
    //0x0ECFB0BC, //+0x0A address,
    //room spawners:,
    0x0ECFB0C6,
    //values: 01 00 2A 00 0F 00  //0x2A - Vassago,
    //0x0ECFB0D0, //+0x0A address,
    //room spawners:,
    0x0ECFB0DA,
    //values: 01 00 0D 00 0F 00  //0x0D - Skeleton Soldier,
    //0x0ECFB0E4, //+0x0A address,
    //room spawners:,
    0x0ECFB0EE,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x0ECFB0F8, //+0x0A address,
    //1: 01 00 1A 00 06 00,
    0x0ECFB102,
    //2: 01 00 0D 00 06 00,
    0x0ECFB182,
    //3: 01 00 0D 00 06 00,
    0x0ECFB202,
    //4: 01 00 0D 00 06 00,
    0x0ECFB282,
    //5: 01 00 0D 00 06 00,
    0x0ECFB302,
    //6: 01 00 0D 00 06 00,
    0x0ECFB382,
    //7: 01 00 0D 00 06 00,
    0x0ECFB402,
    //8: 01 00 2A 00 16 00,
    0x0ECFB482,
    //9: 01 00 54 00 04 00,
    0x0ECFB502
};

int enemy_room_113[] = {
    //num enemy types: 3, num enemies 7,
    //0x0EEA1E04, 0x0EEA1E08,
    0x03,
    //room spawners:,
    0x0EEA1E32,
    //values: 01 00 0A 00 0F 00  //0x0A - Axe Armor,
    //0x0EEA1E3C, //+0x0A address,
    //room spawners:,
    0x0EEA1E46,
    //values: 01 00 03 00 0F 00  //0x03 - Red Skeleton,
    //0x0EEA1E50, //+0x0A address,
    //room spawners:,
    0x0EEA1E5A,
    //values: 01 00 4B 00 00 00  //0x4B - ???,
    //0x0EEA1E64, //+0x0A address,
    //1: 01 00 0A 00 05 00,
    0x0EEA1E72,
    //2: 01 00 0A 00 05 00,
    0x0EEA1EF2,
    //3: 01 00 03 00 04 00,
    0x0EEA1F72,
    //4: 01 00 03 00 04 00,
    0x0EEA1FF2,
    //5: 01 00 03 00 04 00,
    0x0EEA2072,
    //6: 01 00 03 00 04 00,
    0x0EEA20F2,
    //7: 01 00 4B 00 06 00,
    0x0EEA2172
};

int enemy_room_114[] = {
    //num enemy types: 2, num enemies 5,
    //0x0F032A84, 0x0F032A88,
    0x02,
    //room spawners:,
    0x0F032AB2,
    //values: 01 00 29 00 0F 00  //0x29 - Merman,
    //0x0F032ABC, //+0x0A address,
    //room spawners:,
    0x0F032AC6,
    //values: 01 00 05 00 0F 00  //0x05 - Fish Man,
    //0x0F032AD0, //+0x0A address,
    //1: 01 00 29 00 04 00,
    0x0F032AE2,
    //2: 01 00 05 00 05 00,
    0x0F032B62,
    //3: 01 00 05 00 05 00,
    0x0F032BE2,
    //4: 01 00 05 00 04 00,
    0x0F032C62,
    //5: 01 00 05 00 04 00,
    0x0F032CE2
};

int enemy_room_115[] = {
    //num enemy types: 3, num enemies 7,
    //0x0F1C5C84, 0x0F1C5C88,
    0x03,
    //room spawners:,
    0x0F1C5CB2,
    //values: 01 00 29 00 0F 00  //0x29 - Merman,
    //0x0F1C5CBC, //+0x0A address,
    //room spawners:,
    0x0F1C5CC6,
    //values: 01 00 1A 00 02 00  //0x1A - Heavy Armor,
    //0x0F1C5CD0, //+0x0A address,
    //room spawners:,
    0x0F1C5CDA,
    //values: 01 00 62 00 0F 00  //0x62 - Flea Man,
    //0x0F1C5CE4, //+0x0A address,
    //1: 01 00 29 00 06 00,
    0x0F1C5CF2,
    //2: 01 00 29 00 06 00,
    0x0F1C5D72,
    //3: 01 00 62 00 16 00,
    0x0F1C5DF2,
    //4: 01 00 1A 00 06 00,
    0x0F1C5E72,
    //5: 01 00 1A 00 06 00,
    0x0F1C5EF2,
    //6: 01 00 1A 00 06 00,
    0x0F1C5F72,
    //7: 01 00 1A 00 06 00,
    0x0F1C5FF2
};

int enemy_room_116[] = {
    //num enemy types: 3, num enemies 7,
    //0x0F354F84, 0x0F354F88,
    0x03,
    //room spawners:,
    0x0F354FB2,
    //values: 01 00 6B 00 0F 00  //0x6B - Frost Demon,
    //0x0F354FBC, //+0x0A address,
    //room spawners:,
    0x0F354FC6,
    //values: 01 00 69 00 02 00  //0x69 - Death Ripper,
    //0x0F354FD0, //+0x0A address,
    //room spawners:,
    0x0F354FDA,
    //values: 01 00 1A 00 0F 00  //0x1A - Heavy Armor,
    //0x0F354FE4, //+0x0A address,
    //1: 01 00 6B 00 06 00,
    0x0F354FF2,
    //2: 01 00 6B 00 06 00,
    0x0F355072,
    //3: 01 00 1A 00 06 00,
    0x0F3550F2,
    //4: 01 00 69 00 16 00,
    0x0F355172,
    //5: 01 00 1A 00 06 00,
    0x0F3551F2,
    //6: 01 00 1A 00 06 00,
    0x0F355272,
    //7: 01 00 1A 00 06 00,
    0x0F3552F2
};

int enemy_room_117[] = {
    //num enemy types: 2, num enemies 6,
    //0x0F4E6904, 0x0F4E6908,
    0x02,
    //room spawners:,
    0x0F4E6932,
    //values: 01 00 29 00 0F 00  //0x29 - Merman,
    //0x0F4E693C, //+0x0A address,
    //room spawners:,
    0x0F4E6946,
    //values: 01 00 15 00 0F 00  //0x15 - Armor Knight,
    //0x0F4E6950, //+0x0A address,
    //1: 01 00 29 00 04 00,
    0x0F4E6962,
    //2: 01 00 29 00 04 00,
    0x0F4E69E2,
    //3: 01 00 29 00 04 00,
    0x0F4E6A62,
    //4: 01 00 29 00 04 00,
    0x0F4E6AE2,
    //5: 01 00 15 00 05 00,
    0x0F4E6B62,
    //6: 01 00 15 00 05 00,
    0x0F4E6BE2
};

int enemy_room_118[] = {
    //num enemy types: 2, num enemies 6,
    //0x0F678F84, 0x0F678F88,
    0x02,
    //room spawners:,
    0x0F678FB2,
    //values: 01 00 53 00 0F 00  //0x53 - Cyclops,
    //0x0F678FBC, //+0x0A address,
    //room spawners:,
    0x0F678FC6,
    //values: 01 00 29 00 0F 00  //0x29 - Merman,
    //0x0F678FD0, //+0x0A address,
    //1: 01 00 53 00 05 00,
    0x0F678FE2,
    //2: 01 00 53 00 04 00,
    0x0F679062,
    //3: 01 00 29 00 04 00,
    0x0F6790E2,
    //4: 01 00 29 00 04 00,
    0x0F679162,
    //5: 01 00 29 00 04 00,
    0x0F6791E2,
    //6: 01 00 29 00 04 00,
    0x0F679262
};

int enemy_room_119[] = {
    //num enemy types: 2, num enemies 5,
    //0x0F80CB04, 0x0F80CB08,
    0x02,
    //room spawners:,
    0x0F80CB32,
    //values: 01 00 6F 00 0F 00  //0x6F - Lizard Man,
    //0x0F80CB3C, //+0x0A address,
    //room spawners:,
    0x0F80CB46,
    //values: 01 00 53 00 0F 00  //0x53 - Cyclops,
    //0x0F80CB50, //+0x0A address,
    //1: 01 00 6F 00 04 00,
    0x0F80CB62,
    //2: 01 00 6F 00 04 00,
    0x0F80CBE2,
    //3: 01 00 53 00 04 00,
    0x0F80CC62,
    //4: 01 00 6F 00 04 00,
    0x0F80CCE2,
    //5: 01 00 6F 00 04 00,
    0x0F80CD62
};

int enemy_room_120[] = {
    //num enemy types: 3, num enemies 6,
    //0x0F9B0604, 0x0F9B0608,
    0x03,
    //room spawners:,
    0x0F9B0632,
    //values: 01 00 6F 00 0F 00  //0x6F - Lizard Man,
    //0x0F9B063C, //+0x0A address,
    //room spawners:,
    0x0F9B0646,
    //values: 01 00 71 00 02 00  //0x71 - Lizard Knight,
    //0x0F9B0650, //+0x0A address,
    //room spawners:,
    0x0F9B065A,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x0F9B0664, //+0x0A address,
    //1: 01 00 6F 00 14 00,
    0x0F9B0672,
    //2: 01 00 6F 00 14 00,
    0x0F9B06F2,
    //3: 01 00 71 00 04 00,
    0x0F9B0772,
    //4: 01 00 71 00 04 00,
    0x0F9B07F2,
    //5: 01 00 54 00 04 00,
    0x0F9B0872,
    //6: 01 00 54 00 04 00,
    0x0F9B08F2
};

int enemy_room_121[] = {
    //num enemy types: 3, num enemies 14,
    //0x0FB66984, 0x0FB66988,
    0x03,
    //room spawners:,
    0x0FB669B2,
    //values: 01 00 3D 00 0F 00  //0x3D - Doppleganger,
    //0x0FB669BC, //+0x0A address,
    //room spawners:,
    0x0FB669C6,
    //values: 01 00 73 00 0F 00  //0x73 - ???,
    //0x0FB669D0, //+0x0A address,
    //room spawners:,
    0x0FB669DA,
    //values: 01 00 72 00 0F 00  //0x72 - ???,
    //0x0FB669E4, //+0x0A address,
    //1: 01 00 3D 00 07 00,
    0x0FB669F2,
    //2: 01 00 3D 00 07 00,
    0x0FB66A72,
    //3: 01 00 72 00 06 00,
    0x0FB66AF2,
    //4: 01 00 73 00 06 00,
    0x0FB66B72,
    //5: 01 00 72 00 06 00,
    0x0FB66BF2,
    //6: 01 00 72 00 06 00,
    0x0FB66C72,
    //7: 01 00 3D 00 07 00,
    0x0FB66CF2,
    //8: 01 00 3D 00 07 00,
    0x0FB66D72,
    //9: 01 00 72 00 06 00,
    0x0FB66DF2,
    //10: 01 00 72 00 06 00,
    0x0FB66E72,
    //11: 01 00 72 00 06 00,
    0x0FB66EF2,
    //12: 01 00 72 00 06 00,
    0x0FB66F72,
    //13: 01 00 3D 00 06 00,
    0x0FB66FF2,
    //14: 01 00 3D 00 06 00,
    0x0FB67072
};

int enemy_room_122[] = {
    //num enemy types: 1, num enemies 4,
    //0x0FD6C584, 0x0FD6C588,
    0x01,
    //room spawners:,
    0x0FD6C5B2,
    //values: 01 00 05 00 0F 00  //0x05 - Fish Man,
    //0x0FD6C5BC, //+0x0A address,
    //1: 01 00 05 00 04 00,
    0x0FD6C5D2,
    //2: 01 00 05 00 04 00,
    0x0FD6C652,
    //3: 01 00 05 00 05 00,
    0x0FD6C6D2,
    //4: 01 00 05 00 05 00,
    0x0FD6C752
};

int enemy_room_123[] = {
    //num enemy types: 2, num enemies 6,
    //0x0FF71084, 0x0FF71088,
    0x02,
    //room spawners:,
    0x0FF710B2,
    //values: 01 00 05 00 0F 00  //0x05 - Fish Man,
    //0x0FF710BC, //+0x0A address,
    //room spawners:,
    0x0FF710C6,
    //values: 01 00 30 00 02 00  //0x30 - Skeleton Hunter,
    //0x0FF710D0, //+0x0A address,
    //1: 01 00 05 00 04 00,
    0x0FF710E2,
    //2: 01 00 05 00 04 00,
    0x0FF71162,
    //3: 01 00 05 00 04 00,
    0x0FF711E2,
    //4: 01 00 05 00 05 00,
    0x0FF71262,
    //5: 01 00 30 00 04 00,
    0x0FF712E2,
    //6: 01 00 30 00 04 00,
    0x0FF71362
};

int enemy_room_124[] = {
    //num enemy types: 2, num enemies 4,
    //0x10172704, 0x10172708,
    0x02,
    //room spawners:,
    0x10172732,
    //values: 01 00 1A 00 0F 00  //0x1A - Heavy Armor,
    //0x1017273C, //+0x0A address,
    //room spawners:,
    0x10172746,
    //values: 01 00 05 00 0F 00  //0x05 - Fish Man,
    //0x10172750, //+0x0A address,
    //1: 01 00 1A 00 05 00,
    0x10172762,
    //2: 01 00 05 00 04 00,
    0x101727E2,
    //3: 01 00 05 00 04 00,
    0x10172862,
    //4: 01 00 05 00 04 00,
    0x101728E2
};

int enemy_room_125[] = {
    //num enemy types: 2, num enemies 5,
    //0x1036A284, 0x1036A288,
    0x02,
    //room spawners:,
    0x1036A2B2,
    //values: 01 00 29 00 0F 00  //0x29 - Merman,
    //0x1036A2BC, //+0x0A address,
    //room spawners:,
    0x1036A2C6,
    //values: 01 00 6F 00 0F 00  //0x6F - Lizard Man,
    //0x1036A2D0, //+0x0A address,
    //1: 01 00 29 00 05 00,
    0x1036A2E2,
    //2: 01 00 29 00 05 00,
    0x1036A362,
    //3: 01 00 29 00 04 00,
    0x1036A3E2,
    //4: 01 00 6F 00 04 00,
    0x1036A462,
    //5: 01 00 6F 00 04 00,
    0x1036A4E2
};

int enemy_room_126[] = {
    //num enemy types: 2, num enemies 6,
    //0x10580F84, 0x10580F88,
    0x02,
    //room spawners:,
    0x10580FB2,
    //values: 01 00 1A 00 0F 00  //0x1A - Heavy Armor,
    //0x10580FBC, //+0x0A address,
    //room spawners:,
    0x10580FC6,
    //values: 01 00 29 00 0F 00  //0x29 - Merman,
    //0x10580FD0, //+0x0A address,
    //1: 01 00 1A 00 04 00,
    0x10580FE2,
    //2: 01 00 29 00 05 00,
    0x10581062,
    //3: 01 00 29 00 05 00,
    0x105810E2,
    //4: 01 00 29 00 05 00,
    0x10581162,
    //5: 01 00 29 00 05 00,
    0x105811E2,
    //6: 01 00 1A 00 04 00,
    0x10581262
};

int enemy_room_127[] = {
    //num enemy types: 4, num enemies 10,
    //0x10786884, 0x10786888,
    0x04,
    //room spawners:,
    0x107868B2,
    //values: 01 00 17 00 0F 00  //0x17 - Skeleton Archer,
    //0x107868BC, //+0x0A address,
    //room spawners:,
    0x107868C6,
    //values: 01 00 71 00 02 00  //0x71 - Lizard Knight,
    //0x107868D0, //+0x0A address,
    //room spawners:,
    0x107868DA,
    //values: 01 00 6F 00 0D 00  //0x6F - Lizard Man,
    //0x107868E4, //+0x0A address,
    //room spawners:,
    0x107868EE,
    //values: 01 00 4B 00 00 00  //0x4B - ???,
    //0x107868F8, //+0x0A address,
    //1: 01 00 17 00 04 00,
    0x10786902,
    //2: 01 00 17 00 04 00,
    0x10786982,
    //3: 01 00 17 00 04 00,
    0x10786A02,
    //4: 01 00 6F 00 04 00,
    0x10786A82,
    //5: 01 00 6F 00 04 00,
    0x10786B02,
    //6: 01 00 6F 00 04 00,
    0x10786B82,
    //7: 01 00 71 00 04 00,
    0x10786C02,
    //8: 01 00 71 00 04 00,
    0x10786C82,
    //9: 01 00 71 00 04 00,
    0x10786D02,
    //10: 01 00 4B 00 06 00,
    0x10786D82
};

int enemy_room_128[] = {
    //num enemy types: 3, num enemies 7,
    //0x109BE184, 0x109BE188,
    0x03,
    //room spawners:,
    0x109BE1B2,
    //values: 01 00 53 00 0F 00  //0x53 - Cyclops,
    //0x109BE1BC, //+0x0A address,
    //room spawners:,
    0x109BE1C6,
    //values: 01 00 62 00 0D 00  //0x62 - Flea Man,
    //0x109BE1D0, //+0x0A address,
    //room spawners:,
    0x109BE1DA,
    //values: 01 00 69 00 02 00  //0x69 - Death Ripper,
    //0x109BE1E4, //+0x0A address,
    //1: 01 00 53 00 04 00,
    0x109BE1F2,
    //2: 01 00 62 00 04 00,
    0x109BE272,
    //3: 01 00 62 00 04 00,
    0x109BE2F2,
    //4: 01 00 62 00 04 00,
    0x109BE372,
    //5: 01 00 69 00 04 00,
    0x109BE3F2,
    //6: 01 00 69 00 04 00,
    0x109BE472,
    //7: 01 00 69 00 04 00,
    0x109BE4F2
};

int enemy_room_129[] = {
    //num enemy types: 3, num enemies 6,
    //0x10BD2004, 0x10BD2008,
    0x03,
    //room spawners:,
    0x10BD2032,
    //values: 01 00 6F 00 0F 00  //0x6F - Lizard Man,
    //0x10BD203C, //+0x0A address,
    //room spawners:,
    0x10BD2046,
    //values: 01 00 53 00 02 00  //0x53 - Cyclops,
    //0x10BD2050, //+0x0A address,
    //room spawners:,
    0x10BD205A,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x10BD2064, //+0x0A address,
    //1: 01 00 6F 00 04 00,
    0x10BD2072,
    //2: 01 00 6F 00 04 00,
    0x10BD20F2,
    //3: 01 00 6F 00 04 00,
    0x10BD2172,
    //4: 01 00 6F 00 04 00,
    0x10BD21F2,
    //5: 01 00 53 00 04 00,
    0x10BD2272,
    //6: 01 00 54 00 04 00,
    0x10BD22F2
};

int enemy_room_130[] = {
    //num enemy types: 2, num enemies 9,
    //0x10D4B004, 0x10D4B008,
    0x02,
    //room spawners:,
    0x10D4B032,
    //values: 01 00 29 00 0F 00  //0x29 - Merman,
    //0x10D4B03C, //+0x0A address,
    //room spawners:,
    0x10D4B046,
    //values: 01 00 17 00 0D 00  //0x17 - Skeleton Archer,
    //0x10D4B050, //+0x0A address,
    //1: 01 00 29 00 04 00,
    0x10D4B062,
    //2: 01 00 29 00 04 00,
    0x10D4B0E2,
    //3: 01 00 17 00 04 00,
    0x10D4B162,
    //4: 01 00 17 00 04 00,
    0x10D4B1E2,
    //5: 01 00 29 00 04 00,
    0x10D4B262,
    //6: 01 00 29 00 04 00,
    0x10D4B2E2,
    //7: 01 00 29 00 04 00,
    0x10D4B362,
    //8: 01 00 29 00 04 00,
    0x10D4B3E2,
    //9: 01 00 29 00 04 00,
    0x10D4B462
};

int enemy_room_131[] = {
    //num enemy types: 1, num enemies 3,
    //0x10EC3C04, 0x10EC3C08,
    0x01,
    //room spawners:,
    0x10EC3C32,
    //values: 01 00 6F 00 02 00  //0x6F - Lizard Man,
    //0x10EC3C3C, //+0x0A address,
    //1: 01 00 6F 00 04 00,
    0x10EC3C52,
    //2: 01 00 6F 00 04 00,
    0x10EC3CD2,
    //3: 01 00 6F 00 04 00,
    0x10EC3D52
};

int enemy_room_132[] = {
    //num enemy types: 2, num enemies 3,
    //0x111ABE04, 0x111ABE08,
    0x02,
    //room spawners:,
    0x111ABE32,
    //values: 01 00 29 00 0F 00  //0x29 - Merman,
    //0x111ABE3C, //+0x0A address,
    //room spawners:,
    0x111ABE46,
    //values: 01 00 53 00 02 00  //0x53 - Cyclops,
    //0x111ABE50, //+0x0A address,
    //1: 01 00 29 00 04 00,
    0x111ABE62,
    //2: 01 00 29 00 04 00,
    0x111ABEE2,
    //3: 01 00 53 00 04 00,
    0x111ABF62
};

int enemy_room_133[] = {
    //num enemy types: 2, num enemies 6,
    //0x11351F84, 0x11351F88,
    0x02,
    //room spawners:,
    0x11351FB2,
    //values: 01 00 62 00 0F 00  //0x62 - Flea Man,
    //0x11351FBC, //+0x0A address,
    //room spawners:,
    0x11351FC6,
    //values: 01 00 6F 00 0F 00  //0x6F - Lizard Man,
    //0x11351FD0, //+0x0A address,
    //1: 01 00 62 00 04 00,
    0x11351FE2,
    //2: 01 00 62 00 04 00,
    0x11352062,
    //3: 01 00 6F 00 04 00,
    0x113520E2,
    //4: 01 00 6F 00 04 00,
    0x11352162,
    //5: 01 00 6F 00 04 00,
    0x113521E2,
    //6: 01 00 6F 00 04 00,
    0x11352262
};

int enemy_room_134[] = {
    //num enemy types: 1, num enemies 1,
    //0x117ECB04, 0x117ECB08,
    0x01,
    //room spawners:,
    0x117ECB32,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x117ECB3C, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x117ECB52
};

int enemy_room_135[] = {
    //num enemy types: 1, num enemies 2,
    //0x11C53284, 0x11C53288,
    0x01,
    //room spawners:,
    0x11C532B2,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x11C532BC, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x11C532D2,
    //2: 01 00 54 00 04 00,
    0x11C53352
};

int enemy_room_136[] = {
    //num enemy types: 1, num enemies 1,
    //0x12004404, 0x12004408,
    0x01,
    //room spawners:,
    0x12004432,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x1200443C, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x12004452
};

int enemy_room_137[] = {
    //num enemy types: 1, num enemies 2,
    //0x12197C84, 0x12197C88,
    0x01,
    //room spawners:,
    0x12197CB2,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x12197CBC, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x12197CD2,
    //2: 01 00 54 00 04 00,
    0x12197D52
};

int enemy_room_138[] = {
    //num enemy types: 1, num enemies 2,
    //0x1233E084, 0x1233E088,
    0x01,
    //room spawners:,
    0x1233E0B2,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x1233E0BC, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x1233E0D2,
    //2: 01 00 54 00 04 00,
    0x1233E152
};

int enemy_room_139[] = {
    //num enemy types: 1, num enemies 2,
    //0x124C2004, 0x124C2008,
    0x01,
    //room spawners:,
    0x124C2032,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x124C203C, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x124C2052,
    //2: 01 00 54 00 04 00,
    0x124C20D2
};

int enemy_room_140[] = {
    //num enemy types: 4, num enemies 11,
    //0x129E6984, 0x129E6988,
    0x04,
    //room spawners:,
    0x129E69B2,
    //values: 01 00 12 00 00 00  //0x12 - Frost Elemental,
    //0x129E69BC, //+0x0A address,
    //room spawners:,
    0x129E69C6,
    //values: 01 00 73 00 00 00  //0x73 - ???,
    //0x129E69D0, //+0x0A address,
    //room spawners:,
    0x129E69DA,
    //values: 01 00 72 00 00 00  //0x72 - ???,
    //0x129E69E4, //+0x0A address,
    //room spawners:,
    0x129E69EE,
    //values: 01 00 74 00 00 00  //0x74 - ???,
    //0x129E69F8, //+0x0A address,
    //1: 01 00 12 00 06 00,
    0x129E6A02,
    //2: 01 00 74 00 06 00,
    0x129E6A82,
    //3: 01 00 72 00 06 00,
    0x129E6B02,
    //4: 01 00 73 00 06 00,
    0x129E6B82,
    //5: 01 00 72 00 06 00,
    0x129E6C02,
    //6: 01 00 72 00 06 00,
    0x129E6C82,
    //7: 01 00 72 00 06 00,
    0x129E6D02,
    //8: 01 00 72 00 06 00,
    0x129E6D82,
    //9: 01 00 72 00 06 00,
    0x129E6E02,
    //10: 01 00 72 00 06 00,
    0x129E6E82,
    //11: 01 00 12 00 06 00,
    0x129E6F02
};

int enemy_room_141[] = {
    //num enemy types: 4, num enemies 11,
    //0x12B56404, 0x12B56408,
    0x04,
    //room spawners:,
    0x12B56432,
    //values: 01 00 2A 00 00 00  //0x2A - Vassago,
    //0x12B5643C, //+0x0A address,
    //room spawners:,
    0x12B56446,
    //values: 01 00 28 00 00 00  //0x28 - Rune Spirit,
    //0x12B56450, //+0x0A address,
    //room spawners:,
    0x12B5645A,
    //values: 01 00 4B 00 00 00  //0x4B - ???,
    //0x12B56464, //+0x0A address,
    //room spawners:,
    0x12B5646E,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x12B56478, //+0x0A address,
    //1: 01 00 28 00 14 00,
    0x12B56482,
    //2: 01 00 2A 00 06 00,
    0x12B56502,
    //3: 01 00 2A 00 06 00,
    0x12B56582,
    //4: 01 00 2A 00 14 00,
    0x12B56602,
    //5: 01 00 2A 00 06 00,
    0x12B56682,
    //6: 01 00 2A 00 06 00,
    0x12B56702,
    //7: 01 00 4B 00 06 00,
    0x12B56782,
    //8: 01 00 4B 00 06 00,
    0x12B56802,
    //9: 01 00 54 00 04 00,
    0x12B56882,
    //10: 01 00 54 00 04 00,
    0x12B56902,
    //11: 01 00 54 00 04 00,
    0x12B56982
};

int enemy_room_142[] = {
    //num enemy types: 1, num enemies 2,
    //0x134CE504, 0x134CE508,
    0x01,
    //room spawners:,
    0x134CE532,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x134CE53C, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x134CE552,
    //2: 01 00 54 00 04 00,
    0x134CE5D2
};

int enemy_room_143[] = {
    //num enemy types: 7, num enemies 16,
    //0x13751B84, 0x13751B88,
    0x07,
    //room spawners:,
    0x13751BB2,
    //values: 01 00 4A 00 00 00  //0x4A - Joachim,
    //0x13751BBC, //+0x0A address,
    //room spawners:,
    0x13751BC6,
    //values: 01 00 72 00 00 00  //0x72 - ???,
    //0x13751BD0, //+0x0A address,
    //room spawners:,
    0x13751BDA,
    //values: 01 00 73 00 00 00  //0x73 - ???,
    //0x13751BE4, //+0x0A address,
    //room spawners:,
    0x13751BEE,
    //values: 01 00 74 00 00 00  //0x74 - ???,
    //0x13751BF8, //+0x0A address,
    //room spawners:,
    0x13751C02,
    //values: 01 00 68 00 00 00  //0x68 - ???,
    //0x13751C0C, //+0x0A address,
    //room spawners:,
    0x13751C16,
    //values: 01 00 48 00 00 00  //0x48 - ???,
    //0x13751C20, //+0x0A address,
    //room spawners:,
    0x13751C2A,
    //values: 01 00 5F 00 00 00  //0x5F - ???,
    //0x13751C34, //+0x0A address,
    //1: 01 00 4A 00 06 00,
    0x13751C42,
    //2: 01 00 5F 00 06 00,
    0x13751CC2,
    //3: 01 00 48 00 06 00,
    0x13751D42,
    //4: 01 00 68 00 06 00,
    0x13751DC2,
    //5: 01 00 68 00 06 00,
    0x13751E42,
    //6: 01 00 74 00 06 00,
    0x13751EC2,
    //7: 01 00 73 00 06 00,
    0x13751F42,
    //8: 01 00 68 00 07 00,
    0x13751FC2,
    //9: 01 00 72 00 06 00,
    0x13752042,
    //10: 01 00 73 00 06 00,
    0x137520C2,
    //11: 01 00 72 00 06 00,
    0x13752142,
    //12: 01 00 72 00 06 00,
    0x137521C2,
    //13: 01 00 72 00 06 00,
    0x13752242,
    //14: 01 00 72 00 06 00,
    0x137522C2,
    //15: 01 00 72 00 06 00,
    0x13752342,
    //16: 01 00 72 00 06 00,
    0x137523C2
};

int enemy_room_144[] = {
    //num enemy types: 1, num enemies 2,
    //0x13A63884, 0x13A63888,
    0x01,
    //room spawners:,
    0x13A638B2,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x13A638BC, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x13A638D2,
    //2: 01 00 54 00 04 00,
    0x13A63952
};

int enemy_room_145[] = {
    //num enemy types: 2, num enemies 5,
    //0x13D91C04, 0x13D91C08,
    0x02,
    //room spawners:,
    0x13D91C32,
    //values: 01 00 16 00 0F 00  //0x16 - Gargoyle,
    //0x13D91C3C, //+0x0A address,
    //room spawners:,
    0x13D91C46,
    //values: 01 00 70 00 02 00  //0x70 - Poison Lizard,
    //0x13D91C50, //+0x0A address,
    //1: 01 00 16 00 06 00,
    0x13D91C62,
    //2: 01 00 16 00 06 00,
    0x13D91CE2,
    //3: 01 00 16 00 06 00,
    0x13D91D62,
    //4: 01 00 16 00 06 00,
    0x13D91DE2,
    //5: 01 00 70 00 16 00,
    0x13D91E62
};

int enemy_room_146[] = {
    //num enemy types: 1, num enemies 4,
    //0x14136304, 0x14136308,
    0x01,
    //room spawners:,
    0x14136332,
    //values: 01 00 36 00 0F 00  //0x36 - Storm Skeleton,
    //0x1413633C, //+0x0A address,
    //1: 01 00 36 00 04 00,
    0x14136352,
    //2: 01 00 36 00 04 00,
    0x141363D2,
    //3: 01 00 36 00 04 00,
    0x14136452,
    //4: 01 00 36 00 04 00,
    0x141364D2
};

int enemy_room_147[] = {
    //num enemy types: 1, num enemies 2,
    //0x14467A04, 0x14467A08,
    0x01,
    //room spawners:,
    0x14467A32,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x14467A3C, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x14467A52,
    //2: 01 00 54 00 04 00,
    0x14467AD2
};

int enemy_room_148[] = {
    //num enemy types: 4, num enemies 8,
    //0x145F2984, 0x145F2988,
    0x04,
    //room spawners:,
    0x145F29B2,
    //values: 01 00 0C 00 0D 00  //0x0C - Skeleton,
    //0x145F29BC, //+0x0A address,
    //room spawners:,
    0x145F29C6,
    //values: 01 00 67 00 02 00  //0x67 - Spartacus,
    //0x145F29D0, //+0x0A address,
    //room spawners:,
    0x145F29DA,
    //values: 01 00 20 00 02 00  //0x20 - Axe Knight,
    //0x145F29E4, //+0x0A address,
    //room spawners:,
    0x145F29EE,
    //values: 01 00 43 00 0F 00  //0x43 - ???,
    //0x145F29F8, //+0x0A address,
    //1: 01 00 0C 00 04 00,
    0x145F2A02,
    //2: 01 00 0C 00 04 00,
    0x145F2A82,
    //3: 01 00 0C 00 04 00,
    0x145F2B02,
    //4: 01 00 43 00 04 00,
    0x145F2B82,
    //5: 01 00 67 00 04 00,
    0x145F2C02,
    //6: 01 00 67 00 04 00,
    0x145F2C82,
    //7: 01 00 67 00 04 00,
    0x145F2D02,
    //8: 01 00 20 00 04 00,
    0x145F2D82
};

int enemy_room_149[] = {
    //num enemy types: 2, num enemies 2,
    //0x1477FE04, 0x1477FE08,
    0x02,
    //room spawners:,
    0x1477FE32,
    //values: 01 00 47 00 0F 00  //0x47 - Ghost,
    //0x1477FE3C, //+0x0A address,
    //room spawners:,
    0x1477FE46,
    //values: 01 00 6D 00 02 00  //0x6D - Thunder Demon,
    //0x1477FE50, //+0x0A address,
    //1: 01 00 47 00 14 00,
    0x1477FE62,
    //2: 01 00 6D 00 04 00,
    0x1477FEE2
};

int enemy_room_150[] = {
    //num enemy types: 1, num enemies 2,
    //0x1491E984, 0x1491E988,
    0x01,
    //room spawners:,
    0x1491E9B2,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x1491E9BC, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x1491E9D2,
    //2: 01 00 54 00 04 00,
    0x1491EA52
};

int enemy_room_151[] = {
    //num enemy types: 3, num enemies 5,
    //0x14AB1104, 0x14AB1108,
    0x03,
    //room spawners:,
    0x14AB1132,
    //values: 01 00 35 00 0F 00  //0x35 - Evil Stabber,
    //0x14AB113C, //+0x0A address,
    //room spawners:,
    0x14AB1146,
    //values: 01 00 03 00 0F 00  //0x03 - Red Skeleton,
    //0x14AB1150, //+0x0A address,
    //room spawners:,
    0x14AB115A,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x14AB1164, //+0x0A address,
    //1: 01 00 35 00 04 00,
    0x14AB1172,
    //2: 01 00 03 00 04 00,
    0x14AB11F2,
    //3: 01 00 03 00 04 00,
    0x14AB1272,
    //4: 01 00 35 00 04 00,
    0x14AB12F2,
    //5: 01 00 54 00 04 00,
    0x14AB1372
};

int enemy_room_152[] = {
    //num enemy types: 3, num enemies 8,
    //0x14E47084, 0x14E47088,
    0x03,
    //room spawners:,
    0x14E470B2,
    //values: 01 00 0A 00 0F 00  //0x0A - Axe Armor,
    //0x14E470BC, //+0x0A address,
    //room spawners:,
    0x14E470C6,
    //values: 01 00 35 00 0F 00  //0x35 - Evil Stabber,
    //0x14E470D0, //+0x0A address,
    //room spawners:,
    0x14E470DA,
    //values: 01 00 71 00 02 00  //0x71 - Lizard Knight,
    //0x14E470E4, //+0x0A address,
    //1: 01 00 71 00 06 00,
    0x14E470F2,
    //2: 01 00 71 00 06 00,
    0x14E47172,
    //3: 01 00 71 00 06 00,
    0x14E471F2,
    //4: 01 00 71 00 06 00,
    0x14E47272,
    //5: 01 00 71 00 06 00,
    0x14E472F2,
    //6: 01 00 0A 00 16 00,
    0x14E47372,
    //7: 01 00 35 00 06 00,
    0x14E473F2,
    //8: 01 00 35 00 06 00,
    0x14E47472
};

int enemy_room_153[] = {
    //num enemy types: 2, num enemies 6,
    //0x15174104, 0x15174108,
    0x02,
    //room spawners:,
    0x15174132,
    //values: 01 00 11 00 0F 00  //0x11 - Skeleton Swordsman,
    //0x1517413C, //+0x0A address,
    //room spawners:,
    0x15174146,
    //values: 01 00 17 00 0F 00  //0x17 - Skeleton Archer,
    //0x15174150, //+0x0A address,
    //1: 01 00 11 00 05 00,
    0x15174162,
    //2: 01 00 11 00 05 00,
    0x151741E2,
    //3: 01 00 11 00 05 00,
    0x15174262,
    //4: 01 00 11 00 05 00,
    0x151742E2,
    //5: 01 00 17 00 05 00,
    0x15174362,
    //6: 01 00 17 00 05 00,
    0x151743E2
};

int enemy_room_154[] = {
    //num enemy types: 1, num enemies 2,
    //0x15665484, 0x15665488,
    0x01,
    //room spawners:,
    0x156654B2,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x156654BC, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x156654D2,
    //2: 01 00 54 00 04 00,
    0x15665552
};

int enemy_room_155[] = {
    //num enemy types: 3, num enemies 6,
    //0x1599D184, 0x1599D188,
    0x03,
    //room spawners:,
    0x1599D1B2,
    //values: 01 00 4C 00 0F 00  //0x4C - Shadow Wolf,
    //0x1599D1BC, //+0x0A address,
    //room spawners:,
    0x1599D1C6,
    //values: 01 00 47 00 0F 00  //0x47 - Ghost,
    //0x1599D1D0, //+0x0A address,
    //room spawners:,
    0x1599D1DA,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x1599D1E4, //+0x0A address,
    //1: 01 00 47 00 04 00,
    0x1599D1F2,
    //2: 01 00 47 00 04 00,
    0x1599D272,
    //3: 01 00 47 00 04 00,
    0x1599D2F2,
    //4: 01 00 4C 00 04 00,
    0x1599D372,
    //5: 01 00 4C 00 04 00,
    0x1599D3F2,
    //6: 01 00 54 00 04 00,
    0x1599D472
};

int enemy_room_156[] = {
    //num enemy types: 3, num enemies 4,
    //0x15B0D484, 0x15B0D488,
    0x03,
    //room spawners:,
    0x15B0D4B2,
    //values: 01 00 0C 00 0D 00  //0x0C - Skeleton,
    //0x15B0D4BC, //+0x0A address,
    //room spawners:,
    0x15B0D4C6,
    //values: 01 00 69 00 02 00  //0x69 - Death Ripper,
    //0x15B0D4D0, //+0x0A address,
    //room spawners:,
    0x15B0D4DA,
    //values: 01 00 6D 00 02 00  //0x6D - Thunder Demon,
    //0x15B0D4E4, //+0x0A address,
    //1: 01 00 0C 00 1C 00,
    0x15B0D4F2,
    //2: 01 00 6D 00 04 00,
    0x15B0D572,
    //3: 01 00 6D 00 04 00,
    0x15B0D5F2,
    //4: 01 00 69 00 14 00,
    0x15B0D672
};

int enemy_room_157[] = {
    //num enemy types: 2, num enemies 9,
    //0x15C7DB84, 0x15C7DB88,
    0x02,
    //room spawners:,
    0x15C7DBB2,
    //values: 01 00 11 00 0F 00  //0x11 - Skeleton Swordsman,
    //0x15C7DBBC, //+0x0A address,
    //room spawners:,
    0x15C7DBC6,
    //values: 01 00 15 00 02 00  //0x15 - Armor Knight,
    //0x15C7DBD0, //+0x0A address,
    //1: 01 00 11 00 05 00,
    0x15C7DBE2,
    //2: 01 00 11 00 05 00,
    0x15C7DC62,
    //3: 01 00 11 00 05 00,
    0x15C7DCE2,
    //4: 01 00 11 00 05 00,
    0x15C7DD62,
    //5: 01 00 11 00 05 00,
    0x15C7DDE2,
    //6: 01 00 11 00 05 00,
    0x15C7DE62,
    //7: 01 00 15 00 05 00,
    0x15C7DEE2,
    //8: 01 00 15 00 05 00,
    0x15C7DF62,
    //9: 01 00 15 00 05 00,
    0x15C7DFE2
};

int enemy_room_158[] = {
    //num enemy types: 2, num enemies 4,
    //0x15E02D04, 0x15E02D08,
    0x02,
    //room spawners:,
    0x15E02D32,
    //values: 01 00 47 00 0F 00  //0x47 - Ghost,
    //0x15E02D3C, //+0x0A address,
    //room spawners:,
    0x15E02D46,
    //values: 01 00 08 00 0F 00  //0x08 - Bat,
    //0x15E02D50, //+0x0A address,
    //1: 01 00 47 00 14 00,
    0x15E02D62,
    //2: 01 00 08 00 14 00,
    0x15E02DE2,
    //3: 01 00 08 00 04 00,
    0x15E02E62,
    //4: 01 00 08 00 04 00,
    0x15E02EE2
};

int enemy_room_159[] = {
    //num enemy types: 4, num enemies 8,
    //0x16342E84, 0x16342E88,
    0x04,
    //room spawners:,
    0x16342EB2,
    //values: 01 00 67 00 02 00  //0x67 - Spartacus,
    //0x16342EBC, //+0x0A address,
    //room spawners:,
    0x16342EC6,
    //values: 01 00 35 00 0F 00  //0x35 - Evil Stabber,
    //0x16342ED0, //+0x0A address,
    //room spawners:,
    0x16342EDA,
    //values: 01 00 0D 00 0D 00  //0x0D - Skeleton Soldier,
    //0x16342EE4, //+0x0A address,
    //room spawners:,
    0x16342EEE,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x16342EF8, //+0x0A address,
    //1: 01 00 35 00 04 00,
    0x16342F02,
    //2: 01 00 0D 00 04 00,
    0x16342F82,
    //3: 01 00 0D 00 04 00,
    0x16343002,
    //4: 01 00 0D 00 04 00,
    0x16343082,
    //5: 01 00 67 00 04 00,
    0x16343102,
    //6: 01 00 67 00 04 00,
    0x16343182,
    //7: 01 00 67 00 04 00,
    0x16343202,
    //8: 01 00 54 00 04 00,
    0x16343282
};

int enemy_room_160[] = {
    //num enemy types: 3, num enemies 5,
    //0x1653B184, 0x1653B188,
    0x03,
    //room spawners:,
    0x1653B1B2,
    //values: 01 00 16 00 0F 00  //0x16 - Gargoyle,
    //0x1653B1BC, //+0x0A address,
    //room spawners:,
    0x1653B1C6,
    //values: 01 00 4C 00 0F 00  //0x4C - Shadow Wolf,
    //0x1653B1D0, //+0x0A address,
    //room spawners:,
    0x1653B1DA,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x1653B1E4, //+0x0A address,
    //1: 01 00 16 00 04 00,
    0x1653B1F2,
    //2: 01 00 4C 00 04 00,
    0x1653B272,
    //3: 01 00 4C 00 04 00,
    0x1653B2F2,
    //4: 01 00 4C 00 04 00,
    0x1653B372,
    //5: 01 00 54 00 04 00,
    0x1653B3F2
};

int enemy_room_161[] = {
    //num enemy types: 3, num enemies 9,
    //0x166AFB04, 0x166AFB08,
    0x03,
    //room spawners:,
    0x166AFB32,
    //values: 01 00 3A 00 0F 00  //0x3A - Buckbaird,
    //0x166AFB3C, //+0x0A address,
    //room spawners:,
    0x166AFB46,
    //values: 01 00 23 00 0F 00  //0x23 - Astral Warrior,
    //0x166AFB50, //+0x0A address,
    //room spawners:,
    0x166AFB5A,
    //values: 01 00 2E 00 02 00  //0x2E - Dullahan,
    //0x166AFB64, //+0x0A address,
    //1: 01 00 3A 00 06 00,
    0x166AFB72,
    //2: 01 00 3A 00 06 00,
    0x166AFBF2,
    //3: 01 00 23 00 06 00,
    0x166AFC72,
    //4: 01 00 23 00 06 00,
    0x166AFCF2,
    //5: 01 00 2E 00 06 00,
    0x166AFD72,
    //6: 01 00 2E 00 06 00,
    0x166AFDF2,
    //7: 01 00 2E 00 06 00,
    0x166AFE72,
    //8: 01 00 23 00 06 00,
    0x166AFEF2,
    //9: 01 00 23 00 06 00,
    0x166AFF72
};

int enemy_room_162[] = {
    //num enemy types: 2, num enemies 3,
    //0x16818A84, 0x16818A88,
    0x02,
    //room spawners:,
    0x16818AB2,
    //values: 01 00 47 00 0F 00  //0x47 - Ghost,
    //0x16818ABC, //+0x0A address,
    //room spawners:,
    0x16818AC6,
    //values: 01 00 2E 00 02 00  //0x2E - Dullahan,
    //0x16818AD0, //+0x0A address,
    //1: 01 00 47 00 14 00,
    0x16818AE2,
    //2: 01 00 2E 00 04 00,
    0x16818B62,
    //3: 01 00 2E 00 04 00,
    0x16818BE2
};

int enemy_room_163[] = {
    //num enemy types: 1, num enemies 3,
    //0x16978984, 0x16978988,
    0x01,
    //room spawners:,
    0x169789B2,
    //values: 01 00 3A 00 0F 00  //0x3A - Buckbaird,
    //0x169789BC, //+0x0A address,
    //1: 01 00 3A 00 04 00,
    0x169789D2,
    //2: 01 00 3A 00 04 00,
    0x16978A52,
    //3: 01 00 3A 00 04 00,
    0x16978AD2
};

int enemy_room_164[] = {
    //num enemy types: 3, num enemies 8,
    //0x16B71784, 0x16B71788,
    0x03,
    //room spawners:,
    0x16B717B2,
    //values: 01 00 16 00 0F 00  //0x16 - Gargoyle,
    //0x16B717BC, //+0x0A address,
    //room spawners:,
    0x16B717C6,
    //values: 01 00 30 00 02 00  //0x30 - Skeleton Hunter,
    //0x16B717D0, //+0x0A address,
    //room spawners:,
    0x16B717DA,
    //values: 01 00 17 00 0D 00  //0x17 - Skeleton Archer,
    //0x16B717E4, //+0x0A address,
    //1: 01 00 16 00 04 00,
    0x16B717F2,
    //2: 01 00 17 00 04 00,
    0x16B71872,
    //3: 01 00 17 00 04 00,
    0x16B718F2,
    //4: 01 00 17 00 04 00,
    0x16B71972,
    //5: 01 00 30 00 04 00,
    0x16B719F2,
    //6: 01 00 30 00 04 00,
    0x16B71A72,
    //7: 01 00 30 00 04 00,
    0x16B71AF2,
    //8: 01 00 30 00 04 00,
    0x16B71B72
};

int enemy_room_165[] = {
    //num enemy types: 2, num enemies 6,
    //0x16F2F104, 0x16F2F108,
    0x02,
    //room spawners:,
    0x16F2F132,
    //values: 01 00 17 00 0F 00  //0x17 - Skeleton Archer,
    //0x16F2F13C, //+0x0A address,
    //room spawners:,
    0x16F2F146,
    //values: 01 00 6D 00 0F 00  //0x6D - Thunder Demon,
    //0x16F2F150, //+0x0A address,
    //1: 01 00 17 00 04 00,
    0x16F2F162,
    //2: 01 00 17 00 04 00,
    0x16F2F1E2,
    //3: 01 00 6D 00 04 00,
    0x16F2F262,
    //4: 01 00 17 00 04 00,
    0x16F2F2E2,
    //5: 01 00 17 00 04 00,
    0x16F2F362,
    //6: 01 00 6D 00 04 00,
    0x16F2F3E2
};

int enemy_room_166[] = {
    //num enemy types: 3, num enemies 4,
    //0x17120004, 0x17120008,
    0x03,
    //room spawners:,
    0x17120032,
    //values: 01 00 1B 00 0F 00  //0x1B - Man-Eating Plant,
    //0x1712003C, //+0x0A address,
    //room spawners:,
    0x17120046,
    //values: 01 00 19 00 0F 00  //0x19 - Mist,
    //0x17120050, //+0x0A address,
    //room spawners:,
    0x1712005A,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x17120064, //+0x0A address,
    //1: 01 00 1B 00 04 00,
    0x17120072,
    //2: 01 00 19 00 05 00,
    0x171200F2,
    //3: 01 00 19 00 05 00,
    0x17120172,
    //4: 01 00 54 00 04 00,
    0x171201F2
};

int enemy_room_167[] = {
    //num enemy types: 1, num enemies 2,
    //0x172BE184, 0x172BE188,
    0x01,
    //room spawners:,
    0x172BE1B2,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x172BE1BC, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x172BE1D2,
    //2: 01 00 54 00 04 00,
    0x172BE252
};

int enemy_room_168[] = {
    //num enemy types: 1, num enemies 1,
    //0x1745D484, 0x1745D488,
    0x01,
    //room spawners:,
    0x1745D4B2,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x1745D4BC, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x1745D4D2
};

int enemy_room_169[] = {
    //num enemy types: 1, num enemies 2,
    //0x175BC504, 0x175BC508,
    0x01,
    //room spawners:,
    0x175BC532,
    //values: 01 00 2F 00 02 00  //0x2F - Phantom,
    //0x175BC53C, //+0x0A address,
    //1: 01 00 2F 00 04 00,
    0x175BC552,
    //2: 01 00 2F 00 04 00,
    0x175BC5D2
};

int enemy_room_170[] = {
    //num enemy types: 2, num enemies 8,
    //0x17906B84, 0x17906B88,
    0x02,
    //room spawners:,
    0x17906BB2,
    //values: 01 00 15 00 0F 00  //0x15 - Armor Knight,
    //0x17906BBC, //+0x0A address,
    //room spawners:,
    0x17906BC6,
    //values: 01 00 58 00 02 00  //0x58 - Executioner,
    //0x17906BD0, //+0x0A address,
    //1: 01 00 15 00 05 00,
    0x17906BE2,
    //2: 01 00 15 00 05 00,
    0x17906C62,
    //3: 01 00 15 00 05 00,
    0x17906CE2,
    //4: 01 00 15 00 05 00,
    0x17906D62,
    //5: 01 00 58 00 05 00,
    0x17906DE2,
    //6: 01 00 58 00 05 00,
    0x17906E62,
    //7: 01 00 58 00 05 00,
    0x17906EE2,
    //8: 01 00 58 00 05 00,
    0x17906F62
};

int enemy_room_171[] = {
    //num enemy types: 4, num enemies 12,
    //0x17B57D04, 0x17B57D08,
    0x04,
    //room spawners:,
    0x17B57D32,
    //values: 01 00 14 00 0F 00  //0x14 - Skeleton Flower,
    //0x17B57D3C, //+0x0A address,
    //room spawners:,
    0x17B57D46,
    //values: 01 00 69 00 02 00  //0x69 - Death Ripper,
    //0x17B57D50, //+0x0A address,
    //room spawners:,
    0x17B57D5A,
    //values: 01 00 70 00 0F 00  //0x70 - Poison Lizard,
    //0x17B57D64, //+0x0A address,
    //room spawners:,
    0x17B57D6E,
    //values: 01 00 3A 00 0F 00  //0x3A - Buckbaird,
    //0x17B57D78, //+0x0A address,
    //1: 01 00 14 00 06 00,
    0x17B57D82,
    //2: 01 00 3A 00 06 00,
    0x17B57E02,
    //3: 01 00 3A 00 06 00,
    0x17B57E82,
    //4: 01 00 3A 00 06 00,
    0x17B57F02,
    //5: 01 00 3A 00 06 00,
    0x17B57F82,
    //6: 01 00 14 00 06 00,
    0x17B58002,
    //7: 01 00 70 00 16 00,
    0x17B58082,
    //8: 01 00 14 00 06 00,
    0x17B58102,
    //9: 01 00 14 00 06 00,
    0x17B58182,
    //10: 01 00 69 00 06 00,
    0x17B58202,
    //11: 01 00 69 00 06 00,
    0x17B58282,
    //12: 01 00 69 00 06 00,
    0x17B58302
};

int enemy_room_172[] = {
    //num enemy types: 2, num enemies 3,
    //0x17D3A684, 0x17D3A688,
    0x02,
    //room spawners:,
    0x17D3A6B2,
    //values: 01 00 14 00 0F 00  //0x14 - Skeleton Flower,
    //0x17D3A6BC, //+0x0A address,
    //room spawners:,
    0x17D3A6C6,
    //values: 01 00 16 00 0F 00  //0x16 - Gargoyle,
    //0x17D3A6D0, //+0x0A address,
    //1: 01 00 14 00 04 00,
    0x17D3A6E2,
    //2: 01 00 16 00 04 00,
    0x17D3A762,
    //3: 01 00 16 00 04 00,
    0x17D3A7E2
};

int enemy_room_173[] = {
    //num enemy types: 2, num enemies 6,
    //0x17EC5104, 0x17EC5108,
    0x02,
    //room spawners:,
    0x17EC5132,
    //values: 01 00 15 00 0F 00  //0x15 - Armor Knight,
    //0x17EC513C, //+0x0A address,
    //room spawners:,
    0x17EC5146,
    //values: 01 00 47 00 0F 00  //0x47 - Ghost,
    //0x17EC5150, //+0x0A address,
    //1: 01 00 15 00 05 00,
    0x17EC5162,
    //2: 01 00 15 00 05 00,
    0x17EC51E2,
    //3: 01 00 47 00 14 00,
    0x17EC5262,
    //4: 01 00 15 00 05 00,
    0x17EC52E2,
    //5: 01 00 15 00 05 00,
    0x17EC5362,
    //6: 01 00 47 00 14 00,
    0x17EC53E2
};

int enemy_room_174[] = {
    //num enemy types: 2, num enemies 3,
    //0x180B9204, 0x180B9208,
    0x02,
    //room spawners:,
    0x180B9232,
    //values: 01 00 1B 00 0F 00  //0x1B - Man-Eating Plant,
    //0x180B923C, //+0x0A address,
    //room spawners:,
    0x180B9246,
    //values: 01 00 3A 00 0F 00  //0x3A - Buckbaird,
    //0x180B9250, //+0x0A address,
    //1: 01 00 1B 00 04 00,
    0x180B9262,
    //2: 01 00 3A 00 04 00,
    0x180B92E2,
    //3: 01 00 3A 00 04 00,
    0x180B9362
};

int enemy_room_175[] = {
    //num enemy types: 2, num enemies 3,
    //0x1829AA04, 0x1829AA08,
    0x02,
    //room spawners:,
    0x1829AA32,
    //values: 01 00 19 00 0F 00  //0x19 - Mist,
    //0x1829AA3C, //+0x0A address,
    //room spawners:,
    0x1829AA46,
    //values: 01 00 4C 00 0F 00  //0x4C - Shadow Wolf,
    //0x1829AA50, //+0x0A address,
    //1: 01 00 19 00 04 00,
    0x1829AA62,
    //2: 01 00 4C 00 04 00,
    0x1829AAE2,
    //3: 01 00 4C 00 04 00,
    0x1829AB62
};

int enemy_room_176[] = {
    //num enemy types: 2, num enemies 5,
    //0x18481504, 0x18481508,
    0x02,
    //room spawners:,
    0x18481532,
    //values: 01 00 70 00 0F 00  //0x70 - Poison Lizard,
    //0x1848153C, //+0x0A address,
    //room spawners:,
    0x18481546,
    //values: 01 00 6D 00 0F 00  //0x6D - Thunder Demon,
    //0x18481550, //+0x0A address,
    //1: 01 00 70 00 04 00,
    0x18481562,
    //2: 01 00 70 00 04 00,
    0x184815E2,
    //3: 01 00 70 00 04 00,
    0x18481662,
    //4: 01 00 6D 00 04 00,
    0x184816E2,
    //5: 01 00 70 00 04 00,
    0x18481762
};

int enemy_room_177[] = {
    //num enemy types: 3, num enemies 7,
    //0x1882FB84, 0x1882FB88,
    0x03,
    //room spawners:,
    0x1882FBB2,
    //values: 01 00 14 00 0F 00  //0x14 - Skeleton Flower,
    //0x1882FBBC, //+0x0A address,
    //room spawners:,
    0x1882FBC6,
    //values: 01 00 69 00 02 00  //0x69 - Death Ripper,
    //0x1882FBD0, //+0x0A address,
    //room spawners:,
    0x1882FBDA,
    //values: 01 00 70 00 0F 00  //0x70 - Poison Lizard,
    //0x1882FBE4, //+0x0A address,
    //1: 01 00 14 00 06 00,
    0x1882FBF2,
    //2: 01 00 14 00 06 00,
    0x1882FC72,
    //3: 01 00 14 00 06 00,
    0x1882FCF2,
    //4: 01 00 70 00 06 00,
    0x1882FD72,
    //5: 01 00 69 00 06 00,
    0x1882FDF2,
    //6: 01 00 69 00 06 00,
    0x1882FE72,
    //7: 01 00 69 00 06 00,
    0x1882FEF2
};

int enemy_room_178[] = {
    //num enemy types: 1, num enemies 1,
    //0x18A1DF04, 0x18A1DF08,
    0x01,
    //room spawners:,
    0x18A1DF32,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x18A1DF3C, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x18A1DF52
};

int enemy_room_179[] = {
    //num enemy types: 2, num enemies 6,
    //0x18BAA084, 0x18BAA088,
    0x02,
    //room spawners:,
    0x18BAA0B2,
    //values: 01 00 0E 00 0F 00  //0x0E - Thunder Sword,
    //0x18BAA0BC, //+0x0A address,
    //room spawners:,
    0x18BAA0C6,
    //values: 01 00 01 00 0F 00  //0x01 - Spirit,
    //0x18BAA0D0, //+0x0A address,
    //1: 01 00 0E 00 05 00,
    0x18BAA0E2,
    //2: 01 00 0E 00 05 00,
    0x18BAA162,
    //3: 01 00 0E 00 05 00,
    0x18BAA1E2,
    //4: 01 00 0E 00 05 00,
    0x18BAA262,
    //5: 01 00 01 00 14 00,
    0x18BAA2E2,
    //6: 01 00 0E 00 04 00,
    0x18BAA362
};

int enemy_room_180[] = {
    //num enemy types: 1, num enemies 2,
    //0x18D2EA04, 0x18D2EA08,
    0x01,
    //room spawners:,
    0x18D2EA32,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x18D2EA3C, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x18D2EA52,
    //2: 01 00 54 00 04 00,
    0x18D2EAD2
};

int enemy_room_181[] = {
    //num enemy types: 2, num enemies 2,
    //0x18EC4004, 0x18EC4008,
    0x02,
    //room spawners:,
    0x18EC4032,
    //values: 01 00 01 00 0D 00  //0x01 - Spirit,
    //0x18EC403C, //+0x0A address,
    //room spawners:,
    0x18EC4046,
    //values: 01 00 16 00 02 00  //0x16 - Gargoyle,
    //0x18EC4050, //+0x0A address,
    //1: 01 00 01 00 24 00,
    0x18EC4062,
    //2: 01 00 16 00 24 00,
    0x18EC40E2
};

int enemy_room_182[] = {
    //num enemy types: 3, num enemies 4,
    //0x19092204, 0x19092208,
    0x03,
    //room spawners:,
    0x19092232,
    //values: 01 00 0D 00 0B 00  //0x0D - Skeleton Soldier,
    //0x1909223C, //+0x0A address,
    //room spawners:,
    0x19092246,
    //values: 01 00 47 00 0F 00  //0x47 - Ghost,
    //0x19092250, //+0x0A address,
    //room spawners:,
    0x1909225A,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x19092264, //+0x0A address,
    //1: 01 00 47 00 14 00,
    0x19092272,
    //2: 01 00 0D 00 04 00,
    0x190922F2,
    //3: 01 00 0D 00 04 00,
    0x19092372,
    //4: 01 00 54 00 04 00,
    0x190923F2
};

int enemy_room_183[] = {
    //num enemy types: 2, num enemies 4,
    //0x192A3A04, 0x192A3A08,
    0x02,
    //room spawners:,
    0x192A3A32,
    //values: 01 00 70 00 0F 00  //0x70 - Poison Lizard,
    //0x192A3A3C, //+0x0A address,
    //room spawners:,
    0x192A3A46,
    //values: 01 00 47 00 0F 00  //0x47 - Ghost,
    //0x192A3A50, //+0x0A address,
    //1: 01 00 70 00 04 00,
    0x192A3A62,
    //2: 01 00 70 00 04 00,
    0x192A3AE2,
    //3: 01 00 70 00 04 00,
    0x192A3B62,
    //4: 01 00 47 00 14 00,
    0x192A3BE2
};

int enemy_room_184[] = {
    //num enemy types: 1, num enemies 2,
    //0x19442704, 0x19442708,
    0x01,
    //room spawners:,
    0x19442732,
    //values: 01 00 16 00 0F 00  //0x16 - Gargoyle,
    //0x1944273C, //+0x0A address,
    //1: 01 00 16 00 04 00,
    0x19442752,
    //2: 01 00 16 00 04 00,
    0x194427D2
};

int enemy_room_185[] = {
    //num enemy types: 2, num enemies 3,
    //0x195B0684, 0x195B0688,
    0x02,
    //room spawners:,
    0x195B06B2,
    //values: 01 00 35 00 0F 00  //0x35 - Evil Stabber,
    //0x195B06BC, //+0x0A address,
    //room spawners:,
    0x195B06C6,
    //values: 01 00 6D 00 0F 00  //0x6D - Thunder Demon,
    //0x195B06D0, //+0x0A address,
    //1: 01 00 35 00 04 00,
    0x195B06E2,
    //2: 01 00 6D 00 04 00,
    0x195B0762,
    //3: 01 00 6D 00 04 00,
    0x195B07E2
};

int enemy_room_186[] = {
    //num enemy types: 1, num enemies 3,
    //0x1973EA04, 0x1973EA08,
    0x01,
    //room spawners:,
    0x1973EA32,
    //values: 01 00 0E 00 0F 00  //0x0E - Thunder Sword,
    //0x1973EA3C, //+0x0A address,
    //1: 01 00 0E 00 04 00,
    0x1973EA52,
    //2: 01 00 0E 00 04 00,
    0x1973EAD2,
    //3: 01 00 0E 00 04 00,
    0x1973EB52
};

int enemy_room_187[] = {
    //num enemy types: 1, num enemies 2,
    //0x198E0B04, 0x198E0B08,
    0x01,
    //room spawners:,
    0x198E0B32,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x198E0B3C, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x198E0B52,
    //2: 01 00 54 00 04 00,
    0x198E0BD2
};

int enemy_room_188[] = {
    //num enemy types: 7, num enemies 15,
    //0x19ACBD84, 0x19ACBD88,
    0x07,
    //room spawners:,
    0x19ACBDB2,
    //values: 01 00 68 00 00 00  //0x68 - ???,
    //0x19ACBDBC, //+0x0A address,
    //room spawners:,
    0x19ACBDC6,
    //values: 01 00 72 00 00 00  //0x72 - ???,
    //0x19ACBDD0, //+0x0A address,
    //room spawners:,
    0x19ACBDDA,
    //values: 01 00 76 00 00 00  //0x76 - ???,
    //0x19ACBDE4, //+0x0A address,
    //room spawners:,
    0x19ACBDEE,
    //values: 01 00 73 00 00 00  //0x73 - ???,
    //0x19ACBDF8, //+0x0A address,
    //room spawners:,
    0x19ACBE02,
    //values: 01 00 74 00 00 00  //0x74 - ???,
    //0x19ACBE0C, //+0x0A address,
    //room spawners:,
    0x19ACBE16,
    //values: 01 00 4D 00 00 00  //0x4D - ???,
    //0x19ACBE20, //+0x0A address,
    //room spawners:,
    0x19ACBE2A,
    //values: 01 00 32 00 00 00  //0x32 - Medusa,
    //0x19ACBE34, //+0x0A address,
    //1: 01 00 68 00 06 00,
    0x19ACBE42,
    //2: 01 00 32 00 06 00,
    0x19ACBEC2,
    //3: 01 00 4D 00 06 00,
    0x19ACBF42,
    //4: 01 00 68 00 06 00,
    0x19ACBFC2,
    //5: 01 00 76 00 0C 00,
    0x19ACC042,
    //6: 01 00 74 00 06 00,
    0x19ACC0C2,
    //7: 01 00 73 00 06 00,
    0x19ACC142,
    //8: 01 00 72 00 06 00,
    0x19ACC1C2,
    //9: 01 00 73 00 06 00,
    0x19ACC242,
    //10: 01 00 72 00 06 00,
    0x19ACC2C2,
    //11: 01 00 72 00 06 00,
    0x19ACC342,
    //12: 01 00 72 00 06 00,
    0x19ACC3C2,
    //13: 01 00 72 00 06 00,
    0x19ACC442,
    //14: 01 00 72 00 06 00,
    0x19ACC4C2,
    //15: 01 00 72 00 06 00,
    0x19ACC542
};

int enemy_room_189[] = {
    //num enemy types: 2, num enemies 3,
    //0x1A160604, 0x1A160608,
    0x02,
    //room spawners:,
    0x1A160632,
    //values: 01 00 20 00 00 00  //0x20 - Axe Knight,
    //0x1A16063C, //+0x0A address,
    //room spawners:,
    0x1A160646,
    //values: 01 00 70 00 00 00  //0x70 - Poison Lizard,
    //0x1A160650, //+0x0A address,
    //1: 01 00 20 00 04 00,
    0x1A160662,
    //2: 01 00 70 00 04 00,
    0x1A1606E2,
    //3: 01 00 70 00 04 00,
    0x1A160762
};

int enemy_room_190[] = {
    //num enemy types: 4, num enemies 11,
    //0x1A2A6804, 0x1A2A6808,
    0x04,
    //room spawners:,
    0x1A2A6832,
    //values: 01 00 63 00 00 00  //0x63 - Thunder Elemental,
    //0x1A2A683C, //+0x0A address,
    //room spawners:,
    0x1A2A6846,
    //values: 01 00 73 00 00 00  //0x73 - ???,
    //0x1A2A6850, //+0x0A address,
    //room spawners:,
    0x1A2A685A,
    //values: 01 00 72 00 00 00  //0x72 - ???,
    //0x1A2A6864, //+0x0A address,
    //room spawners:,
    0x1A2A686E,
    //values: 01 00 74 00 00 00  //0x74 - ???,
    //0x1A2A6878, //+0x0A address,
    //1: 01 00 63 00 06 00,
    0x1A2A6882,
    //2: 01 00 74 00 06 00,
    0x1A2A6902,
    //3: 01 00 72 00 06 00,
    0x1A2A6982,
    //4: 01 00 73 00 06 00,
    0x1A2A6A02,
    //5: 01 00 72 00 06 00,
    0x1A2A6A82,
    //6: 01 00 72 00 06 00,
    0x1A2A6B02,
    //7: 01 00 63 00 06 00,
    0x1A2A6B82,
    //8: 01 00 72 00 06 00,
    0x1A2A6C02,
    //9: 01 00 72 00 06 00,
    0x1A2A6C82,
    //10: 01 00 72 00 06 00,
    0x1A2A6D02,
    //11: 01 00 72 00 06 00,
    0x1A2A6D82
};

int enemy_room_191[] = {
    //num enemy types: 2, num enemies 3,
    //0x1A4D9B04, 0x1A4D9B08,
    0x02,
    //room spawners:,
    0x1A4D9B32,
    //values: 01 00 44 00 00 00  //0x44 - ???,
    //0x1A4D9B3C, //+0x0A address,
    //room spawners:,
    0x1A4D9B46,
    //values: 01 00 68 00 00 00  //0x68 - ???,
    //0x1A4D9B50, //+0x0A address,
    //1: 01 00 68 00 07 00,
    0x1A4D9B62,
    //2: 01 00 68 00 07 00,
    0x1A4D9BE2,
    //3: 01 00 44 00 06 00,
    0x1A4D9C62
};

int enemy_room_192[] = {
    //num enemy types: 2, num enemies 5,
    //0x1A82A904, 0x1A82A908,
    0x02,
    //room spawners:,
    0x1A82A932,
    //values: 01 00 67 00 0F 00  //0x67 - Spartacus,
    //0x1A82A93C, //+0x0A address,
    //room spawners:,
    0x1A82A946,
    //values: 01 00 34 00 02 00  //0x34 - Gaap,
    //0x1A82A950, //+0x0A address,
    //1: 01 00 67 00 04 00,
    0x1A82A962,
    //2: 01 00 67 00 04 00,
    0x1A82A9E2,
    //3: 01 00 34 00 04 00,
    0x1A82AA62,
    //4: 01 00 67 00 04 00,
    0x1A82AAE2,
    //5: 01 00 67 00 04 00,
    0x1A82AB62
};

int enemy_room_193[] = {
    //num enemy types: 2, num enemies 6,
    //0x1A9D8684, 0x1A9D8688,
    0x02,
    //room spawners:,
    0x1A9D86B2,
    //values: 01 00 34 00 0F 00  //0x34 - Gaap,
    //0x1A9D86BC, //+0x0A address,
    //room spawners:,
    0x1A9D86C6,
    //values: 01 00 13 00 0F 00  //0x13 - Chaos Sword,
    //0x1A9D86D0, //+0x0A address,
    //1: 01 00 34 00 05 00,
    0x1A9D86E2,
    //2: 01 00 34 00 05 00,
    0x1A9D8762,
    //3: 01 00 13 00 04 00,
    0x1A9D87E2,
    //4: 01 00 13 00 04 00,
    0x1A9D8862,
    //5: 01 00 13 00 04 00,
    0x1A9D88E2,
    //6: 01 00 13 00 04 00,
    0x1A9D8962
};

int enemy_room_194[] = {
    //num enemy types: 2, num enemies 5,
    //0x1AB64D84, 0x1AB64D88,
    0x02,
    //room spawners:,
    0x1AB64DB2,
    //values: 01 00 69 00 0F 00  //0x69 - Death Ripper,
    //0x1AB64DBC, //+0x0A address,
    //room spawners:,
    0x1AB64DC6,
    //values: 01 00 13 00 0F 00  //0x13 - Chaos Sword,
    //0x1AB64DD0, //+0x0A address,
    //1: 01 00 69 00 04 00,
    0x1AB64DE2,
    //2: 01 00 69 00 04 00,
    0x1AB64E62,
    //3: 01 00 69 00 04 00,
    0x1AB64EE2,
    //4: 01 00 13 00 04 00,
    0x1AB64F62,
    //5: 01 00 13 00 04 00,
    0x1AB64FE2
};

int enemy_room_195[] = {
    //num enemy types: 2, num enemies 6,
    //0x1B1EDE84, 0x1B1EDE88,
    0x02,
    //room spawners:,
    0x1B1EDEB2,
    //values: 01 00 57 00 0F 00  //0x57 - Red Ogre,
    //0x1B1EDEBC, //+0x0A address,
    //room spawners:,
    0x1B1EDEC6,
    //values: 01 00 69 00 02 00  //0x69 - Death Ripper,
    //0x1B1EDED0, //+0x0A address,
    //1: 01 00 57 00 04 00,
    0x1B1EDEE2,
    //2: 01 00 57 00 04 00,
    0x1B1EDF62,
    //3: 01 00 69 00 04 00,
    0x1B1EDFE2,
    //4: 01 00 69 00 04 00,
    0x1B1EE062,
    //5: 01 00 57 00 04 00,
    0x1B1EE0E2,
    //6: 01 00 69 00 04 00,
    0x1B1EE162
};

int enemy_room_196[] = {
    //num enemy types: 2, num enemies 12,
    //0x1B38EF84, 0x1B38EF88,
    0x02,
    //room spawners:,
    0x1B38EFB2,
    //values: 01 00 57 00 0F 00  //0x57 - Red Ogre,
    //0x1B38EFBC, //+0x0A address,
    //room spawners:,
    0x1B38EFC6,
    //values: 01 00 2E 00 0F 00  //0x2E - Dullahan,
    //0x1B38EFD0, //+0x0A address,
    //1: 01 00 57 00 05 00,
    0x1B38EFE2,
    //2: 01 00 57 00 05 00,
    0x1B38F062,
    //3: 01 00 57 00 05 00,
    0x1B38F0E2,
    //4: 01 00 57 00 05 00,
    0x1B38F162,
    //5: 01 00 2E 00 05 00,
    0x1B38F1E2,
    //6: 01 00 2E 00 04 00,
    0x1B38F262,
    //7: 01 00 2E 00 05 00,
    0x1B38F2E2,
    //8: 01 00 2E 00 04 00,
    0x1B38F362,
    //9: 01 00 2E 00 05 00,
    0x1B38F3E2,
    //10: 01 00 2E 00 04 00,
    0x1B38F462,
    //11: 01 00 2E 00 05 00,
    0x1B38F4E2,
    //12: 01 00 2E 00 04 00,
    0x1B38F562
};

int enemy_room_197[] = {
    //num enemy types: 2, num enemies 5,
    //0x1B52F184, 0x1B52F188,
    0x02,
    //room spawners:,
    0x1B52F1B2,
    //values: 01 00 34 00 0F 00  //0x34 - Gaap,
    //0x1B52F1BC, //+0x0A address,
    //room spawners:,
    0x1B52F1C6,
    //values: 01 00 71 00 0F 00  //0x71 - Lizard Knight,
    //0x1B52F1D0, //+0x0A address,
    //1: 01 00 34 00 04 00,
    0x1B52F1E2,
    //2: 01 00 71 00 04 00,
    0x1B52F262,
    //3: 01 00 71 00 04 00,
    0x1B52F2E2,
    //4: 01 00 71 00 04 00,
    0x1B52F362,
    //5: 01 00 71 00 04 00,
    0x1B52F3E2
};

int enemy_room_198[] = {
    //num enemy types: 1, num enemies 1,
    //0x1B69D604, 0x1B69D608,
    0x01,
    //room spawners:,
    0x1B69D632,
    //values: 01 00 16 00 00 00  //0x16 - Gargoyle,
    //0x1B69D63C, //+0x0A address,
    //1: 01 00 16 00 04 00,
    0x1B69D652
};

int enemy_room_199[] = {
    //num enemy types: 2, num enemies 5,
    //0x1B815084, 0x1B815088,
    0x02,
    //room spawners:,
    0x1B8150B2,
    //values: 01 00 6A 00 0F 00  //0x6A - Flame Demon,
    //0x1B8150BC, //+0x0A address,
    //room spawners:,
    0x1B8150C6,
    //values: 01 00 20 00 0F 00  //0x20 - Axe Knight,
    //0x1B8150D0, //+0x0A address,
    //1: 01 00 6A 00 04 00,
    0x1B8150E2,
    //2: 01 00 20 00 04 00,
    0x1B815162,
    //3: 01 00 20 00 04 00,
    0x1B8151E2,
    //4: 01 00 20 00 04 00,
    0x1B815262,
    //5: 01 00 20 00 04 00,
    0x1B8152E2
};

int enemy_room_200[] = {
    //num enemy types: 3, num enemies 8,
    //0x1BB3EB04, 0x1BB3EB08,
    0x03,
    //room spawners:,
    0x1BB3EB32,
    //values: 01 00 67 00 0F 00  //0x67 - Spartacus,
    //0x1BB3EB3C, //+0x0A address,
    //room spawners:,
    0x1BB3EB46,
    //values: 01 00 13 00 0F 00  //0x13 - Chaos Sword,
    //0x1BB3EB50, //+0x0A address,
    //room spawners:,
    0x1BB3EB5A,
    //values: 01 00 4B 00 00 00  //0x4B - ???,
    //0x1BB3EB64, //+0x0A address,
    //1: 01 00 67 00 04 00,
    0x1BB3EB72,
    //2: 01 00 67 00 04 00,
    0x1BB3EBF2,
    //3: 01 00 67 00 04 00,
    0x1BB3EC72,
    //4: 01 00 13 00 04 00,
    0x1BB3ECF2,
    //5: 01 00 13 00 04 00,
    0x1BB3ED72,
    //6: 01 00 13 00 04 00,
    0x1BB3EDF2,
    //7: 01 00 13 00 04 00,
    0x1BB3EE72,
    //8: 01 00 4B 00 06 00,
    0x1BB3EEF2
};

int enemy_room_201[] = {
    //num enemy types: 1, num enemies 1,
    //0x1BCAC184, 0x1BCAC188,
    0x01,
    //room spawners:,
    0x1BCAC1B2,
    //values: 01 00 16 00 00 00  //0x16 - Gargoyle,
    //0x1BCAC1BC, //+0x0A address,
    //1: 01 00 16 00 04 00,
    0x1BCAC1D2
};

int enemy_room_202[] = {
    //num enemy types: 2, num enemies 6,
    //0x1BE24204, 0x1BE24208,
    0x02,
    //room spawners:,
    0x1BE24232,
    //values: 01 00 34 00 0F 00  //0x34 - Gaap,
    //0x1BE2423C, //+0x0A address,
    //room spawners:,
    0x1BE24246,
    //values: 01 00 69 00 0F 00  //0x69 - Death Ripper,
    //0x1BE24250, //+0x0A address,
    //1: 01 00 34 00 04 00,
    0x1BE24262,
    //2: 01 00 34 00 04 00,
    0x1BE242E2,
    //3: 01 00 69 00 04 00,
    0x1BE24362,
    //4: 01 00 69 00 04 00,
    0x1BE243E2,
    //5: 01 00 69 00 04 00,
    0x1BE24462,
    //6: 01 00 69 00 04 00,
    0x1BE244E2
};

int enemy_room_203[] = {
    //num enemy types: 2, num enemies 7,
    //0x1C15B384, 0x1C15B388,
    0x02,
    //room spawners:,
    0x1C15B3B2,
    //values: 01 00 57 00 0F 00  //0x57 - Red Ogre,
    //0x1C15B3BC, //+0x0A address,
    //room spawners:,
    0x1C15B3C6,
    //values: 01 00 67 00 0F 00  //0x67 - Spartacus,
    //0x1C15B3D0, //+0x0A address,
    //1: 01 00 57 00 04 00,
    0x1C15B3E2,
    //2: 01 00 57 00 04 00,
    0x1C15B462,
    //3: 01 00 57 00 04 00,
    0x1C15B4E2,
    //4: 01 00 67 00 04 00,
    0x1C15B562,
    //5: 01 00 67 00 04 00,
    0x1C15B5E2,
    //6: 01 00 57 00 04 00,
    0x1C15B662,
    //7: 01 00 57 00 04 00,
    0x1C15B6E2
};

int enemy_room_204[] = {
    //num enemy types: 4, num enemies 11,
    //0x1C6B9404, 0x1C6B9408,
    0x04,
    //room spawners:,
    0x1C6B9432,
    //values: 01 00 59 00 0F 00  //0x59 - Doppleganger,
    //0x1C6B943C, //+0x0A address,
    //room spawners:,
    0x1C6B9446,
    //values: 01 00 54 00 0B 00  //0x54 - ???,
    //0x1C6B9450, //+0x0A address,
    //room spawners:,
    0x1C6B945A,
    //values: 01 00 73 00 0F 00  //0x73 - ???,
    //0x1C6B9464, //+0x0A address,
    //room spawners:,
    0x1C6B946E,
    //values: 01 00 72 00 0F 00  //0x72 - ???,
    //0x1C6B9478, //+0x0A address,
    //1: 01 00 59 00 06 00,
    0x1C6B9482,
    //2: 01 00 54 00 06 00,
    0x1C6B9502,
    //3: 01 00 72 00 06 00,
    0x1C6B9582,
    //4: 01 00 73 00 06 00,
    0x1C6B9602,
    //5: 01 00 72 00 06 00,
    0x1C6B9682,
    //6: 01 00 72 00 06 00,
    0x1C6B9702,
    //7: 01 00 59 00 06 00,
    0x1C6B9782,
    //8: 01 00 72 00 06 00,
    0x1C6B9802,
    //9: 01 00 72 00 06 00,
    0x1C6B9882,
    //10: 01 00 72 00 06 00,
    0x1C6B9902,
    //11: 01 00 72 00 06 00,
    0x1C6B9982
};

int enemy_room_205[] = {
    //num enemy types: 1, num enemies 2,
    //0x1C875E84, 0x1C875E88,
    0x01,
    //room spawners:,
    0x1C875EB2,
    //values: 01 00 2F 00 0F 00  //0x2F - Phantom,
    //0x1C875EBC, //+0x0A address,
    //1: 01 00 2F 00 04 00,
    0x1C875ED2,
    //2: 01 00 2F 00 04 00,
    0x1C875F52
};

int enemy_room_206[] = {
    //num enemy types: 2, num enemies 7,
    //0x1C9FC704, 0x1C9FC708,
    0x02,
    //room spawners:,
    0x1C9FC732,
    //values: 01 00 5D 00 0F 00  //0x5D - Mirage Skeleton,
    //0x1C9FC73C, //+0x0A address,
    //room spawners:,
    0x1C9FC746,
    //values: 01 00 4B 00 00 00  //0x4B - ???,
    //0x1C9FC750, //+0x0A address,
    //1: 01 00 5D 00 04 00,
    0x1C9FC762,
    //2: 01 00 5D 00 04 00,
    0x1C9FC7E2,
    //3: 01 00 5D 00 04 00,
    0x1C9FC862,
    //4: 01 00 5D 00 04 00,
    0x1C9FC8E2,
    //5: 01 00 5D 00 04 00,
    0x1C9FC962,
    //6: 01 00 5D 00 04 00,
    0x1C9FC9E2,
    //7: 01 00 4B 00 06 00,
    0x1C9FCA62
};

int enemy_room_207[] = {
    //num enemy types: 2, num enemies 5,
    //0x1CB9AB84, 0x1CB9AB88,
    0x02,
    //room spawners:,
    0x1CB9ABB2,
    //values: 01 00 2F 00 0F 00  //0x2F - Phantom,
    //0x1CB9ABBC, //+0x0A address,
    //room spawners:,
    0x1CB9ABC6,
    //values: 01 00 69 00 0F 00  //0x69 - Death Ripper,
    //0x1CB9ABD0, //+0x0A address,
    //1: 01 00 2F 00 04 00,
    0x1CB9ABE2,
    //2: 01 00 69 00 04 00,
    0x1CB9AC62,
    //3: 01 00 69 00 04 00,
    0x1CB9ACE2,
    //4: 01 00 69 00 04 00,
    0x1CB9AD62,
    //5: 01 00 69 00 04 00,
    0x1CB9ADE2
};

int enemy_room_208[] = {
    //num enemy types: 2, num enemies 5,
    //0x1CD36984, 0x1CD36988,
    0x02,
    //room spawners:,
    0x1CD369B2,
    //values: 01 00 2F 00 0F 00  //0x2F - Phantom,
    //0x1CD369BC, //+0x0A address,
    //room spawners:,
    0x1CD369C6,
    //values: 01 00 71 00 0F 00  //0x71 - Lizard Knight,
    //0x1CD369D0, //+0x0A address,
    //1: 01 00 2F 00 04 00,
    0x1CD369E2,
    //2: 01 00 71 00 04 00,
    0x1CD36A62,
    //3: 01 00 71 00 04 00,
    0x1CD36AE2,
    //4: 01 00 71 00 04 00,
    0x1CD36B62,
    //5: 01 00 71 00 04 00,
    0x1CD36BE2
};

int enemy_room_209[] = {
    //num enemy types: 1, num enemies 6,
    //0x1CEEAC84, 0x1CEEAC88,
    0x01,
    //room spawners:,
    0x1CEEACB2,
    //values: 01 00 69 00 0F 00  //0x69 - Death Ripper,
    //0x1CEEACBC, //+0x0A address,
    //1: 01 00 69 00 04 00,
    0x1CEEACD2,
    //2: 01 00 69 00 04 00,
    0x1CEEAD52,
    //3: 01 00 69 00 04 00,
    0x1CEEADD2,
    //4: 01 00 69 00 04 00,
    0x1CEEAE52,
    //5: 01 00 69 00 14 00,
    0x1CEEAED2,
    //6: 01 00 69 00 14 00,
    0x1CEEAF52
};

int enemy_room_210[] = {
    //num enemy types: 2, num enemies 2,
    //0x1D08E484, 0x1D08E488,
    0x02,
    //room spawners:,
    0x1D08E4B2,
    //values: 01 00 34 00 0F 00  //0x34 - Gaap,
    //0x1D08E4BC, //+0x0A address,
    //room spawners:,
    0x1D08E4C6,
    //values: 01 00 16 00 0F 00  //0x16 - Gargoyle,
    //0x1D08E4D0, //+0x0A address,
    //1: 01 00 34 00 04 00,
    0x1D08E4E2,
    //2: 01 00 16 00 04 00,
    0x1D08E562
};

int enemy_room_211[] = {
    //num enemy types: 2, num enemies 3,
    //0x1D231B84, 0x1D231B88,
    0x02,
    //room spawners:,
    0x1D231BB2,
    //values: 01 00 58 00 0F 00  //0x58 - Executioner,
    //0x1D231BBC, //+0x0A address,
    //room spawners:,
    0x1D231BC6,
    //values: 01 00 57 00 02 00  //0x57 - Red Ogre,
    //0x1D231BD0, //+0x0A address,
    //1: 01 00 58 00 04 00,
    0x1D231BE2,
    //2: 01 00 58 00 04 00,
    0x1D231C62,
    //3: 01 00 57 00 04 00,
    0x1D231CE2
};

int enemy_room_212[] = {
    //num enemy types: 1, num enemies 4,
    //0x1D3D5504, 0x1D3D5508,
    0x01,
    //room spawners:,
    0x1D3D5532,
    //values: 01 00 71 00 0F 00  //0x71 - Lizard Knight,
    //0x1D3D553C, //+0x0A address,
    //1: 01 00 71 00 04 00,
    0x1D3D5552,
    //2: 01 00 71 00 04 00,
    0x1D3D55D2,
    //3: 01 00 71 00 04 00,
    0x1D3D5652,
    //4: 01 00 71 00 14 00,
    0x1D3D56D2
};

int enemy_room_213[] = {
    //num enemy types: 2, num enemies 4,
    //0x1D578B84, 0x1D578B88,
    0x02,
    //room spawners:,
    0x1D578BB2,
    //values: 01 00 71 00 02 00  //0x71 - Lizard Knight,
    //0x1D578BBC, //+0x0A address,
    //room spawners:,
    0x1D578BC6,
    //values: 01 00 65 00 0F 00  //0x65 - Hanged Man,
    //0x1D578BD0, //+0x0A address,
    //1: 01 00 65 00 14 00,
    0x1D578BE2,
    //2: 01 00 65 00 14 00,
    0x1D578C62,
    //3: 01 00 71 00 04 00,
    0x1D578CE2,
    //4: 01 00 71 00 04 00,
    0x1D578D62
};

int enemy_room_214[] = {
    //num enemy types: 1, num enemies 4,
    //0x1D71C104, 0x1D71C108,
    0x01,
    //room spawners:,
    0x1D71C132,
    //values: 01 00 67 00 0F 00  //0x67 - Spartacus,
    //0x1D71C13C, //+0x0A address,
    //1: 01 00 67 00 04 00,
    0x1D71C152,
    //2: 01 00 67 00 04 00,
    0x1D71C1D2,
    //3: 01 00 67 00 04 00,
    0x1D71C252,
    //4: 01 00 67 00 14 00,
    0x1D71C2D2
};

int enemy_room_215[] = {
    //num enemy types: 2, num enemies 4,
    //0x1D8BF804, 0x1D8BF808,
    0x02,
    //room spawners:,
    0x1D8BF832,
    //values: 01 00 2E 00 0F 00  //0x2E - Dullahan,
    //0x1D8BF83C, //+0x0A address,
    //room spawners:,
    0x1D8BF846,
    //values: 01 00 20 00 02 00  //0x20 - Axe Knight,
    //0x1D8BF850, //+0x0A address,
    //1: 01 00 2E 00 04 00,
    0x1D8BF862,
    //2: 01 00 2E 00 04 00,
    0x1D8BF8E2,
    //3: 01 00 2E 00 04 00,
    0x1D8BF962,
    //4: 01 00 20 00 14 00,
    0x1D8BF9E2
};

int enemy_room_216[] = {
    //num enemy types: 2, num enemies 3,
    //0x1DA62A84, 0x1DA62A88,
    0x02,
    //room spawners:,
    0x1DA62AB2,
    //values: 01 00 65 00 02 00  //0x65 - Hanged Man,
    //0x1DA62ABC, //+0x0A address,
    //room spawners:,
    0x1DA62AC6,
    //values: 01 00 6C 00 0F 00  //0x6C - Lesser Demon,
    //0x1DA62AD0, //+0x0A address,
    //1: 01 00 6C 00 04 00,
    0x1DA62AE2,
    //2: 01 00 6C 00 04 00,
    0x1DA62B62,
    //3: 01 00 65 00 14 00,
    0x1DA62BE2
};

int enemy_room_217[] = {
    //num enemy types: 1, num enemies 1,
    //0x1DE6CB04, 0x1DE6CB08,
    0x01,
    //room spawners:,
    0x1DE6CB32,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x1DE6CB3C, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x1DE6CB52
};

int enemy_room_218[] = {
    //num enemy types: 1, num enemies 2,
    //0x1E1DDA04, 0x1E1DDA08,
    0x01,
    //room spawners:,
    0x1E1DDA32,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x1E1DDA3C, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x1E1DDA52,
    //2: 01 00 54 00 04 00,
    0x1E1DDAD2
};

int enemy_room_219[] = {
    //num enemy types: 1, num enemies 2,
    //0x1E371104, 0x1E371108,
    0x01,
    //room spawners:,
    0x1E371132,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x1E37113C, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x1E371152,
    //2: 01 00 54 00 04 00,
    0x1E3711D2
};

int enemy_room_220[] = {
    //num enemy types: 2, num enemies 4,
    //0x1E66EF04, 0x1E66EF08,
    0x02,
    //room spawners:,
    0x1E66EF32,
    //values: 01 00 68 00 00 00  //0x68 - ???,
    //0x1E66EF3C, //+0x0A address,
    //room spawners:,
    0x1E66EF46,
    //values: 01 00 51 00 00 00  //0x51 - Death,
    //0x1E66EF50, //+0x0A address,
    //1: 01 00 51 00 06 00,
    0x1E66EF62,
    //2: 01 00 68 00 06 00,
    0x1E66EFE2,
    //3: 01 00 68 00 06 00,
    0x1E66F062,
    //4: 01 00 68 00 06 00,
    0x1E66F0E2
};

int enemy_room_221[] = {
    //num enemy types: 1, num enemies 1,
    //0x1EAE6B04, 0x1EAE6B08,
    0x01,
    //room spawners:,
    0x1EAE6B32,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x1EAE6B3C, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x1EAE6B52
};

int enemy_room_222[] = {
    //num enemy types: 1, num enemies 2,
    //0x1EC79B84, 0x1EC79B88,
    0x01,
    //room spawners:,
    0x1EC79BB2,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x1EC79BBC, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x1EC79BD2,
    //2: 01 00 54 00 04 00,
    0x1EC79C52
};

int enemy_room_223[] = {
    //num enemy types: 1, num enemies 1,
    //0x1EDC0B84, 0x1EDC0B88,
    0x01,
    //room spawners:,
    0x1EDC0BB2,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x1EDC0BBC, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x1EDC0BD2
};

int enemy_room_224[] = {
    //num enemy types: 1, num enemies 2,
    //0x1EF44104, 0x1EF44108,
    0x01,
    //room spawners:,
    0x1EF44132,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x1EF4413C, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x1EF44152,
    //2: 01 00 54 00 04 00,
    0x1EF441D2
};

int enemy_room_225[] = {
    //num enemy types: 2, num enemies 5,
    //0x1F173184, 0x1F173188,
    0x02,
    //room spawners:,
    0x1F1731B2,
    //values: 01 00 68 00 00 00  //0x68 - ???,
    //0x1F1731BC, //+0x0A address,
    //room spawners:,
    0x1F1731C6,
    //values: 01 00 52 00 00 00  //0x52 - Walter,
    //0x1F1731D0, //+0x0A address,
    //1: 01 00 68 00 06 00,
    0x1F1731E2,
    //2: 01 00 52 00 06 00,
    0x1F173262,
    //3: 01 00 68 00 06 00,
    0x1F1732E2,
    //4: 01 00 52 00 06 00,
    0x1F173362,
    //5: 01 00 68 00 06 00,
    0x1F1733E2
};

int enemy_room_226[] = {
    //num enemy types: 1, num enemies 1,
    //0x1F5D0B84, 0x1F5D0B88,
    0x01,
    //room spawners:,
    0x1F5D0BB2,
    //values: 01 00 45 00 0F 00  //0x45 - ???,
    //0x1F5D0BBC, //+0x0A address,
    //1: 01 00 45 00 04 00,
    0x1F5D0BD2
};

int enemy_room_227[] = {
    //num enemy types: 1, num enemies 1,
    //0x1F776C04, 0x1F776C08,
    0x01,
    //room spawners:,
    0x1F776C32,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x1F776C3C, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x1F776C52
};

int enemy_room_228[] = {
    //num enemy types: 5, num enemies 11,
    //0x1F8E4604, 0x1F8E4608,
    0x05,
    //room spawners:,
    0x1F8E4632,
    //values: 01 00 2D 00 0F 00  //0x2D - Forgotten One,
    //0x1F8E463C, //+0x0A address,
    //room spawners:,
    0x1F8E4646,
    //values: 01 00 73 00 0F 00  //0x73 - ???,
    //0x1F8E4650, //+0x0A address,
    //room spawners:,
    0x1F8E465A,
    //values: 01 00 72 00 0F 00  //0x72 - ???,
    //0x1F8E4664, //+0x0A address,
    //room spawners:,
    0x1F8E466E,
    //values: 01 00 74 00 0F 00  //0x74 - ???,
    //0x1F8E4678, //+0x0A address,
    //room spawners:,
    0x1F8E4682,
    //values: 01 00 2C 00 0F 00  //0x2C - Maggot,
    //0x1F8E468C, //+0x0A address,
    //1: 01 00 2D 00 06 00,
    0x1F8E46A2,
    //2: 01 00 2C 00 04 00,
    0x1F8E4722,
    //3: 01 00 74 00 06 00,
    0x1F8E47A2,
    //4: 01 00 72 00 06 00,
    0x1F8E4822,
    //5: 01 00 73 00 06 00,
    0x1F8E48A2,
    //6: 01 00 72 00 06 00,
    0x1F8E4922,
    //7: 01 00 72 00 06 00,
    0x1F8E49A2,
    //8: 01 00 72 00 06 00,
    0x1F8E4A22,
    //9: 01 00 72 00 06 00,
    0x1F8E4AA2,
    //10: 01 00 72 00 06 00,
    0x1F8E4B22,
    //11: 01 00 72 00 06 00,
    0x1F8E4BA2
};

int enemy_room_229[] = {
    //num enemy types: 1, num enemies 1,
    //0x1FAE0C84, 0x1FAE0C88,
    0x01,
    //room spawners:,
    0x1FAE0CB2,
    //values: 01 00 3F 00 00 00  //0x3F - ???,
    //0x1FAE0CBC, //+0x0A address,
    //1: 01 00 3F 00 04 00,
    0x1FAE0CD2
};

int enemy_room_230[] = {
    //num enemy types: 3, num enemies 4,
    //0x2002CE04, 0x2002CE08,
    0x03,
    //room spawners:,
    0x2002CE32,
    //values: 01 00 73 00 0F 00  //0x73 - ???,
    //0x2002CE3C, //+0x0A address,
    //room spawners:,
    0x2002CE46,
    //values: 01 00 72 00 0F 00  //0x72 - ???,
    //0x2002CE50, //+0x0A address,
    //room spawners:,
    0x2002CE5A,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x2002CE64, //+0x0A address,
    //1: 01 00 73 00 24 00,
    0x2002CE72,
    //2: 01 00 72 00 24 00,
    0x2002CEF2,
    //3: 01 00 54 00 04 00,
    0x2002CF72,
    //4: 01 00 54 00 04 00,
    0x2002CFF2
};

int enemy_room_231[] = {
    //num enemy types: 1, num enemies 2,
    //0x20259A04, 0x20259A08,
    0x01,
    //room spawners:,
    0x20259A32,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x20259A3C, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x20259A52,
    //2: 01 00 54 00 04 00,
    0x20259AD2
};

int enemy_room_232[] = {
    //num enemy types: 1, num enemies 2,
    //0x2044CA84, 0x2044CA88,
    0x01,
    //room spawners:,
    0x2044CAB2,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x2044CABC, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x2044CAD2,
    //2: 01 00 54 00 04 00,
    0x2044CB52
};

int enemy_room_233[] = {
    //num enemy types: 3, num enemies 8,
    //0x205EA404, 0x205EA408,
    0x03,
    //room spawners:,
    0x205EA432,
    //values: 01 00 0C 00 0F 00  //0x0C - Skeleton,
    //0x205EA43C, //+0x0A address,
    //room spawners:,
    0x205EA446,
    //values: 01 00 2E 00 02 00  //0x2E - Dullahan,
    //0x205EA450, //+0x0A address,
    //room spawners:,
    0x205EA45A,
    //values: 01 00 11 00 02 00  //0x11 - Skeleton Swordsman,
    //0x205EA464, //+0x0A address,
    //1: 01 00 0C 00 06 00,
    0x205EA472,
    //2: 01 00 0C 00 06 00,
    0x205EA4F2,
    //3: 01 00 0C 00 06 00,
    0x205EA572,
    //4: 01 00 0C 00 06 00,
    0x205EA5F2,
    //5: 01 00 11 00 06 00,
    0x205EA672,
    //6: 01 00 11 00 06 00,
    0x205EA6F2,
    //7: 01 00 11 00 06 00,
    0x205EA772,
    //8: 01 00 2E 00 06 00,
    0x205EA7F2
};

int enemy_room_234[] = {
    //num enemy types: 1, num enemies 2,
    //0x20804504, 0x20804508,
    0x01,
    //room spawners:,
    0x20804532,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x2080453C, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x20804552,
    //2: 01 00 54 00 04 00,
    0x208045D2
};

int enemy_room_235[] = {
    //num enemy types: 3, num enemies 4,
    //0x209E5684, 0x209E5688,
    0x03,
    //room spawners:,
    0x209E56B2,
    //values: 01 00 4F 00 0F 00  //0x4F - Golden Knight,
    //0x209E56BC, //+0x0A address,
    //room spawners:,
    0x209E56C6,
    //values: 01 00 53 00 02 00  //0x53 - Cyclops,
    //0x209E56D0, //+0x0A address,
    //room spawners:,
    0x209E56DA,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x209E56E4, //+0x0A address,
    //1: 01 00 4F 00 04 00,
    0x209E56F2,
    //2: 01 00 53 00 04 00,
    0x209E5772,
    //3: 01 00 53 00 04 00,
    0x209E57F2,
    //4: 01 00 54 00 04 00,
    0x209E5872
};

int enemy_room_236[] = {
    //num enemy types: 1, num enemies 1,
    //0x20CAE284, 0x20CAE288,
    0x01,
    //room spawners:,
    0x20CAE2B2,
    //values: 01 00 68 00 00 00  //0x68 - ???,
    //0x20CAE2BC, //+0x0A address,
    //1: 01 00 68 00 05 00,
    0x20CAE2D2
};

int enemy_room_237[] = {
    //num enemy types: 1, num enemies 1,
    //0x20E95A84, 0x20E95A88,
    0x01,
    //room spawners:,
    0x20E95AB2,
    //values: 01 00 75 00 00 00  //0x75 - ???,
    //0x20E95ABC, //+0x0A address,
    //1: 01 00 75 00 04 00,
    0x20E95AD2
};

int enemy_room_238[] = {
    //num enemy types: 2, num enemies 3,
    //0x21B43E04, 0x21B43E08,
    0x02,
    //room spawners:,
    0x21B43E32,
    //values: 01 00 0C 00 0F 00  //0x0C - Skeleton,
    //0x21B43E3C, //+0x0A address,
    //room spawners:,
    0x21B43E46,
    //values: 01 00 3E 00 0F 00  //0x3E - ???,
    //0x21B43E50, //+0x0A address,
    //1: 01 00 0C 00 04 00,
    0x21B43E62,
    //2: 01 00 0C 00 04 00,
    0x21B43EE2,
    //3: 01 00 3E 00 04 00,
    0x21B43F62
};

int enemy_room_239[] = {
    //num enemy types: 4, num enemies 7,
    //0x21D03904, 0x21D03908,
    0x04,
    //room spawners:,
    0x21D03932,
    //values: 01 00 0C 00 0F 00  //0x0C - Skeleton,
    //0x21D0393C, //+0x0A address,
    //room spawners:,
    0x21D03946,
    //values: 01 00 30 00 0F 00  //0x30 - Skeleton Hunter,
    //0x21D03950, //+0x0A address,
    //room spawners:,
    0x21D0395A,
    //values: 01 00 4B 00 00 00  //0x4B - ???,
    //0x21D03964, //+0x0A address,
    //room spawners:,
    0x21D0396E,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x21D03978, //+0x0A address,
    //1: 01 00 0C 00 04 00,
    0x21D03982,
    //2: 01 00 0C 00 04 00,
    0x21D03A02,
    //3: 01 00 0C 00 04 00,
    0x21D03A82,
    //4: 01 00 0C 00 04 00,
    0x21D03B02,
    //5: 01 00 30 00 04 00,
    0x21D03B82,
    //6: 01 00 4B 00 06 00,
    0x21D03C02,
    //7: 01 00 54 00 04 00,
    0x21D03C82
};

int enemy_room_240[] = {
    //num enemy types: 4, num enemies 7,
    //0x21EE6984, 0x21EE6988,
    0x04,
    //room spawners:,
    0x21EE69B2,
    //values: 01 00 0C 00 00 00  //0x0C - Skeleton,
    //0x21EE69BC, //+0x0A address,
    //room spawners:,
    0x21EE69C6,
    //values: 01 00 17 00 00 00  //0x17 - Skeleton Archer,
    //0x21EE69D0, //+0x0A address,
    //room spawners:,
    0x21EE69DA,
    //values: 01 00 4B 00 00 00  //0x4B - ???,
    //0x21EE69E4, //+0x0A address,
    //room spawners:,
    0x21EE69EE,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x21EE69F8, //+0x0A address,
    //1: 01 00 0C 00 04 00,
    0x21EE6A02,
    //2: 01 00 0C 00 04 00,
    0x21EE6A82,
    //3: 01 00 17 00 04 00,
    0x21EE6B02,
    //4: 01 00 0C 00 04 00,
    0x21EE6B82,
    //5: 01 00 0C 00 04 00,
    0x21EE6C02,
    //6: 01 00 4B 00 06 00,
    0x21EE6C82,
    //7: 01 00 54 00 04 00,
    0x21EE6D02
};

int enemy_room_241[] = {
    //num enemy types: 2, num enemies 2,
    //0x2209E704, 0x2209E708,
    0x02,
    //room spawners:,
    0x2209E732,
    //values: 01 00 62 00 02 00  //0x62 - Flea Man,
    //0x2209E73C, //+0x0A address,
    //room spawners:,
    0x2209E746,
    //values: 01 00 24 00 02 00  //0x24 - Astral Knight,
    //0x2209E750, //+0x0A address,
    //1: 01 00 62 00 14 00,
    0x2209E762,
    //2: 01 00 24 00 14 00,
    0x2209E7E2
};

int enemy_room_242[] = {
    //num enemy types: 3, num enemies 5,
    //0x22269B04, 0x22269B08,
    0x03,
    //room spawners:,
    0x22269B32,
    //values: 01 00 27 00 02 00  //0x27 - Ghost Soldier,
    //0x22269B3C, //+0x0A address,
    //room spawners:,
    0x22269B46,
    //values: 01 00 30 00 02 00  //0x30 - Skeleton Hunter,
    //0x22269B50, //+0x0A address,
    //room spawners:,
    0x22269B5A,
    //values: 01 00 4B 00 00 00  //0x4B - ???,
    //0x22269B64, //+0x0A address,
    //1: 01 00 30 00 04 00,
    0x22269B72,
    //2: 01 00 30 00 04 00,
    0x22269BF2,
    //3: 01 00 30 00 04 00,
    0x22269C72,
    //4: 01 00 27 00 14 00,
    0x22269CF2,
    //5: 01 00 4B 00 06 00,
    0x22269D72
};

int enemy_room_243[] = {
    //num enemy types: 1, num enemies 1,
    //0x2245EE84, 0x2245EE88,
    0x01,
    //room spawners:,
    0x2245EEB2,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x2245EEBC, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x2245EED2
};

int enemy_room_244[] = {
    //num enemy types: 4, num enemies 4,
    //0x22646804, 0x22646808,
    0x04,
    //room spawners:,
    0x22646832,
    //values: 01 00 27 00 02 00  //0x27 - Ghost Soldier,
    //0x2264683C, //+0x0A address,
    //room spawners:,
    0x22646846,
    //values: 01 00 1A 00 02 00  //0x1A - Heavy Armor,
    //0x22646850, //+0x0A address,
    //room spawners:,
    0x2264685A,
    //values: 01 00 4B 00 00 00  //0x4B - ???,
    //0x22646864, //+0x0A address,
    //room spawners:,
    0x2264686E,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x22646878, //+0x0A address,
    //1: 01 00 27 00 14 00,
    0x22646882,
    //2: 01 00 1A 00 04 00,
    0x22646902,
    //3: 01 00 4B 00 06 00,
    0x22646982,
    //4: 01 00 54 00 04 00,
    0x22646A02
};

int enemy_room_245[] = {
    //num enemy types: 1, num enemies 3,
    //0x229AD204, 0x229AD208,
    0x01,
    //room spawners:,
    0x229AD232,
    //values: 01 00 4C 00 00 00  //0x4C - Shadow Wolf,
    //0x229AD23C, //+0x0A address,
    //1: 01 00 4C 00 04 00,
    0x229AD252,
    //2: 01 00 4C 00 04 00,
    0x229AD2D2,
    //3: 01 00 4C 00 04 00,
    0x229AD352
};

int enemy_room_246[] = {
    //num enemy types: 3, num enemies 3,
    //0x22B66D04, 0x22B66D08,
    0x03,
    //room spawners:,
    0x22B66D32,
    //values: 01 00 07 00 00 00  //0x07 - Zombie,
    //0x22B66D3C, //+0x0A address,
    //room spawners:,
    0x22B66D46,
    //values: 01 00 01 00 00 00  //0x01 - Spirit,
    //0x22B66D50, //+0x0A address,
    //room spawners:,
    0x22B66D5A,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x22B66D64, //+0x0A address,
    //1: 01 00 07 00 14 00,
    0x22B66D72,
    //2: 01 00 01 00 14 00,
    0x22B66DF2,
    //3: 01 00 54 00 04 00,
    0x22B66E72
};

int enemy_room_247[] = {
    //num enemy types: 1, num enemies 2,
    //0x22D0CF04, 0x22D0CF08,
    0x01,
    //room spawners:,
    0x22D0CF32,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x22D0CF3C, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x22D0CF52,
    //2: 01 00 54 00 04 00,
    0x22D0CFD2
};

int enemy_room_248[] = {
    //num enemy types: 1, num enemies 2,
    //0x22EB3184, 0x22EB3188,
    0x01,
    //room spawners:,
    0x22EB31B2,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x22EB31BC, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x22EB31D2,
    //2: 01 00 54 00 04 00,
    0x22EB3252
};

int enemy_room_249[] = {
    //num enemy types: 1, num enemies 1,
    //0x2305A784, 0x2305A788,
    0x01,
    //room spawners:,
    0x2305A7B2,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x2305A7BC, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x2305A7D2
};

int enemy_room_250[] = {
    //num enemy types: 1, num enemies 2,
    //0x231FF404, 0x231FF408,
    0x01,
    //room spawners:,
    0x231FF432,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x231FF43C, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x231FF452,
    //2: 01 00 54 00 04 00,
    0x231FF4D2
};

int enemy_room_251[] = {
    //num enemy types: 1, num enemies 2,
    //0x233A5C84, 0x233A5C88,
    0x01,
    //room spawners:,
    0x233A5CB2,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x233A5CBC, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x233A5CD2,
    //2: 01 00 54 00 04 00,
    0x233A5D52
};

int enemy_room_252[] = {
    //num enemy types: 1, num enemies 2,
    //0x23539704, 0x23539708,
    0x01,
    //room spawners:,
    0x23539732,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x2353973C, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x23539752,
    //2: 01 00 54 00 04 00,
    0x235397D2
};

/*
int enemy_room_253[] = {
    //num enemy types: 3, num enemies 8,
    //0x23761B84, 0x23761B88,
    0x03,
    //room spawners:,
    0x23761BB2,
    //values: 01 00 53 00 0F 00  //0x53 - Cyclops,
    //0x23761BBC, //+0x0A address,
    //room spawners:,
    0x23761BC6,
    //values: 01 00 57 00 0F 00  //0x57 - Red Ogre,
    //0x23761BD0, //+0x0A address,
    //room spawners:,
    0x23761BDA,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x23761BE4, //+0x0A address,
    //1: 01 00 53 00 05 00,
    0x23761BF2,
    //2: 01 00 53 00 05 00,
    0x23761C72,
    //3: 01 00 53 00 05 00,
    0x23761CF2,
    //4: 01 00 57 00 05 00,
    0x23761D72,
    //5: 01 00 57 00 05 00,
    0x23761DF2,
    //6: 01 00 57 00 05 00,
    0x23761E72,
    //7: 01 00 54 00 04 00,
    0x23761EF2,
    //8: 01 00 54 00 04 00,
    0x23761F72
};
*/

int enemy_room_254[] = {
    //num enemy types: 2, num enemies 6,
    //0x238EC184, 0x238EC188,
    0x02,
    //room spawners:,
    0x238EC1B2,
    //values: 01 00 58 00 02 00  //0x58 - Executioner,
    //0x238EC1BC, //+0x0A address,
    //room spawners:,
    0x238EC1C6,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x238EC1D0, //+0x0A address,
    //1: 01 00 58 00 05 00,
    0x238EC1E2,
    //2: 01 00 58 00 05 00,
    0x238EC262,
    //3: 01 00 58 00 05 00,
    0x238EC2E2,
    //4: 01 00 58 00 05 00,
    0x238EC362,
    //5: 01 00 58 00 04 00,
    0x238EC3E2,
    //6: 01 00 54 00 04 00,
    0x238EC462
};

int enemy_room_255[] = {
    //num enemy types: 6, num enemies 14,
    //0x23AF5584, 0x23AF5588,
    0x06,
    //room spawners:,
    0x23AF55B2,
    //values: 01 00 3B 00 0F 00  //0x3B - Succubus,
    //0x23AF55BC, //+0x0A address,
    //room spawners:,
    0x23AF55C6,
    //values: 01 00 72 00 0F 00  //0x72 - ???,
    //0x23AF55D0, //+0x0A address,
    //room spawners:,
    0x23AF55DA,
    //values: 01 00 73 00 0F 00  //0x73 - ???,
    //0x23AF55E4, //+0x0A address,
    //room spawners:,
    0x23AF55EE,
    //values: 01 00 74 00 0F 00  //0x74 - ???,
    //0x23AF55F8, //+0x0A address,
    //room spawners:,
    0x23AF5602,
    //values: 01 00 68 00 0F 00  //0x68 - ???,
    //0x23AF560C, //+0x0A address,
    //room spawners:,
    0x23AF5616,
    //values: 01 00 46 00 0F 00  //0x46 - ???,
    //0x23AF5620, //+0x0A address,
    //1: 01 00 3B 00 06 00,
    0x23AF5632,
    //2: 01 00 46 00 06 00,
    0x23AF56B2,
    //3: 01 00 68 00 06 00,
    0x23AF5732,
    //4: 01 00 68 00 26 00,
    0x23AF57B2,
    //5: 01 00 74 00 06 00,
    0x23AF5832,
    //6: 01 00 73 00 06 00,
    0x23AF58B2,
    //7: 01 00 72 00 06 00,
    0x23AF5932,
    //8: 01 00 73 00 06 00,
    0x23AF59B2,
    //9: 01 00 72 00 06 00,
    0x23AF5A32,
    //10: 01 00 72 00 06 00,
    0x23AF5AB2,
    //11: 01 00 72 00 06 00,
    0x23AF5B32,
    //12: 01 00 72 00 06 00,
    0x23AF5BB2,
    //13: 01 00 72 00 06 00,
    0x23AF5C32,
    //14: 01 00 72 00 06 00,
    0x23AF5CB2
};

int enemy_room_256[] = {
    //num enemy types: 3, num enemies 6,
    //0x23CE4184, 0x23CE4188,
    0x03,
    //room spawners:,
    0x23CE41B2,
    //values: 01 00 47 00 0F 00  //0x47 - Ghost,
    //0x23CE41BC, //+0x0A address,
    //room spawners:,
    0x23CE41C6,
    //values: 01 00 0F 00 0F 00  //0x0F - Skeleton Warrior,
    //0x23CE41D0, //+0x0A address,
    //room spawners:,
    0x23CE41DA,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x23CE41E4, //+0x0A address,
    //1: 01 00 47 00 04 00,
    0x23CE41F2,
    //2: 01 00 47 00 04 00,
    0x23CE4272,
    //3: 01 00 0F 00 04 00,
    0x23CE42F2,
    //4: 01 00 0F 00 04 00,
    0x23CE4372,
    //5: 01 00 0F 00 04 00,
    0x23CE43F2,
    //6: 01 00 54 00 04 00,
    0x23CE4472
};

int enemy_room_257[] = {
    //num enemy types: 4, num enemies 8,
    //0x23ED6884, 0x23ED6888,
    0x04,
    //room spawners:,
    0x23ED68B2,
    //values: 01 00 62 00 02 00  //0x62 - Flea Man,
    //0x23ED68BC, //+0x0A address,
    //room spawners:,
    0x23ED68C6,
    //values: 01 00 0F 00 0F 00  //0x0F - Skeleton Warrior,
    //0x23ED68D0, //+0x0A address,
    //room spawners:,
    0x23ED68DA,
    //values: 01 00 47 00 0F 00  //0x47 - Ghost,
    //0x23ED68E4, //+0x0A address,
    //room spawners:,
    0x23ED68EE,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x23ED68F8, //+0x0A address,
    //1: 01 00 47 00 06 00,
    0x23ED6902,
    //2: 01 00 47 00 06 00,
    0x23ED6982,
    //3: 01 00 0F 00 06 00,
    0x23ED6A02,
    //4: 01 00 0F 00 06 00,
    0x23ED6A82,
    //5: 01 00 0F 00 06 00,
    0x23ED6B02,
    //6: 01 00 62 00 16 00,
    0x23ED6B82,
    //7: 01 00 54 00 04 00,
    0x23ED6C02,
    //8: 01 00 54 00 04 00,
    0x23ED6C82
};

int enemy_room_258[] = {
    //num enemy types: 3, num enemies 7,
    //0x240C9A04, 0x240C9A08,
    0x03,
    //room spawners:,
    0x240C9A32,
    //values: 01 00 27 00 00 00  //0x27 - Ghost Soldier,
    //0x240C9A3C, //+0x0A address,
    //room spawners:,
    0x240C9A46,
    //values: 01 00 0F 00 00 00  //0x0F - Skeleton Warrior,
    //0x240C9A50, //+0x0A address,
    //room spawners:,
    0x240C9A5A,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x240C9A64, //+0x0A address,
    //1: 01 00 27 00 04 00,
    0x240C9A72,
    //2: 01 00 27 00 04 00,
    0x240C9AF2,
    //3: 01 00 27 00 04 00,
    0x240C9B72,
    //4: 01 00 27 00 04 00,
    0x240C9BF2,
    //5: 01 00 0F 00 04 00,
    0x240C9C72,
    //6: 01 00 0F 00 04 00,
    0x240C9CF2,
    //7: 01 00 54 00 04 00,
    0x240C9D72
};

int enemy_room_259[] = {
    //num enemy types: 3, num enemies 6,
    //0x242B7584, 0x242B7588,
    0x03,
    //room spawners:,
    0x242B75B2,
    //values: 01 00 27 00 0F 00  //0x27 - Ghost Soldier,
    //0x242B75BC, //+0x0A address,
    //room spawners:,
    0x242B75C6,
    //values: 01 00 0F 00 0F 00  //0x0F - Skeleton Warrior,
    //0x242B75D0, //+0x0A address,
    //room spawners:,
    0x242B75DA,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x242B75E4, //+0x0A address,
    //1: 01 00 27 00 04 00,
    0x242B75F2,
    //2: 01 00 27 00 04 00,
    0x242B7672,
    //3: 01 00 27 00 04 00,
    0x242B76F2,
    //4: 01 00 0F 00 04 00,
    0x242B7772,
    //5: 01 00 0F 00 04 00,
    0x242B77F2,
    //6: 01 00 54 00 04 00,
    0x242B7872
};

int enemy_room_260[] = {
    //num enemy types: 3, num enemies 7,
    //0x244ADC84, 0x244ADC88,
    0x03,
    //room spawners:,
    0x244ADCB2,
    //values: 01 00 47 00 00 00  //0x47 - Ghost,
    //0x244ADCBC, //+0x0A address,
    //room spawners:,
    0x244ADCC6,
    //values: 01 00 01 00 00 00  //0x01 - Spirit,
    //0x244ADCD0, //+0x0A address,
    //room spawners:,
    0x244ADCDA,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x244ADCE4, //+0x0A address,
    //1: 01 00 01 00 04 00,
    0x244ADCF2,
    //2: 01 00 01 00 04 00,
    0x244ADD72,
    //3: 01 00 47 00 04 00,
    0x244ADDF2,
    //4: 01 00 47 00 04 00,
    0x244ADE72,
    //5: 01 00 01 00 04 00,
    0x244ADEF2,
    //6: 01 00 01 00 04 00,
    0x244ADF72,
    //7: 01 00 54 00 04 00,
    0x244ADFF2
};

int enemy_room_261[] = {
    //num enemy types: 2, num enemies 5,
    //0x24673604, 0x24673608,
    0x02,
    //room spawners:,
    0x24673632,
    //values: 01 00 01 00 00 00  //0x01 - Spirit,
    //0x2467363C, //+0x0A address,
    //room spawners:,
    0x24673646,
    //values: 01 00 4C 00 00 00  //0x4C - Shadow Wolf,
    //0x24673650, //+0x0A address,
    //1: 01 00 01 00 14 00,
    0x24673662,
    //2: 01 00 4C 00 04 00,
    0x246736E2,
    //3: 01 00 4C 00 04 00,
    0x24673762,
    //4: 01 00 4C 00 04 00,
    0x246737E2,
    //5: 01 00 4C 00 04 00,
    0x24673862
};

int enemy_room_262[] = {
    //num enemy types: 2, num enemies 2,
    //0x24837A04, 0x24837A08,
    0x02,
    //room spawners:,
    0x24837A32,
    //values: 01 00 24 00 00 00  //0x24 - Astral Knight,
    //0x24837A3C, //+0x0A address,
    //room spawners:,
    0x24837A46,
    //values: 01 00 20 00 00 00  //0x20 - Axe Knight,
    //0x24837A50, //+0x0A address,
    //1: 01 00 20 00 04 00,
    0x24837A62,
    //2: 01 00 24 00 14 00,
    0x24837AE2
};

int enemy_room_263[] = {
    //num enemy types: 3, num enemies 7,
    //0x24A24B04, 0x24A24B08,
    0x03,
    //room spawners:,
    0x24A24B32,
    //values: 01 00 0F 00 0F 00  //0x0F - Skeleton Warrior,
    //0x24A24B3C, //+0x0A address,
    //room spawners:,
    0x24A24B46,
    //values: 01 00 01 00 0F 00  //0x01 - Spirit,
    //0x24A24B50, //+0x0A address,
    //room spawners:,
    0x24A24B5A,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x24A24B64, //+0x0A address,
    //1: 01 00 01 00 04 00,
    0x24A24B72,
    //2: 01 00 01 00 04 00,
    0x24A24BF2,
    //3: 01 00 01 00 04 00,
    0x24A24C72,
    //4: 01 00 01 00 04 00,
    0x24A24CF2,
    //5: 01 00 0F 00 04 00,
    0x24A24D72,
    //6: 01 00 0F 00 04 00,
    0x24A24DF2,
    //7: 01 00 54 00 04 00,
    0x24A24E72
};

int enemy_room_264[] = {
    //num enemy types: 3, num enemies 5,
    //0x24BDE884, 0x24BDE888,
    0x03,
    //room spawners:,
    0x24BDE8B2,
    //values: 01 00 07 00 0F 00  //0x07 - Zombie,
    //0x24BDE8BC, //+0x0A address,
    //room spawners:,
    0x24BDE8C6,
    //values: 01 00 65 00 02 00  //0x65 - Hanged Man,
    //0x24BDE8D0, //+0x0A address,
    //room spawners:,
    0x24BDE8DA,
    //values: 01 00 4C 00 0F 00  //0x4C - Shadow Wolf,
    //0x24BDE8E4, //+0x0A address,
    //1: 01 00 07 00 16 00,
    0x24BDE8F2,
    //2: 01 00 4C 00 14 00,
    0x24BDE972,
    //3: 01 00 65 00 06 00,
    0x24BDE9F2,
    //4: 01 00 65 00 06 00,
    0x24BDEA72,
    //5: 01 00 65 00 06 00,
    0x24BDEAF2
};

int enemy_room_265[] = {
    //num enemy types: 3, num enemies 3,
    //0x24DA2884, 0x24DA2888,
    0x03,
    //room spawners:,
    0x24DA28B2,
    //values: 01 00 15 00 0F 00  //0x15 - Armor Knight,
    //0x24DA28BC, //+0x0A address,
    //room spawners:,
    0x24DA28C6,
    //values: 01 00 24 00 0D 00  //0x24 - Astral Knight,
    //0x24DA28D0, //+0x0A address,
    //room spawners:,
    0x24DA28DA,
    //values: 01 00 67 00 02 00  //0x67 - Spartacus,
    //0x24DA28E4, //+0x0A address,
    //1: 01 00 15 00 04 00,
    0x24DA28F2,
    //2: 01 00 24 00 14 00,
    0x24DA2972,
    //3: 01 00 67 00 14 00,
    0x24DA29F2
};

int enemy_room_266[] = {
    //num enemy types: 1, num enemies 2,
    //0x24F26C04, 0x24F26C08,
    0x01,
    //room spawners:,
    0x24F26C32,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x24F26C3C, //+0x0A address,
    //1: 01 00 54 00 04 00,
    0x24F26C52,
    //2: 01 00 54 00 04 00,
    0x24F26CD2
};

int enemy_room_267[] = {
    //num enemy types: 2, num enemies 7,
    //0x250F8204, 0x250F8208,
    0x02,
    //room spawners:,
    0x250F8232,
    //values: 01 00 65 00 0F 00  //0x65 - Hanged Man,
    //0x250F823C, //+0x0A address,
    //room spawners:,
    0x250F8246,
    //values: 01 00 54 00 00 00  //0x54 - ???,
    //0x250F8250, //+0x0A address,
    //1: 01 00 65 00 04 00,
    0x250F8262,
    //2: 01 00 65 00 04 00,
    0x250F82E2,
    //3: 01 00 65 00 04 00,
    0x250F8362,
    //4: 01 00 65 00 04 00,
    0x250F83E2,
    //5: 01 00 65 00 14 00,
    0x250F8462,
    //6: 01 00 65 00 14 00,
    0x250F84E2,
    //7: 01 00 54 00 04 00,
    0x250F8562
};

int enemy_room_268[] = {
    //num enemy types: 3, num enemies 7,
    //0x252B3B04, 0x252B3B08,
    0x03,
    //room spawners:,
    0x252B3B32,
    //values: 01 00 20 00 0F 00  //0x20 - Axe Knight,
    //0x252B3B3C, //+0x0A address,
    //room spawners:,
    0x252B3B46,
    //values: 01 00 3A 00 0F 00  //0x3A - Buckbaird,
    //0x252B3B50, //+0x0A address,
    //room spawners:,
    0x252B3B5A,
    //values: 01 00 4B 00 00 00  //0x4B - ???,
    //0x252B3B64, //+0x0A address,
    //1: 01 00 20 00 04 00,
    0x252B3B72,
    //2: 01 00 3A 00 04 00,
    0x252B3BF2,
    //3: 01 00 3A 00 04 00,
    0x252B3C72,
    //4: 01 00 3A 00 04 00,
    0x252B3CF2,
    //5: 01 00 3A 00 04 00,
    0x252B3D72,
    //6: 01 00 3A 00 04 00,
    0x252B3DF2,
    //7: 01 00 4B 00 06 00,
    0x252B3E72
};

int enemy_room_269[] = {
    //num enemy types: 3, num enemies 3,
    //0x25473184, 0x25473188,
    0x03,
    //room spawners:,
    0x254731B2,
    //values: 01 00 0A 00 0F 00  //0x0A - Axe Armor,
    //0x254731BC, //+0x0A address,
    //room spawners:,
    0x254731C6,
    //values: 01 00 6C 00 02 00  //0x6C - Lesser Demon,
    //0x254731D0, //+0x0A address,
    //room spawners:,
    0x254731DA,
    //values: 01 00 01 00 0D 00  //0x01 - Spirit,
    //0x254731E4, //+0x0A address,
    //1: 01 00 0A 00 04 00,
    0x254731F2,
    //2: 01 00 01 00 14 00,
    0x25473272,
    //3: 01 00 6C 00 14 00,
    0x254732F2
};

int enemy_room_270[] = {
    //num enemy types: 1, num enemies 6,
    //0x25637384, 0x25637388,
    0x01,
    //room spawners:,
    0x256373B2,
    //values: 01 00 47 00 02 00  //0x47 - Ghost,
    //0x256373BC, //+0x0A address,
    //1: 01 00 47 00 04 00,
    0x256373D2,
    //2: 01 00 47 00 04 00,
    0x25637452,
    //3: 01 00 47 00 04 00,
    0x256374D2,
    //4: 01 00 47 00 04 00,
    0x25637552,
    //5: 01 00 47 00 04 00,
    0x256375D2,
    //6: 01 00 47 00 04 00,
    0x25637652
};

int enemy_room_271[] = {
    //num enemy types: 1, num enemies 4,
    //0x25AD2D04, 0x25AD2D08,
    0x01,
    //room spawners:,
    0x25AD2D32,
    //values: 01 00 62 00 02 00  //0x62 - Flea Man,
    //0x25AD2D3C, //+0x0A address,
    //1: 01 00 62 00 04 00,
    0x25AD2D52,
    //2: 01 00 62 00 04 00,
    0x25AD2DD2,
    //3: 01 00 62 00 04 00,
    0x25AD2E52,
    //4: 01 00 62 00 04 00,
    0x25AD2ED2
};

int enemy_room_272[] = {
    //num enemy types: 1, num enemies 4,
    //0x25EDA184, 0x25EDA188,
    0x01,
    //room spawners:,
    0x25EDA1B2,
    //values: 01 00 4C 00 0F 00  //0x4C - Shadow Wolf,
    //0x25EDA1BC, //+0x0A address,
    //1: 01 00 4C 00 04 00,
    0x25EDA1D2,
    //2: 01 00 4C 00 04 00,
    0x25EDA252,
    //3: 01 00 4C 00 04 00,
    0x25EDA2D2,
    //4: 01 00 4C 00 04 00,
    0x25EDA352
};

int enemy_room_273[] = {
    //num enemy types: 2, num enemies 2,
    //0x26035A84, 0x26035A88,
    0x02,
    //room spawners:,
    0x26035AB2,
    //values: 01 00 4F 00 02 00  //0x4F - Golden Knight,
    //0x26035ABC, //+0x0A address,
    //room spawners:,
    0x26035AC6,
    //values: 01 00 62 00 02 00  //0x62 - Flea Man,
    //0x26035AD0, //+0x0A address,
    //1: 01 00 4F 00 14 00,
    0x26035AE2,
    //2: 01 00 62 00 14 00,
    0x26035B62
};

int enemy_room_274[] = {
    //num enemy types: 1, num enemies 1,
    //0x26238604, 0x26238608,
    0x01,
    //room spawners:,
    0x26238632,
    //values: 01 00 62 00 02 00  //0x62 - Flea Man,
    //0x2623863C, //+0x0A address,
    //1: 01 00 62 00 14 00,
    0x26238652
};

int enemy_room_275[] = {
    //num enemy types: 2, num enemies 4,
    //0x263D9904, 0x263D9908,
    0x02,
    //room spawners:,
    0x263D9932,
    //values: 01 00 47 00 02 00  //0x47 - Ghost,
    //0x263D993C, //+0x0A address,
    //room spawners:,
    0x263D9946,
    //values: 01 00 0C 00 0D 00  //0x0C - Skeleton,
    //0x263D9950, //+0x0A address,
    //1: 01 00 0C 00 04 00,
    0x263D9962,
    //2: 01 00 0C 00 04 00,
    0x263D99E2,
    //3: 01 00 47 00 14 00,
    0x263D9A62,
    //4: 01 00 47 00 14 00,
    0x263D9AE2
};

int enemy_room_276[] = {
    //num enemy types: 1, num enemies 3,
    //0x265E8F04, 0x265E8F08,
    0x01,
    //room spawners:,
    0x265E8F32,
    //values: 01 00 3A 00 0F 00  //0x3A - Buckbaird,
    //0x265E8F3C, //+0x0A address,
    //1: 01 00 3A 00 04 00,
    0x265E8F52,
    //2: 01 00 3A 00 05 00,
    0x265E8FD2,
    //3: 01 00 3A 00 05 00,
    0x265E9052
};

int enemy_room_277[] = {
    //num enemy types: 2, num enemies 5,
    //0x26CD8484, 0x26CD8488,
    0x02,
    //room spawners:,
    0x26CD84B2,
    //values: 01 00 4C 00 02 00  //0x4C - Shadow Wolf,
    //0x26CD84BC, //+0x0A address,
    //room spawners:,
    0x26CD84C6,
    //values: 01 00 30 00 02 00  //0x30 - Skeleton Hunter,
    //0x26CD84D0, //+0x0A address,
    //1: 01 00 4C 00 04 00,
    0x26CD84E2,
    //2: 01 00 4C 00 04 00,
    0x26CD8562,
    //3: 01 00 4C 00 04 00,
    0x26CD85E2,
    //4: 01 00 4C 00 04 00,
    0x26CD8662,
    //5: 01 00 30 00 04 00,
    0x26CD86E2
};

int enemy_room_278[] = {
    //num enemy types: 1, num enemies 1,
    //0x26E78784, 0x26E78788,
    0x01,
    //room spawners:,
    0x26E787B2,
    //values: 01 00 08 00 02 00  //0x08 - Bat,
    //0x26E787BC, //+0x0A address,
    //1: 01 00 08 00 14 00,
    0x26E787D2
};

int enemy_room_279[] = {
    //num enemy types: 1, num enemies 3,
    //0x270F7484, 0x270F7488,
    0x01,
    //room spawners:,
    0x270F74B2,
    //values: 01 00 62 00 02 00  //0x62 - Flea Man,
    //0x270F74BC, //+0x0A address,
    //1: 01 00 62 00 04 00,
    0x270F74D2,
    //2: 01 00 62 00 04 00,
    0x270F7552,
    //3: 01 00 62 00 04 00,
    0x270F75D2
};

int enemy_room_280[] = {
    //num enemy types: 1, num enemies 2,
    //0x272D6784, 0x272D6788,
    0x01,
    //room spawners:,
    0x272D67B2,
    //values: 01 00 47 00 00 00  //0x47 - Ghost,
    //0x272D67BC, //+0x0A address,
    //1: 01 00 47 00 04 00,
    0x272D67D2,
    //2: 01 00 47 00 04 00,
    0x272D6852
};

int enemy_room_281[] = {
    //num enemy types: 1, num enemies 3,
    //0x274B4C04, 0x274B4C08,
    0x01,
    //room spawners:,
    0x274B4C32,
    //values: 01 00 47 00 0F 00  //0x47 - Ghost,
    //0x274B4C3C, //+0x0A address,
    //1: 01 00 47 00 04 00,
    0x274B4C52,
    //2: 01 00 47 00 04 00,
    0x274B4CD2,
    //3: 01 00 47 00 14 00,
    0x274B4D52
};

int enemy_room_282[] = {
    //num enemy types: 1, num enemies 2,
    //0x276AC304, 0x276AC308,
    0x01,
    //room spawners:,
    0x276AC332,
    //values: 01 00 47 00 00 00  //0x47 - Ghost,
    //0x276AC33C, //+0x0A address,
    //1: 01 00 47 00 04 00,
    0x276AC352,
    //2: 01 00 47 00 04 00,
    0x276AC3D2
};

int enemy_room_283[] = {
    //num enemy types: 1, num enemies 3,
    //0x278A1884, 0x278A1888,
    0x01,
    //room spawners:,
    0x278A18B2,
    //values: 01 00 47 00 0F 00  //0x47 - Ghost,
    //0x278A18BC, //+0x0A address,
    //1: 01 00 47 00 04 00,
    0x278A18D2,
    //2: 01 00 47 00 04 00,
    0x278A1952,
    //3: 01 00 47 00 14 00,
    0x278A19D2
};





int* enemy_rooms[] = {
		enemy_room_1,
enemy_room_2,
enemy_room_3,
enemy_room_4,
enemy_room_5,
enemy_room_6,
enemy_room_7,
enemy_room_8,
enemy_room_9,
enemy_room_10,
enemy_room_11,
enemy_room_12,
enemy_room_13,
enemy_room_14,
enemy_room_15,
enemy_room_16,
enemy_room_17,
enemy_room_18,
enemy_room_19,
enemy_room_20,
enemy_room_21,
enemy_room_22,
enemy_room_23,
enemy_room_24,
enemy_room_25,
enemy_room_26,
enemy_room_27,
enemy_room_28,
enemy_room_29,
enemy_room_30,
enemy_room_31,
enemy_room_32,
enemy_room_33,
enemy_room_34,
enemy_room_35,
enemy_room_36,
enemy_room_37,
enemy_room_38,
enemy_room_39,
enemy_room_40,
enemy_room_41,
enemy_room_42,
enemy_room_43,
enemy_room_44,
enemy_room_45,
//enemy_room_46,
enemy_room_47,
enemy_room_48,
enemy_room_49,
enemy_room_50,
enemy_room_51,
enemy_room_52,
enemy_room_53,
enemy_room_54,
enemy_room_55,
enemy_room_56,
enemy_room_57,
enemy_room_58,
enemy_room_59,
enemy_room_60,
enemy_room_61,
enemy_room_62,
enemy_room_63,
enemy_room_64,
enemy_room_65,
enemy_room_66,
enemy_room_67,
enemy_room_68,
enemy_room_69,
enemy_room_70,
enemy_room_71,
enemy_room_72,
enemy_room_73,
enemy_room_74,
enemy_room_75,
enemy_room_76,
enemy_room_77,
enemy_room_78,
enemy_room_79,
enemy_room_80,
enemy_room_81,
enemy_room_82,
enemy_room_83,
enemy_room_84,
enemy_room_85,
enemy_room_86,
enemy_room_87,
enemy_room_88,
enemy_room_89,
enemy_room_90,
enemy_room_91,
enemy_room_92,
enemy_room_93,
enemy_room_94,
enemy_room_95,
enemy_room_96,
enemy_room_97,
enemy_room_98,
enemy_room_99,
enemy_room_100,
enemy_room_101,
enemy_room_102,
enemy_room_103,
enemy_room_104,
enemy_room_105,
enemy_room_106,
enemy_room_107,
enemy_room_108,
enemy_room_109,
enemy_room_110,
enemy_room_111,
enemy_room_112,
enemy_room_113,
enemy_room_114,
enemy_room_115,
enemy_room_116,
enemy_room_117,
enemy_room_118,
enemy_room_119,
enemy_room_120,
enemy_room_121,
enemy_room_122,
enemy_room_123,
enemy_room_124,
enemy_room_125,
enemy_room_126,
enemy_room_127,
enemy_room_128,
enemy_room_129,
enemy_room_130,
enemy_room_131,
enemy_room_132,
enemy_room_133,
enemy_room_134,
enemy_room_135,
enemy_room_136,
enemy_room_137,
enemy_room_138,
enemy_room_139,
enemy_room_140,
enemy_room_141,
enemy_room_142,
enemy_room_143,
enemy_room_144,
enemy_room_145,
enemy_room_146,
enemy_room_147,
enemy_room_148,
enemy_room_149,
enemy_room_150,
enemy_room_151,
enemy_room_152,
enemy_room_153,
enemy_room_154,
enemy_room_155,
enemy_room_156,
enemy_room_157,
enemy_room_158,
enemy_room_159,
enemy_room_160,
enemy_room_161,
enemy_room_162,
enemy_room_163,
enemy_room_164,
enemy_room_165,
enemy_room_166,
enemy_room_167,
enemy_room_168,
enemy_room_169,
enemy_room_170,
enemy_room_171,
enemy_room_172,
enemy_room_173,
enemy_room_174,
enemy_room_175,
enemy_room_176,
enemy_room_177,
enemy_room_178,
enemy_room_179,
enemy_room_180,
enemy_room_181,
enemy_room_182,
enemy_room_183,
enemy_room_184,
enemy_room_185,
enemy_room_186,
enemy_room_187,
enemy_room_188,
enemy_room_189,
enemy_room_190,
enemy_room_191,
enemy_room_192,
enemy_room_193,
enemy_room_194,
enemy_room_195,
enemy_room_196,
enemy_room_197,
enemy_room_198,
enemy_room_199,
enemy_room_200,
enemy_room_201,
enemy_room_202,
enemy_room_203,
enemy_room_204,
enemy_room_205,
enemy_room_206,
enemy_room_207,
enemy_room_208,
enemy_room_209,
enemy_room_210,
enemy_room_211,
enemy_room_212,
enemy_room_213,
enemy_room_214,
enemy_room_215,
enemy_room_216,
enemy_room_217,
enemy_room_218,
enemy_room_219,
enemy_room_220,
enemy_room_221,
enemy_room_222,
enemy_room_223,
enemy_room_224,
enemy_room_225,
enemy_room_226,
enemy_room_227,
enemy_room_228,
enemy_room_229,
enemy_room_230,
enemy_room_231,
enemy_room_232,
enemy_room_233,
enemy_room_234,
enemy_room_235,
enemy_room_236,
enemy_room_237,
enemy_room_238,
enemy_room_239,
enemy_room_240,
enemy_room_241,
enemy_room_242,
enemy_room_243,
enemy_room_244,
enemy_room_245,
enemy_room_246,
enemy_room_247,
enemy_room_248,
enemy_room_249,
enemy_room_250,
enemy_room_251,
enemy_room_252,
//enemy_room_253,
enemy_room_254,
enemy_room_255,
enemy_room_256,
enemy_room_257,
enemy_room_258,
enemy_room_259,
enemy_room_260,
enemy_room_261,
enemy_room_262,
enemy_room_263,
enemy_room_264,
enemy_room_265,
enemy_room_266,
enemy_room_267,
enemy_room_268,
enemy_room_269,
enemy_room_270,
enemy_room_271,
enemy_room_272,
enemy_room_273,
enemy_room_274,
enemy_room_275,
enemy_room_276,
enemy_room_277,
enemy_room_278,
enemy_room_279,
enemy_room_280,
enemy_room_281,
enemy_room_282,
enemy_room_283,
		
	};
int enemy_rooms_lengths[] = {
sizeof(enemy_room_1)/sizeof(enemy_room_1[0]),
sizeof(enemy_room_2)/sizeof(enemy_room_2[0]),
sizeof(enemy_room_3)/sizeof(enemy_room_3[0]),
sizeof(enemy_room_4)/sizeof(enemy_room_4[0]),
sizeof(enemy_room_5)/sizeof(enemy_room_5[0]),
sizeof(enemy_room_6)/sizeof(enemy_room_6[0]),
sizeof(enemy_room_7)/sizeof(enemy_room_7[0]),
sizeof(enemy_room_8)/sizeof(enemy_room_8[0]),
sizeof(enemy_room_9)/sizeof(enemy_room_9[0]),
sizeof(enemy_room_10)/sizeof(enemy_room_10[0]),
sizeof(enemy_room_11)/sizeof(enemy_room_11[0]),
sizeof(enemy_room_12)/sizeof(enemy_room_12[0]),
sizeof(enemy_room_13)/sizeof(enemy_room_13[0]),
sizeof(enemy_room_14)/sizeof(enemy_room_14[0]),
sizeof(enemy_room_15)/sizeof(enemy_room_15[0]),
sizeof(enemy_room_16)/sizeof(enemy_room_16[0]),
sizeof(enemy_room_17)/sizeof(enemy_room_17[0]),
sizeof(enemy_room_18)/sizeof(enemy_room_18[0]),
sizeof(enemy_room_19)/sizeof(enemy_room_19[0]),
sizeof(enemy_room_20)/sizeof(enemy_room_20[0]),
sizeof(enemy_room_21)/sizeof(enemy_room_21[0]),
sizeof(enemy_room_22)/sizeof(enemy_room_22[0]),
sizeof(enemy_room_23)/sizeof(enemy_room_23[0]),
sizeof(enemy_room_24)/sizeof(enemy_room_24[0]),
sizeof(enemy_room_25)/sizeof(enemy_room_25[0]),
sizeof(enemy_room_26)/sizeof(enemy_room_26[0]),
sizeof(enemy_room_27)/sizeof(enemy_room_27[0]),
sizeof(enemy_room_28)/sizeof(enemy_room_28[0]),
sizeof(enemy_room_29)/sizeof(enemy_room_29[0]),
sizeof(enemy_room_30)/sizeof(enemy_room_30[0]),
sizeof(enemy_room_31)/sizeof(enemy_room_31[0]),
sizeof(enemy_room_32)/sizeof(enemy_room_32[0]),
sizeof(enemy_room_33)/sizeof(enemy_room_33[0]),
sizeof(enemy_room_34)/sizeof(enemy_room_34[0]),
sizeof(enemy_room_35)/sizeof(enemy_room_35[0]),
sizeof(enemy_room_36)/sizeof(enemy_room_36[0]),
sizeof(enemy_room_37)/sizeof(enemy_room_37[0]),
sizeof(enemy_room_38)/sizeof(enemy_room_38[0]),
sizeof(enemy_room_39)/sizeof(enemy_room_39[0]),
sizeof(enemy_room_40)/sizeof(enemy_room_40[0]),
sizeof(enemy_room_41)/sizeof(enemy_room_41[0]),
sizeof(enemy_room_42)/sizeof(enemy_room_42[0]),
sizeof(enemy_room_43)/sizeof(enemy_room_43[0]),
sizeof(enemy_room_44)/sizeof(enemy_room_44[0]),
sizeof(enemy_room_45)/sizeof(enemy_room_45[0]),
//sizeof(enemy_room_46)/sizeof(enemy_room_46[0]),
sizeof(enemy_room_47)/sizeof(enemy_room_47[0]),
sizeof(enemy_room_48)/sizeof(enemy_room_48[0]),
sizeof(enemy_room_49)/sizeof(enemy_room_49[0]),
sizeof(enemy_room_50)/sizeof(enemy_room_50[0]),
sizeof(enemy_room_51)/sizeof(enemy_room_51[0]),
sizeof(enemy_room_52)/sizeof(enemy_room_52[0]),
sizeof(enemy_room_53)/sizeof(enemy_room_53[0]),
sizeof(enemy_room_54)/sizeof(enemy_room_54[0]),
sizeof(enemy_room_55)/sizeof(enemy_room_55[0]),
sizeof(enemy_room_56)/sizeof(enemy_room_56[0]),
sizeof(enemy_room_57)/sizeof(enemy_room_57[0]),
sizeof(enemy_room_58)/sizeof(enemy_room_58[0]),
sizeof(enemy_room_59)/sizeof(enemy_room_59[0]),
sizeof(enemy_room_60)/sizeof(enemy_room_60[0]),
sizeof(enemy_room_61)/sizeof(enemy_room_61[0]),
sizeof(enemy_room_62)/sizeof(enemy_room_62[0]),
sizeof(enemy_room_63)/sizeof(enemy_room_63[0]),
sizeof(enemy_room_64)/sizeof(enemy_room_64[0]),
sizeof(enemy_room_65)/sizeof(enemy_room_65[0]),
sizeof(enemy_room_66)/sizeof(enemy_room_66[0]),
sizeof(enemy_room_67)/sizeof(enemy_room_67[0]),
sizeof(enemy_room_68)/sizeof(enemy_room_68[0]),
sizeof(enemy_room_69)/sizeof(enemy_room_69[0]),
sizeof(enemy_room_70)/sizeof(enemy_room_70[0]),
sizeof(enemy_room_71)/sizeof(enemy_room_71[0]),
sizeof(enemy_room_72)/sizeof(enemy_room_72[0]),
sizeof(enemy_room_73)/sizeof(enemy_room_73[0]),
sizeof(enemy_room_74)/sizeof(enemy_room_74[0]),
sizeof(enemy_room_75)/sizeof(enemy_room_75[0]),
sizeof(enemy_room_76)/sizeof(enemy_room_76[0]),
sizeof(enemy_room_77)/sizeof(enemy_room_77[0]),
sizeof(enemy_room_78)/sizeof(enemy_room_78[0]),
sizeof(enemy_room_79)/sizeof(enemy_room_79[0]),
sizeof(enemy_room_80)/sizeof(enemy_room_80[0]),
sizeof(enemy_room_81)/sizeof(enemy_room_81[0]),
sizeof(enemy_room_82)/sizeof(enemy_room_82[0]),
sizeof(enemy_room_83)/sizeof(enemy_room_83[0]),
sizeof(enemy_room_84)/sizeof(enemy_room_84[0]),
sizeof(enemy_room_85)/sizeof(enemy_room_85[0]),
sizeof(enemy_room_86)/sizeof(enemy_room_86[0]),
sizeof(enemy_room_87)/sizeof(enemy_room_87[0]),
sizeof(enemy_room_88)/sizeof(enemy_room_88[0]),
sizeof(enemy_room_89)/sizeof(enemy_room_89[0]),
sizeof(enemy_room_90)/sizeof(enemy_room_90[0]),
sizeof(enemy_room_91)/sizeof(enemy_room_91[0]),
sizeof(enemy_room_92)/sizeof(enemy_room_92[0]),
sizeof(enemy_room_93)/sizeof(enemy_room_93[0]),
sizeof(enemy_room_94)/sizeof(enemy_room_94[0]),
sizeof(enemy_room_95)/sizeof(enemy_room_95[0]),
sizeof(enemy_room_96)/sizeof(enemy_room_96[0]),
sizeof(enemy_room_97)/sizeof(enemy_room_97[0]),
sizeof(enemy_room_98)/sizeof(enemy_room_98[0]),
sizeof(enemy_room_99)/sizeof(enemy_room_99[0]),
sizeof(enemy_room_100)/sizeof(enemy_room_100[0]),
sizeof(enemy_room_101)/sizeof(enemy_room_101[0]),
sizeof(enemy_room_102)/sizeof(enemy_room_102[0]),
sizeof(enemy_room_103)/sizeof(enemy_room_103[0]),
sizeof(enemy_room_104)/sizeof(enemy_room_104[0]),
sizeof(enemy_room_105)/sizeof(enemy_room_105[0]),
sizeof(enemy_room_106)/sizeof(enemy_room_106[0]),
sizeof(enemy_room_107)/sizeof(enemy_room_107[0]),
sizeof(enemy_room_108)/sizeof(enemy_room_108[0]),
sizeof(enemy_room_109)/sizeof(enemy_room_109[0]),
sizeof(enemy_room_110)/sizeof(enemy_room_110[0]),
sizeof(enemy_room_111)/sizeof(enemy_room_111[0]),
sizeof(enemy_room_112)/sizeof(enemy_room_112[0]),
sizeof(enemy_room_113)/sizeof(enemy_room_113[0]),
sizeof(enemy_room_114)/sizeof(enemy_room_114[0]),
sizeof(enemy_room_115)/sizeof(enemy_room_115[0]),
sizeof(enemy_room_116)/sizeof(enemy_room_116[0]),
sizeof(enemy_room_117)/sizeof(enemy_room_117[0]),
sizeof(enemy_room_118)/sizeof(enemy_room_118[0]),
sizeof(enemy_room_119)/sizeof(enemy_room_119[0]),
sizeof(enemy_room_120)/sizeof(enemy_room_120[0]),
sizeof(enemy_room_121)/sizeof(enemy_room_121[0]),
sizeof(enemy_room_122)/sizeof(enemy_room_122[0]),
sizeof(enemy_room_123)/sizeof(enemy_room_123[0]),
sizeof(enemy_room_124)/sizeof(enemy_room_124[0]),
sizeof(enemy_room_125)/sizeof(enemy_room_125[0]),
sizeof(enemy_room_126)/sizeof(enemy_room_126[0]),
sizeof(enemy_room_127)/sizeof(enemy_room_127[0]),
sizeof(enemy_room_128)/sizeof(enemy_room_128[0]),
sizeof(enemy_room_129)/sizeof(enemy_room_129[0]),
sizeof(enemy_room_130)/sizeof(enemy_room_130[0]),
sizeof(enemy_room_131)/sizeof(enemy_room_131[0]),
sizeof(enemy_room_132)/sizeof(enemy_room_132[0]),
sizeof(enemy_room_133)/sizeof(enemy_room_133[0]),
sizeof(enemy_room_134)/sizeof(enemy_room_134[0]),
sizeof(enemy_room_135)/sizeof(enemy_room_135[0]),
sizeof(enemy_room_136)/sizeof(enemy_room_136[0]),
sizeof(enemy_room_137)/sizeof(enemy_room_137[0]),
sizeof(enemy_room_138)/sizeof(enemy_room_138[0]),
sizeof(enemy_room_139)/sizeof(enemy_room_139[0]),
sizeof(enemy_room_140)/sizeof(enemy_room_140[0]),
sizeof(enemy_room_141)/sizeof(enemy_room_141[0]),
sizeof(enemy_room_142)/sizeof(enemy_room_142[0]),
sizeof(enemy_room_143)/sizeof(enemy_room_143[0]),
sizeof(enemy_room_144)/sizeof(enemy_room_144[0]),
sizeof(enemy_room_145)/sizeof(enemy_room_145[0]),
sizeof(enemy_room_146)/sizeof(enemy_room_146[0]),
sizeof(enemy_room_147)/sizeof(enemy_room_147[0]),
sizeof(enemy_room_148)/sizeof(enemy_room_148[0]),
sizeof(enemy_room_149)/sizeof(enemy_room_149[0]),
sizeof(enemy_room_150)/sizeof(enemy_room_150[0]),
sizeof(enemy_room_151)/sizeof(enemy_room_151[0]),
sizeof(enemy_room_152)/sizeof(enemy_room_152[0]),
sizeof(enemy_room_153)/sizeof(enemy_room_153[0]),
sizeof(enemy_room_154)/sizeof(enemy_room_154[0]),
sizeof(enemy_room_155)/sizeof(enemy_room_155[0]),
sizeof(enemy_room_156)/sizeof(enemy_room_156[0]),
sizeof(enemy_room_157)/sizeof(enemy_room_157[0]),
sizeof(enemy_room_158)/sizeof(enemy_room_158[0]),
sizeof(enemy_room_159)/sizeof(enemy_room_159[0]),
sizeof(enemy_room_160)/sizeof(enemy_room_160[0]),
sizeof(enemy_room_161)/sizeof(enemy_room_161[0]),
sizeof(enemy_room_162)/sizeof(enemy_room_162[0]),
sizeof(enemy_room_163)/sizeof(enemy_room_163[0]),
sizeof(enemy_room_164)/sizeof(enemy_room_164[0]),
sizeof(enemy_room_165)/sizeof(enemy_room_165[0]),
sizeof(enemy_room_166)/sizeof(enemy_room_166[0]),
sizeof(enemy_room_167)/sizeof(enemy_room_167[0]),
sizeof(enemy_room_168)/sizeof(enemy_room_168[0]),
sizeof(enemy_room_169)/sizeof(enemy_room_169[0]),
sizeof(enemy_room_170)/sizeof(enemy_room_170[0]),
sizeof(enemy_room_171)/sizeof(enemy_room_171[0]),
sizeof(enemy_room_172)/sizeof(enemy_room_172[0]),
sizeof(enemy_room_173)/sizeof(enemy_room_173[0]),
sizeof(enemy_room_174)/sizeof(enemy_room_174[0]),
sizeof(enemy_room_175)/sizeof(enemy_room_175[0]),
sizeof(enemy_room_176)/sizeof(enemy_room_176[0]),
sizeof(enemy_room_177)/sizeof(enemy_room_177[0]),
sizeof(enemy_room_178)/sizeof(enemy_room_178[0]),
sizeof(enemy_room_179)/sizeof(enemy_room_179[0]),
sizeof(enemy_room_180)/sizeof(enemy_room_180[0]),
sizeof(enemy_room_181)/sizeof(enemy_room_181[0]),
sizeof(enemy_room_182)/sizeof(enemy_room_182[0]),
sizeof(enemy_room_183)/sizeof(enemy_room_183[0]),
sizeof(enemy_room_184)/sizeof(enemy_room_184[0]),
sizeof(enemy_room_185)/sizeof(enemy_room_185[0]),
sizeof(enemy_room_186)/sizeof(enemy_room_186[0]),
sizeof(enemy_room_187)/sizeof(enemy_room_187[0]),
sizeof(enemy_room_188)/sizeof(enemy_room_188[0]),
sizeof(enemy_room_189)/sizeof(enemy_room_189[0]),
sizeof(enemy_room_190)/sizeof(enemy_room_190[0]),
sizeof(enemy_room_191)/sizeof(enemy_room_191[0]),
sizeof(enemy_room_192)/sizeof(enemy_room_192[0]),
sizeof(enemy_room_193)/sizeof(enemy_room_193[0]),
sizeof(enemy_room_194)/sizeof(enemy_room_194[0]),
sizeof(enemy_room_195)/sizeof(enemy_room_195[0]),
sizeof(enemy_room_196)/sizeof(enemy_room_196[0]),
sizeof(enemy_room_197)/sizeof(enemy_room_197[0]),
sizeof(enemy_room_198)/sizeof(enemy_room_198[0]),
sizeof(enemy_room_199)/sizeof(enemy_room_199[0]),
sizeof(enemy_room_200)/sizeof(enemy_room_200[0]),
sizeof(enemy_room_201)/sizeof(enemy_room_201[0]),
sizeof(enemy_room_202)/sizeof(enemy_room_202[0]),
sizeof(enemy_room_203)/sizeof(enemy_room_203[0]),
sizeof(enemy_room_204)/sizeof(enemy_room_204[0]),
sizeof(enemy_room_205)/sizeof(enemy_room_205[0]),
sizeof(enemy_room_206)/sizeof(enemy_room_206[0]),
sizeof(enemy_room_207)/sizeof(enemy_room_207[0]),
sizeof(enemy_room_208)/sizeof(enemy_room_208[0]),
sizeof(enemy_room_209)/sizeof(enemy_room_209[0]),
sizeof(enemy_room_210)/sizeof(enemy_room_210[0]),
sizeof(enemy_room_211)/sizeof(enemy_room_211[0]),
sizeof(enemy_room_212)/sizeof(enemy_room_212[0]),
sizeof(enemy_room_213)/sizeof(enemy_room_213[0]),
sizeof(enemy_room_214)/sizeof(enemy_room_214[0]),
sizeof(enemy_room_215)/sizeof(enemy_room_215[0]),
sizeof(enemy_room_216)/sizeof(enemy_room_216[0]),
sizeof(enemy_room_217)/sizeof(enemy_room_217[0]),
sizeof(enemy_room_218)/sizeof(enemy_room_218[0]),
sizeof(enemy_room_219)/sizeof(enemy_room_219[0]),
sizeof(enemy_room_220)/sizeof(enemy_room_220[0]),
sizeof(enemy_room_221)/sizeof(enemy_room_221[0]),
sizeof(enemy_room_222)/sizeof(enemy_room_222[0]),
sizeof(enemy_room_223)/sizeof(enemy_room_223[0]),
sizeof(enemy_room_224)/sizeof(enemy_room_224[0]),
sizeof(enemy_room_225)/sizeof(enemy_room_225[0]),
sizeof(enemy_room_226)/sizeof(enemy_room_226[0]),
sizeof(enemy_room_227)/sizeof(enemy_room_227[0]),
sizeof(enemy_room_228)/sizeof(enemy_room_228[0]),
sizeof(enemy_room_229)/sizeof(enemy_room_229[0]),
sizeof(enemy_room_230)/sizeof(enemy_room_230[0]),
sizeof(enemy_room_231)/sizeof(enemy_room_231[0]),
sizeof(enemy_room_232)/sizeof(enemy_room_232[0]),
sizeof(enemy_room_233)/sizeof(enemy_room_233[0]),
sizeof(enemy_room_234)/sizeof(enemy_room_234[0]),
sizeof(enemy_room_235)/sizeof(enemy_room_235[0]),
sizeof(enemy_room_236)/sizeof(enemy_room_236[0]),
sizeof(enemy_room_237)/sizeof(enemy_room_237[0]),
sizeof(enemy_room_238)/sizeof(enemy_room_238[0]),
sizeof(enemy_room_239)/sizeof(enemy_room_239[0]),
sizeof(enemy_room_240)/sizeof(enemy_room_240[0]),
sizeof(enemy_room_241)/sizeof(enemy_room_241[0]),
sizeof(enemy_room_242)/sizeof(enemy_room_242[0]),
sizeof(enemy_room_243)/sizeof(enemy_room_243[0]),
sizeof(enemy_room_244)/sizeof(enemy_room_244[0]),
sizeof(enemy_room_245)/sizeof(enemy_room_245[0]),
sizeof(enemy_room_246)/sizeof(enemy_room_246[0]),
sizeof(enemy_room_247)/sizeof(enemy_room_247[0]),
sizeof(enemy_room_248)/sizeof(enemy_room_248[0]),
sizeof(enemy_room_249)/sizeof(enemy_room_249[0]),
sizeof(enemy_room_250)/sizeof(enemy_room_250[0]),
sizeof(enemy_room_251)/sizeof(enemy_room_251[0]),
sizeof(enemy_room_252)/sizeof(enemy_room_252[0]),
//sizeof(enemy_room_253)/sizeof(enemy_room_253[0]),
sizeof(enemy_room_254)/sizeof(enemy_room_254[0]),
sizeof(enemy_room_255)/sizeof(enemy_room_255[0]),
sizeof(enemy_room_256)/sizeof(enemy_room_256[0]),
sizeof(enemy_room_257)/sizeof(enemy_room_257[0]),
sizeof(enemy_room_258)/sizeof(enemy_room_258[0]),
sizeof(enemy_room_259)/sizeof(enemy_room_259[0]),
sizeof(enemy_room_260)/sizeof(enemy_room_260[0]),
sizeof(enemy_room_261)/sizeof(enemy_room_261[0]),
sizeof(enemy_room_262)/sizeof(enemy_room_262[0]),
sizeof(enemy_room_263)/sizeof(enemy_room_263[0]),
sizeof(enemy_room_264)/sizeof(enemy_room_264[0]),
sizeof(enemy_room_265)/sizeof(enemy_room_265[0]),
sizeof(enemy_room_266)/sizeof(enemy_room_266[0]),
sizeof(enemy_room_267)/sizeof(enemy_room_267[0]),
sizeof(enemy_room_268)/sizeof(enemy_room_268[0]),
sizeof(enemy_room_269)/sizeof(enemy_room_269[0]),
sizeof(enemy_room_270)/sizeof(enemy_room_270[0]),
sizeof(enemy_room_271)/sizeof(enemy_room_271[0]),
sizeof(enemy_room_272)/sizeof(enemy_room_272[0]),
sizeof(enemy_room_273)/sizeof(enemy_room_273[0]),
sizeof(enemy_room_274)/sizeof(enemy_room_274[0]),
sizeof(enemy_room_275)/sizeof(enemy_room_275[0]),
sizeof(enemy_room_276)/sizeof(enemy_room_276[0]),
sizeof(enemy_room_277)/sizeof(enemy_room_277[0]),
sizeof(enemy_room_278)/sizeof(enemy_room_278[0]),
sizeof(enemy_room_279)/sizeof(enemy_room_279[0]),
sizeof(enemy_room_280)/sizeof(enemy_room_280[0]),
sizeof(enemy_room_281)/sizeof(enemy_room_281[0]),
sizeof(enemy_room_282)/sizeof(enemy_room_282[0]),
sizeof(enemy_room_283)/sizeof(enemy_room_283[0]),
};
	
	const size_t enemy_rooms_count = sizeof(enemy_rooms) / sizeof(enemy_rooms[0]);
	
	int forbidden_overwrites[] = {
		0x00,0x02,0x06,//0x0C,
		0x012,0x18,0x1A,0x1B,0x1C,0x1D,
		0x26,0x2B,0x2C,0x2D,
		0x31,0x32,0x33,0x37,0x3B,0x3C,0x3D,0x3E,0x3F,//0x38 //was excluded for item reason, but room excluded
		0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x48,0x49,0x4A,0x4B,0x4D,
		0x50,0x51,0x52,0x54,0x55,0x56,0x59,0x5A,0x5B,0x5C,0x5E,0x5F,
		0x60,0x63,0x64,0x68,0x6E,
		0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x7B,0x7C,0x7D,0x7E,0x7F,
		0x16,0x05,0x62, //excluded for CS reasons TODO: remove rooms instead of enemies...
	}; //and anything larger than 0x7F

	int forbidden_writes[] = {
		0x7F,0x7E,0x7D,0x7C,0x7B,0x7A,0x79,0x78,0x77,0x76,0x75,0x74,0x73,0x72,
		0x6E,0x68,0x64,0x63,0x60,
		0x5F,0x5E,0x5D,0x5C,0x5B,0x5A,0x59,0x56,0x55,0x54,0x52,0x51,0x50,
		0x4F,0x4D,0x4B,0x4A,0x49,0x48,0x46,0x45,0x44,0x43,0x42,0x41,0x40,
		0x3F,0x3E,0x3D,0x3C,0x3B,0x37,0x33,0x32,0x31,
		0x2D,0x2C,0x2B,0x26,
		0x1D,0x1C,0x18,0x12,
		0x06,0x03,0x02,0x00,
		//0x4F, 0x5D, 0x03, //potentially unbeatable enemies
		//0x14, //if it spawns where a flying enemy was that could make it 'impossible' to hit.
	}; //and anything larger than 0x7F (obviously)

	int SIZE = 1;
	unsigned char buffer[SIZE];
	int room_index = 0;	
	unsigned char newByte;
	unsigned char overwritten_val;
	_Bool valid = true;
	int randVal = 0;
	int idx = 0; 
	

	for(int i = 0; i < enemy_rooms_count; i++)
	{
		for(int t = 1; t <= enemy_rooms[i][0]; t++) //<= because it is indexed from 1 through enemy_rooms[i][0];
		{	
			fseek(fp,enemy_rooms[i][t],SEEK_SET);
			fread(buffer,sizeof(buffer),1,fp);
			overwritten_val = buffer[0];
			do
			{
				valid = true;
				randVal = rand() % 0x72;
				for(int n = 0; n < (sizeof(forbidden_writes) / sizeof(forbidden_writes[0])); n++)
				{
					if(randVal == forbidden_writes[n])
					{
						valid = false;
						break;
					}
				}
				for(int n = 1; n < t; n++) //
				{
					fseek(fp,enemy_rooms[i][n],SEEK_SET);
					fread(buffer,sizeof(buffer),1,fp);
					if(randVal == buffer[0])
					{
						valid = false;
						break;
					}
				}
			}while(!valid);// || randVal == overwritten_val); //testing purpose: do not choose the default enemy.
			newByte = (unsigned char)randVal;
			
			fseek(fp,enemy_rooms[i][t],SEEK_SET);
			fread(buffer,sizeof(buffer),1,fp);
			valid = true;
			for(int n = 0; n < (sizeof(forbidden_overwrites) / sizeof(forbidden_overwrites[0])); n++)
			{	
				if(forbidden_overwrites[n] == buffer[0])
				{
					valid = false;
					break;
				}
			}
			if(valid)
			{
				fseek(fp,enemy_rooms[i][t],SEEK_SET);
				fread(buffer,sizeof(buffer),1,fp);
				overwritten_val = buffer[0];
				fseek(fp,enemy_rooms[i][t],SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
				fseek(fp,enemy_rooms[i][t]+0x0A,SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
				for(int n = ((enemy_rooms[i][0])+1); n < enemy_rooms_lengths[i]; n++) 
				{
					fseek(fp,enemy_rooms[i][n],SEEK_SET);
					fread(buffer,sizeof(buffer),1,fp);
					if(buffer[0] == overwritten_val)
					{
						fseek(fp,enemy_rooms[i][n],SEEK_SET);
						fwrite(&newByte,sizeof(newByte),1,fp);
					}
				}
			}
		}
	}
	
	
	//int chance = 90; //might change this to user decided...
	randomize_Golden_Knight_to_Boss(fp,chance);
}

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
		 //Black = 0x5C;
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
	 };
	 int GFbT_torches[] =		
	 {
		//Blue = 0x5A;
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
	 };
	 int ASML_torches[] =
	 {
		//Red = 0x5B;
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
	 };
	 int HoSR_torches[] = 		
	 {
		 //0x6703980, //Yellow = 0x5D;
		0x02DF7290,
		0x02DF7390,
		0x02FC7700,
		0x02FC7780,
		0x02FC7800,
		0x02FC7880,
		0x02FC7900,
		0x02FC7980,
		0x02FC7A00,
	 };
	 int Theatre_torches[] =  	
	 {
		 //White = 0x59;
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

	int attempts = 0;

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
			fseek(fp, torches[i], SEEK_SET); 
			newByte = items_no_progression[randVal];
			fwrite(&newByte,sizeof(newByte),1,fp);
		}
	}


	for(int i = 0; i < item_progression_LEN; i++)
	{
		if ((rand() % 100) < 33)
		{
			valid = true;;

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
				newByte = item_progression[i];
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

void on_golden_knight_toggled(GtkToggleButton *toggle, gpointer user_data)
{
    CheckBoxData *cb_data = (CheckBoxData *)user_data;

    gboolean active = gtk_toggle_button_get_active(toggle);

    // Show or hide the entry box based on checkbox state
    gtk_widget_set_visible(cb_data->golden_knight_entry, active);
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
			if (i == 47) {
				const gchar *text = gtk_entry_get_text(GTK_ENTRY(cb_data->golden_knight_entry));
				int chance = atoi(text);

				// Validate
				if (chance < 0 || chance > 100) {
				GtkWidget *dialog = gtk_message_dialog_new(NULL,
						GTK_DIALOG_MODAL,
						GTK_MESSAGE_ERROR,
						GTK_BUTTONS_OK,
						"Golden Knight chance must be between 0 and 100.");
					gtk_dialog_run(GTK_DIALOG(dialog));
					gtk_widget_destroy(dialog);
					return; // Stop submission
				}

				randomize_enemy_spawns(fp, chance);
				player_chooses[23] = 'Y';
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
		fprintf(fptr,"enemy spawn randomization: %c \n",(char)player_chooses[23]);
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
	// Tell GTK where to find schemas and themes
    g_setenv("GSETTINGS_SCHEMA_DIR", "share/glib-2.0/schemas", TRUE);
    g_setenv("GTK_DATA_PREFIX", ".", TRUE);
    g_setenv("XDG_DATA_DIRS", "share", TRUE);
	
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
		"Torch-sanity","AreaLocking","Enemy Randomization",
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
		"Lets 'most' torches in the game have item drops in them\n","Uses the colored keys to lock the areas\nThis likely breaks the standard functionality\nThe keys are set to be in 'random' torches",
		"Changes enemy spawns in the game\nThis does not have logic to make sure all enemies still exist yet...\n",
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
	/*
		I would like to add a box for the user to type under checkbox 48 if it is checked
		it should have the tool-tip "%chance Golden Knight becomes a boss"
		we should limit that to between 0 and 100 for the input on submitting it. it will be sent to the on_submit to be used for:
			if(i == 47){
				randomize_enemy_spawns(fp,chance);
				player_chooses[23] = 'Y';
			}
	*/
	
	// Create the Golden Knight chance entry (hidden by default)
	cb_data.golden_knight_entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(cb_data.golden_knight_entry), "85"); 
	gtk_widget_set_tooltip_text(
		cb_data.golden_knight_entry,
		"% chance Golden Knight becomes a boss"
	);


	gtk_widget_set_visible(cb_data.golden_knight_entry, FALSE);

	// Attach it under checkbox #48 (index 47)
	int row = 47 / COLS + 1;
	gtk_grid_attach(GTK_GRID(grid), cb_data.golden_knight_entry, 47 % COLS, row, 1, 1);
	g_signal_connect(cb_data.checkboxes[47], "toggled", G_CALLBACK(on_golden_knight_toggled), &cb_data);

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
