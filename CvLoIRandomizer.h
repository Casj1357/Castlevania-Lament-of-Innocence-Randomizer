#ifndef CVLOI_RANDOMIZER_H
#define CVLOI_RANDOMIZER_H

#include <stddef.h>

#define MAX_PATH 512
#define MAX_SKILLS 15    // adjust if you add more skills

typedef struct {
    /* Top entries */
    char iso_path[MAX_PATH];
    char seed_string[MAX_PATH];
    char spoiler_path[MAX_PATH];

    /* Mode selection */
    int mode;

    /* Item shuffle */
    int item_shuffle_enabled;
    int item_shuffle_mode;

    /* Main randomizer options */
    int orbs_anywhere;
    int key_items_not_on_drops;
    int key_doors_not_require_default;
    int warp_room_random;
    int area_locking;
    int torch_sanity;
    int switch_shuffle;

    /* Sphere depth */
    int sphere_depth_enabled;
    int sphere_depth;

    /* Boss / Enemy */
    int boss_loadzones;
    int doppelganger;
    int enemy_random;
    int enemy_percent;
    int enemy_hp;
    int enemy_tolerance;

    /* Changes */
    int modify_powerups;
    int randomize_relic_mp;

    /* Subweapons */
    int subweapon_attacks;
    int pumpkin_subweapons;
    int subweapon_heart_costs;
    int start_with_random_subweapon;

    /* DEF */
    int armor_def_random;
    int starting_def_random;

    /* Starting skills */
    int starting_skills_enabled;
    int skills[MAX_SKILLS];
    int num_skills;

    /* QoL */
    int qol_wolfsfoot;
    int qol_starting;
    int qol_shop;
    int qol_foods;
	int qol_item_limits;
	int item_limit;

    /* Hints / Seed check */
    int hints;
    int check_seed;

} RandomizerSettings;

void run_randomizer(RandomizerSettings *s);
unsigned int hash_string(const char* str);
void show_message(const char* msg);

#endif

