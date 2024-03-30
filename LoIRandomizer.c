#include <stdio.h>
#include <stdlib.h>

#define true 1
#define false 0

_Bool DEBUG = false;

/*TODO:
potential boss HP halver/doubler.

fix ExecutionerDrop?
*/

//offsets
//marker stones
const int MarkerStone1 = 0x20259A60; //Entrance
const int MarkerStone2 = 0x2044CAE0; //Entrance
const int MarkerStone3 = 0x2002CF80; //Entrance
const int MarkerStone4 = 0x09A6A980; //ASML
const int MarkerStone5 = 0x1745D4E0; //GFbT
const int MarkerStone6 = 0x05323DE0; //HoSR
const int MarkerStone7 = 0x23ED6C10; //Theatre
const int MarkerStone8 = 0x117ECB60; //DPoW
//white orb
const int WhiteOrb = 0x0A0C6B60; //ASML
//Potions
const int EntrancePotion = 0x20259AE0;
const int GFbTPotion = 0x14AB1380;
const int DPoWPotion = 0x10BD2300;
const int ASMLFlameElementalPotion = 0x0B240470;
const int ASMLMegingjordPotion = 0x0C556070;
const int HoSR1stPotion = 0x0484E010;
const int HoSR2ndPotion = 0x049E4D80;
const int Theatre1stPotion = 0x23CE4480;
const int Theatre2ndPotion = 0x244AE000;
//High Potions
const int HoSRHighPotion = 0x02FC9690;
const int PoETHighPotion = 0x1F776C60;
const int ASMLHangedManHighPotion = 0x0B09D570;
const int ASMLMegingjordHighPotion = 0x0C555FF0;
const int TheatreHighPotion = 0x21EE6D10;
//Super Potions
const int DPoWSuperPotion = 0x134CE560;
const int ASMLSuperPotion = 0x0C05D210;
const int PotMMSuperPotion = 0x1DE6CB60;
//Heart Repair
const int DPoWHeartRepair = 0x12B56990;
const int TheatreHeartRepair = 0x21D03C90;
//Mana Prism
const int TheatreManaPrism = 0x250F8570;
//Serum
const int EntranceSerum = 0x2044CB60;
const int HoSRSerum = 0x02C297B0;
//Uncurse Potions
const int EntranceUncursePotion = 0x20804560;
const int HoSRUncursePotion = 0x02C29830;
//Magical Ticket
const int EntranceMagicalTicket = 0x208045E0;
//Curtain Time Bell
const int TheatreCurtainTimeBell = 0x233A5CE0;
//Food
const int HoSRNeapolitan = 0x0319A080;
const int ASMLShortcake = 0x0BBD3080;
const int HoSRRamen = 0x08017400; //
//Keys
const int WhiteTigerKey = 0x089FB1E0; //HoSR
const int BlueDragonKey = 0x124C2060; //DPoW
const int RedPhoenixKey = 0x18D2EA60; //GFbT
const int BlackTurtleKey = 0x24F26C60; //Theatre
const int YellowDragonKey = 0x0A77A3E0; //ASML
//Ancient Texts
const int AncientText2 = 0x2245EEE0; //Theatre
const int AncientText1 = 0x0BEFB3F0;
const int AncientText3 = 0x0C1BC8F0;
const int AncientText4 = 0x09D38F00;
//maps
const int Map1 = 0x2002D000; //Entrance
const int Map2 = 0x0D05C770; //ASML
const int Map3 = 0x0ECFB510; //DPoW
const int Map4 = 0x1599D480; //GFbT
const int Map5 = 0x238EC470; //Theatre
//Event Items
const int ToolBag = 0x14467A60;
const int ETablet = 0x0B9E9C60;
const int VITablet = 0x1EDC0BE0;
const int DragonCrest = 0x1EF44160;
const int UnlockJewel = 0x1EAE6B60; //
//Relics
const int WolfsFoot = 0x0AB0E960;
const int SaiseiIncense = 0x198E0B60;
const int BlackBishop = 0x08CFE360;
const int LucifersSword = 0x231FF460;
const int LittleHammer = 0x1233E0E0;
const int MeditativeIncence = 0x13337450; //
//Accessories
const int Draupnir = 0x13A638E0;
const int AromaEarring = 0x22B66E80;
const int RacoonCharm = 0x11C532E0;
const int BloodyCape = 0x08EA35E0;
const int RingofFire = 0x1491E9E0;
const int ArticRing = 0x0C34EAE0;
const int RingofThunder = 0x18A1DF60;
const int HeartBrooch = 0x22EB31E0;
const int JewelCrush = 0x172BE1E0;
const int Megingjord = 0x0C6C00E0;
const int Brisingamen = 0x23539760;
//Heart Max Up
const int ASMLHPHeartUp = 0x0B240370;
const int ASMLFlameElementalHeartUp = 0x0B2403F0;
const int DPoWHeartUp1 = 0x11C53360; //1st?
const int DPoWHeartUp2 = 0x12197CE0; //2nd?
const int GFbTHeartUp = 0x19092400;
const int PotMMHeartUp = 0x1E1DDA60;
const int TheatreHeartUp = 0x22D0CF60;
//MP Max Up
const int HoSRMPMaxUp = 0x088769E0;
const int ASMLMPMaxUp = 0x0BD97A80;
const int DPoWMPMaxUp = 0x12FD6ED0;
const int TheatreMPMaxUp = 0x22646A10;
const int PotMMMPMaxUp = 0x1E371160;
/* */
//HP Max Up
const int HoSRHPMaxUp1 = 0x02C29730;
const int HoSRHPMaxUp2 = 0x086CF560;
const int ASMLHPMaxUp = 0x0B3E7EE0;
const int DPoWHPMaxUpBF1 = 0x0F9B0880;
const int HoSRHPMaxUpBF2 = 0x12004460;
const int TheatreHPMaxUp1 = 0x2305A7E0;
const int TheatreHPMaxUp2 = 0x23761F00;
const int GFbTHPMaxUp = 0x156654E0;
//const int PotMMDoppelHPMaxUp = 0x1C6B9510; //??? could cause problems
const int PotMMHPMaxUp = 0x1EC79C60; //
/* */
//$1000
const int HoSR1000 = 0x026F22D0; //replace with whip of lightning?
const int ASML1000 = 0x0AC7A7E0; //replace with whip of flames?
const int DPoW1000 = 0x134CE5E0; //replace with whip of ice?
//$400
const int GFbT4001 = 0x15665560; //replace with red orb?
const int GFbT4002 = 0x16343290; //replace with blue orb?
const int GFbT4003 = 0x1653B400; //replace with yellow orb?
const int GFbT4004 = 0x17120200; //replace with green orb?
const int HoSR400 = 0x04B91A10; //replace with purple orb?   

const int ASML4001 = 0x09908580; //replace with white bishop?

const int ASML4002 = 0x0ADDC6F0; //replace with Sacrificial doll?
const int DPoW4001 = 0x12B56890; //replace with Jade Mask?

const int DPoW4002 = 0x12B56910; //replace with Diamond?

const int Theatre4001 = 0x23ED6C90; //replace with earth plate?
const int Theatre4002 = 0x240C9D80; //replace with meteor plate?
const int Theatre4003 = 0x242B7880; //replace with moonlight plate?
const int Theatre4004 = 0x24A24E80; //replace with solar plate?
/* */
//Torches
//Knives
const int HoSRKnife = 0x0708D580;
const int ASMLKnife = 0x0DA607F0;
const int GFbTKnife = 0x15FBCB00;
const int PotMMKnife = 0x1A6882F0;
const int TheatreKnife = 0x26237B70;
//Axes
const int HoSRAxe = 0x0335EA80;
const int ASMLAxe = 0x0CED53F0;
const int DPoWFrostElementalAxe = 0x10171D80;
const int DPoWBridgeLeverAxe = 0x11034C70;
const int GFbTAxe = 0x15827A70;
const int PotMMAxe = 0x1A688370;
const int TheatreAxe = 0x242B5610;
//Holy Water
const int HoSRHolyWater = 0x063C3EF0;
const int GFbTHolyWater = 0x17778C00;
const int PotMMHolyWater = 0x1A6883F0;
const int TheatreHolyWater = 0x240C7A90;
//Crystal
const int HoSRCrystal = 0x02DF7310;
const int ASMLCrystal = 0x0D05AA70;
const int DPoWCrystal = 0x114C3AF0;
const int GFbTCrystal = 0x17EC3EF0;
const int PotMMCrystal = 0x1A688470;
const int TheatreCrystal = 0x265E8270;
//Cross
const int HoSRCross = 0x06703A00;
const int ASMLWhiteOrbCross = 0x0D1A24F0;
const int ASML3FCross = 0x0DE96970;
const int DPoWCross = 0x11636670;
const int PotMMCross = 0x1A6884F0;
//$250
const int GFbT250Torch = 0x14E46170; //replace with black orb?
//Drops
//RelicDrops
const int InvincibleJarDrop = 0x00761EEA; //rare
const int CrystalSkullDrop = 0x00760EEA; //rare
/* */
//ACC Drops
const int GargoylePerseussRing = 0x006F0D6A; //rare
const int PoisonLizardAntiPoisonRing = 0x006FD46A; //rare
const int HeavyArmorQigongBelt = 0x006FCCEA; //rare
const int SkeletonWarriorMagneticNecklace = 0x007621EA; //rare
const int CyclopsPikoPikoHammer = 0x006ECCEA; //rare
const int ExecutionerClericsRing = 0x006ECBEA; //rare
const int SpartacusMemberPlate = 0x007623EA; //rare
const int LesserDemonAromaEarring = 0x006E7BEA; //rare
const int EvilStabberAssassinNecklace = 0x006EF76A; //rare
const int DeathReaperPiyoPiyoShoes = 0x006F00EA; //rare
const int LizardKnightTalisman = 0x006FD56A; //rare
const int HangedManCoinofHappiness = 0x006FCDEA; //rare
/* */
//Food Drops
const int DullahanWine = 0x006ED5EA; //rare
const int AxeKnightWine = 0x006E79EA; //rare
const int RedOgreCurry = 0x006ECAEA; //rare
const int MermanSmallMeat = 0x006EFEE8; //common
const int MermanSushi = 0x006EFEEA; //rare
const int FishManSmallMeat = 0x006EFFE8; //common
const int FishManSushi = 0x006EFFEA; //rare
const int ManEatingPlantHamburger = 0x007608E8; //common
const int LizardManBigMeat = 0x006FD36A; //rare
const int FleaManTomatoJuice = 0x006F01EA; //rare
/* */
//Jewel Drops
const int AxeArmorZircon = 0x006E7AE8; //common //replace with turquise?
const int ArmorKnightRuby = 0x006FCBE8; //common
const int HeavyArmorSapphire = 0x006FCCE8; //common
const int CyclopsTurquise = 0x006ECCE8; //common
const int AxeKnightOpal = 0x006E79E8; //common
/* */
const int GolemDrop = 0x006FC998 + 0x50;
const int FlameElementalDrop = 0x006EFA98 + 0x50;
const int MedusaDrop = 0x006FD868; 
const int ThunderElementalDrop = 0x007767E8; 
//const int UndeadParasiteDrop = 0x006EC898 + 0x50;
//const int BlueDoppelDrop
const int FrostElementalDrop = 0x006F0818 + 0x50;
const int JoachimDrop = 0x006FCE98 + 0x50;
const int SuccubusDrop = 0x00776198 + 0x50;

/* */
//Money Drops
/*TODO: TEST */
const int EvilSwordCommon = 0x006EF868; //replace with mobius brooch?

const int EvilSwordRare = 0x006EF86A; //replace with magical ticket?
const int SkeletonArcherRare = 0x007617EA; //replace with memorial ticket?

const int ArmorKnightRare = 0x006FCBEA; //replace with ruby?
const int SkeletonHunterRare = 0x007616EA; //replace with sapphire?
/* */
/*//!Must add addresses for the Orbs and Whips to occupy else not all items are guarenteed to be written!*/
	//potentially fixed!

int PotMMChecks[] = {
	CrystalSkullDrop, InvincibleJarDrop,PotMMCross,PotMMCrystal,PotMMHolyWater,PotMMAxe,PotMMKnife,PotMMHPMaxUp,//PotMMDoppelHPMaxUp,
	PotMMHeartUp,UnlockJewel,DragonCrest,VITablet,PotMMSuperPotion,DullahanWine,LizardKnightTalisman,DeathReaperPiyoPiyoShoes,LesserDemonAromaEarring
};
int LocksChecks[] = {
	Megingjord,HeartBrooch,LittleHammer,BlackBishop,SaiseiIncense,ToolBag,WhiteOrb,Draupnir,WolfsFoot,DragonCrest,SuccubusDrop,GolemDrop
};

int DropRateAddress[] = {
	InvincibleJarDrop + 6, 
	CrystalSkullDrop + 6, //1
	
	GargoylePerseussRing + 6, //2
	PoisonLizardAntiPoisonRing + 6,
	HeavyArmorQigongBelt + 6,
	SkeletonWarriorMagneticNecklace + 6,
	CyclopsPikoPikoHammer + 6,
	ExecutionerClericsRing + 6,
    SpartacusMemberPlate + 6,
    LesserDemonAromaEarring + 6, 
	EvilStabberAssassinNecklace + 6,
	DeathReaperPiyoPiyoShoes + 6,
	LizardKnightTalisman + 6,
	HangedManCoinofHappiness + 6, //13
	
	DullahanWine + 6, //test //14
	AxeKnightWine + 6, //test
	RedOgreCurry + 6,
	MermanSmallMeat + 4,
	MermanSushi + 6,
	FishManSmallMeat + 4,
	FishManSushi + 6,
	ManEatingPlantHamburger + 4,
	LizardManBigMeat + 6,
	FleaManTomatoJuice + 6, //23
	
	AxeArmorZircon + 4, //24
	ArmorKnightRuby + 4,
	HeavyArmorSapphire + 4,
	CyclopsTurquise + 4,
	AxeKnightOpal + 4, //28
	
	//Boss Drops rates
	GolemDrop + 4, //29
	FlameElementalDrop + 4,
	MedusaDrop + 4,
	ThunderElementalDrop + 4,
	//UndeadParasiteDrop + 4,
	FrostElementalDrop + 4,
	JoachimDrop + 4,
	SuccubusDrop + 4, //35
	
	EvilSwordCommon + 4, //36
	EvilSwordRare + 6,
	SkeletonArcherRare + 6,
	ArmorKnightRare + 6,
	SkeletonHunterRare + 6 //40
};

//Array of Addresses
int AddressArray[] = {
	MarkerStone1,MarkerStone2,MarkerStone3,MarkerStone4,MarkerStone5,MarkerStone6,MarkerStone7,MarkerStone8,
	WhiteOrb,
	EntrancePotion,GFbTPotion,DPoWPotion,ASMLFlameElementalPotion,ASMLMegingjordPotion,HoSR1stPotion,HoSR2ndPotion,Theatre1stPotion,Theatre2ndPotion,
	HoSRHighPotion,PoETHighPotion,ASMLHangedManHighPotion,ASMLMegingjordHighPotion,TheatreHighPotion,
	DPoWSuperPotion,ASMLSuperPotion,PotMMSuperPotion,
	DPoWHeartRepair,TheatreHeartRepair,
	TheatreManaPrism,
	EntranceSerum,HoSRSerum,
	EntranceUncursePotion,HoSRUncursePotion,
	EntranceMagicalTicket,
	TheatreCurtainTimeBell,
	HoSRNeapolitan,ASMLShortcake,HoSRRamen,
	WhiteTigerKey,BlueDragonKey,RedPhoenixKey,BlackTurtleKey,YellowDragonKey,
	AncientText2,AncientText1,AncientText3,AncientText4,
	Map1,Map2,Map3,Map4,Map5,
	ToolBag,ETablet,VITablet,DragonCrest,UnlockJewel,
	WolfsFoot,SaiseiIncense,BlackBishop,LucifersSword,LittleHammer,MeditativeIncence,
	Draupnir,AromaEarring,RacoonCharm,BloodyCape,RingofFire,ArticRing,RingofThunder,HeartBrooch,JewelCrush,Megingjord,Brisingamen,
	ASMLHPHeartUp,ASMLFlameElementalHeartUp,DPoWHeartUp1,DPoWHeartUp2,GFbTHeartUp,PotMMHeartUp,TheatreHeartUp,
	HoSRMPMaxUp,ASMLMPMaxUp,DPoWMPMaxUp,TheatreMPMaxUp,PotMMMPMaxUp,
	HoSRHPMaxUp1,HoSRHPMaxUp2,ASMLHPMaxUp,DPoWHPMaxUpBF1,HoSRHPMaxUpBF2,TheatreHPMaxUp1,TheatreHPMaxUp2,GFbTHPMaxUp,//PotMMDoppelHPMaxUp,
	PotMMHPMaxUp,
	HoSR1000,ASML1000,DPoW1000,
	GFbT4001,GFbT4002,GFbT4003,GFbT4004,HoSR400,
	ASML4001,
	ASML4002,DPoW4001,
	DPoW4002,
	Theatre4001,Theatre4002,Theatre4003,Theatre4004,
	HoSRKnife,ASMLKnife,GFbTKnife,PotMMKnife,TheatreKnife,
	HoSRAxe,ASMLAxe,DPoWFrostElementalAxe,DPoWBridgeLeverAxe,GFbTAxe,PotMMAxe,TheatreAxe,
	HoSRHolyWater,GFbTHolyWater,PotMMHolyWater,TheatreHolyWater,
	HoSRCrystal,ASMLCrystal,DPoWCrystal,GFbTCrystal,PotMMCrystal,TheatreCrystal,
	HoSRCross,ASMLWhiteOrbCross,ASML3FCross,DPoWCross,PotMMCross,
	GFbT250Torch,
	InvincibleJarDrop,CrystalSkullDrop,
	GargoylePerseussRing,PoisonLizardAntiPoisonRing,HeavyArmorQigongBelt,SkeletonWarriorMagneticNecklace,CyclopsPikoPikoHammer,ExecutionerClericsRing,SpartacusMemberPlate,LesserDemonAromaEarring,EvilStabberAssassinNecklace,DeathReaperPiyoPiyoShoes,LizardKnightTalisman,HangedManCoinofHappiness,
	DullahanWine,AxeKnightWine,RedOgreCurry,MermanSmallMeat,MermanSushi,FishManSmallMeat,FishManSushi,ManEatingPlantHamburger,LizardManBigMeat,FleaManTomatoJuice,
	AxeArmorZircon,ArmorKnightRuby,HeavyArmorSapphire,CyclopsTurquise,AxeKnightOpal,
	GolemDrop,FlameElementalDrop,MedusaDrop,ThunderElementalDrop,//UndeadParasiteDrop,
		FrostElementalDrop,JoachimDrop,SuccubusDrop,
	EvilSwordCommon,
	EvilSwordRare,SkeletonArcherRare,
	ArmorKnightRare,SkeletonHunterRare
};

int ItemArray[] = {
	0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,0x70,0x71,
	0x84,
	0x2A,0x2A,0x2A,0x2A,0x2A,0x2A,0x2A,0x2A,0x2A,
	0x2B,0x2B,0x2B,0x2B,0x2B,
	0x2C,0x2C,0x2C,
	0x2D,0x2D,
	0x2E,
	0x2F,0x2F,
	0x30,0x30,
	0x31,
	0x42,
	0x4C,0x48,0x45,
	0x59,0x5A,0x5B,0x5C,0x5D,
	0x62,0x61,0x72,0x73,
	0x63,0x64,0x65,0x66,0x67,
	0x5F,0x55,0x56,0x5E,0x58,
	0x86,0x87,0x8B,0x8D,0x8F,0x88,
	0x0A,0x0B,0x0F,0x13,0x14,0x15,0x17,0x1D,0x1E,0x20,
	0x91,0x91,0x91,0x91,0x91,0x91,0x91,
	0x92,0x92,0x92,0x92,0x92,
	0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,//0x90,
	0x04,0x02,0x03,
	0x7F,0x80,0x81,0x82,0x83,
	0x8C,
	0x1A,0x22,
	0x3C,
	0x06,0x07,0x08,0x09,
	0xAA,0xAA,0xAA,0xAA,0xAA,
	0xAB,0xAB,0xAB,0xAB,0xAB,0xAB,0xAB,
	0xAC,0xAC,0xAC,0xAC,
	0xAD,0xAD,0xAD,0xAD,0xAD,0xAD,
	0xAE,0xAE,0xAE,0xAE,0xAE,
	0x85,
	0x89,0x8A,
	0x10,0x11,0x0C,0x1B,0x21,0x12,0x1F,0x0B,0x1C,0x19,0x16,0x0D,
	0x46,0x46,0x4A,0x43,0x49,0x43,0x49,0x47,0x44,0x4B,
	0x40,0x3D,0x3E,0x40,0x3F,
	0x2A,0x2A,0x2A,0x2A,0x2A,0x2A,0x2A,//0x2A, //boss drops
	0x18,
	0x31,0x32,
	0x3D,0x3E,
	0x05
}; //list seems to be missing an item...

char *AddressArrayNames[] = {
"MarkerStone1",
"MarkerStone2",
"MarkerStone3",
"MarkerStone4",
"MarkerStone5",
"MarkerStone6",
"MarkerStone7",
"MarkerStone8",
"WhiteOrb",
"EntrancePotion",
"GFbTPotion",
"DPoWPotion",
"ASMLFlameElementalPotion",
"ASMLMegingjordPotion",
"HoSR1stPotion",
"HoSR2ndPotion",
"Theatre1stPotion",
"Theatre2ndPotion",
"HoSRHighPotion",
"PoETHighPotion",
"ASMLHangedManHighPotion",
"ASMLMegingjordHighPotion",
"TheatreHighPotion",
"DPoWSuperPotion",
"ASMLSuperPotion",
"PotMMSuperPotion",
"DPoWHeartRepair",
"TheatreHeartRepair",
"TheatreManaPrism",
"EntranceSerum",
"HoSRSerum",
"EntranceUncursePotion",
"HoSRUncursePotion",
"EntranceMagicalTicket",
"TheatreCurtainTimeBell",
"HoSRNeapolitan",
"ASMLShortcake",
"HoSRRamen",
"WhiteTigerKey",
"BlueDragonKey",
"RedPhoenixKey",
"BlackTurtleKey",
"YellowDragonKey",
"AncientText2",
"AncientText1",
"AncientText3",
"AncientText4",
"Map1",
"Map2",
"Map3",
"Map4",
"Map5",
"ToolBag",
"ETablet",
"VITablet",
"DragonCrest",
"UnlockJewel",
"WolfsFoot",
"SaiseiIncense",
"BlackBishop",
"LucifersSword",
"LittleHammer",
"MeditativeIncence",
"Draupnir",
"AromaEarring",
"RacoonCharm",
"BloodyCape",
"RingofFire",
"ArticRing",
"RingofThunder",
"HeartBrooch",
"JewelCrush",
"Megingjord",
"Brisingamen",
"ASMLHPHeartUp",
"ASMLFlameElementalHeartUp",
"DPoWHeartUp1",
"DPoWHeartUp2",
"GFbTHeartUp",
"PotMMHeartUp",
"TheatreHeartUp",
"HoSRMPMaxUp",
"ASMLMPMaxUp",
"DPoWMPMaxUp",
"TheatreMPMaxUp",
"PotMMMPMaxUp",
"HoSRHPMaxUp1",
"HoSRHPMaxUp2",
"ASMLHPMaxUp",
"DPoWHPMaxUpBF1",
"HoSRHPMaxUpBF2",
"TheatreHPMaxUp1",
"TheatreHPMaxUp2",
"GFbTHPMaxUp",
//"PotMMDoppelHPMaxUp",
"PotMMHPMaxUp",
"HoSR1000",
"ASML1000",
"DPoW1000",
"GFbT4001",
"GFbT4002",
"GFbT4003",
"GFbT4004",
"HoSR400",
"ASML4001",
"ASML4002",
"DPoW4001",
"DPoW4002",
"Theatre4001",
"Theatre4002",
"Theatre4003",
"Theatre4004",
	//111
"HoSRKnife", //ID 111
"ASMLKnife",
"GFbTKnife",
"PotMMKnife",
"TheatreKnife",
"HoSRAxe",
"ASMLAxe",
"DPoWFrostElementalAxe",
"DPoWBridgeLeverAxe",
"GFbTAxe",
"PotMMAxe",
"TheatreAxe",
"HoSRHolyWater",
"GFbTHolyWater",
"PotMMHolyWater",
"TheatreHolyWater",
"HoSRCrystal",
"ASMLCrystal",
"DPoWCrystal",
"GFbTCrystal",
"PotMMCrystal",
"TheatreCrystal",
"HoSRCross",
"ASMLWhiteOrbCross",
"ASML3FCross",
"DPoWCross",
"PotMMCross",
"GFbT250Torch",
"InvincibleJarDrop",
"CrystalSkullDrop",
"GargoylePerseussRing",
"PoisonLizardAntiPoisonRing",
"HeavyArmorQigongBelt",
"SkeletonWarriorMagneticNecklace",
"CyclopsPikoPikoHammer",
"ExecutionerClericsRing",
"SpartacusMemberPlate",
"LesserDemonAromaEarring",
"EvilStabberAssassinNecklace",
"DeathReaperPiyoPiyoShoes",
"LizardKnightTalisman",
"HangedManCoinofHappiness",
"DullahanWine",
"AxeKnightWine",
"RedOgreCurry",
"MermanSmallMeat",
"MermanSushi",
"FishManSmallMeat",
"FishManSushi",
"ManEatingPlantHamburger",
"LizardManBigMeat",
"FleaManTomatoJuice",
"AxeArmorZircon",
"ArmorKnightRuby",
"HeavyArmorSapphire",
"CyclopsTurquise",
"AxeKnightOpal",
"GolemDrop",
"FlameElementalDrop",
"MedusaDrop",
"ThunderElementalDrop",
//"UndeadParasiteDrop",
"FrostElementalDrop",
"JoachimDrop",
"SuccubusDrop",
"EvilSwordCommon",
"EvilSwordRare",
"SkeletonArcherRare",
"ArmorKnightRare",
"SkeletonHunterRare"
};


char *itemNames[] = {"NONE","Whip of Alchemy","Whip of Flames","Whip of Ice","Whip of Lightning","Vampire Killer","Earth Plate","Meteor Plate","Moonlight Plate",
"Solar Plate","Draupnir","Aroma Earring","Qigong Belt","Coin of Happiness","Raccoon Charm","Bloody Cape","Perseus Ring","Anti-Poison Ring","Cleric's Ring",
"Ring of Fire","Artic Ring","Ring of Thunder","Talisman","Heart Brooch","Mobius's Brooch","Piyo-piyo Shoes","Sacrificial Doll","Magnetic Necklace","Assassin Necklace",
"Jewel Crush","Megingjord","MemberPlate","Brisingamen","Piko-piko Hammer","Jade Mask","UNUSED","UNUSED","UNUSED","UNUSED","UNUSED","UNUSED","UNUSED","Potion",
"High Potion","Super Potion","Heart Repair","Mana Prism","Serum","Uncurse Potion","Magical Ticket","Memorial Ticket","UNUSED","UNUSED","UNUSED","UNUSED","UNUSED","UNUSED"
,"UNUSED","UNUSED","UNUSED","Diamond","Ruby","Sapphire","Opal","Turquoise","Zircon","Curtain Time Bell","Small Meat","Big Meat","Ramen","Wine","Hamburger","Shortcake",
"Sushi","Curry","Tomato Juice","Neapolitan","UNUSED","UNUSED","UNUSED","UNUSED","UNUSED","UNUSED","UNUSED","UNUSED","e Tablet", "VI Tablet", "IV Tablet","Unlock Jewel",
"White Tiger Key","Blue Dragon Key","Red Phoenix Key","Black Turtle Key","Yellow Dragon Key","Dragon Crest","Tool Bag","Music Box","Ancient Text 1","Ancient Text 2",
"Map 1","Map 2","Map 3","Map 4","Map 5","Map 6","Map 7","Marker Stone 1","Marker Stone 2","Marker Stone 3","Marker Stone 4","Marker Stone 5","Marker Stone 6","Marker Stone 7",
"Marker Stone 8","Ancient Text 3","Ancient Text 4","UNUSED","UNUSED","UNUSED","UNUSED","UNUSED","UNUSED","UNUSED","UNUSED","UNUSED","UNUSED","UNUSED","Red Orb","Blue Orb",
"Yellow Orb","Green Orb","Purple Orb","White Orb","Black Orb","Wolf's Foot","Saisei Incense","Meditative Incense","Invincible Jar","Crystal Skull","Black Bishop","White Bishop",
"Lucifer's Sword","Svarog Statue","Little Hammer","HP Max Up","Heart Max Up","MP Max Up","small heart","big heart","$100","$25","$250","$400","$1000","UNUSED","UNUSED","UNUSED"
,"UNUSED","UNUSED","UNUSED","UNUSED","UNUSED","$1","$5","$10","Rosario","Bloody Skull","UNUSED","UNUSED","UNUSED","Knife","Axe","Holy Water","Crystal","Cross","Pumpkin"
};

int addressArrayLength = *(&AddressArray + 1) - AddressArray;
int addressArrayNamesLength = *(&AddressArrayNames + 1) - AddressArrayNames;

struct addressList {
	int addressOffset;
	char *name;
};


int main()
{
		char choose999;
		do
		{
			if(choose999 == 'Y' || choose999 == 'N')
				break;
			printf("Do you want to read the list of compromises and new features that may be comming? Y/N \n");
			scanf("%c",&choose999);
		}while(choose999 != 'Y' || choose999 != 'N');
		if(choose999 == 'Y')
		{
			printf("Svarog Statue not randomized due to Golden Knight \n");
			printf("Entering Pagoda still gives a copy of Vampire Killer \n");
			printf("GFbT $250 included from torch suffles but does not show on map like sub-weapon torches \n");
			printf("All bosses default 'drop' set to whip of alchemy so no softlocks from non-whip/non-orb items on bosses \n");
			printf("HP Up after Red Doppel not changed due to potential softlocks \n");
			printf("Forgotten One drop not included because of phases potentially causing problems \n");
			printf("Undead Parasite drop not included because of it not working properly \n");
			printf("store shuffle can never include whips because the game does not check if those are possible to sell \n");
			printf("store shuffle can not include armors by default because of how the game handles the store \n");
			printf("there is currently no 'glitch' logic for skipping wolf's foot requirements \n");
			//printf("starting skills have not been found yet (comming soon?) \n");
			//printf("boss hp shuffler not implemented (yet) [Comming Soon] \n");
			printf("Guarentee beatable seed logic not implemented (yet) \n");
			printf("Logic for Crazy Mode not done \n");
			printf("Hints not done for Orb/Whip Shuffle (yet) \n");
			printf("\n");
			//printf("Armor DEF values shuffler not implemented (yet) [Comming Soon] \n");
			//printf("Power Up increase value(s) could be changed from default [Comming Soon] \n");
			printf("Setting what items one wants randomized instead of using the 'default' list [comming soon?] \n");
			//printf("starting INT randomization not implemented (yet) [Comming Soon] \n");
			//printf("starting heart count randomization not implemented (yet) [Comming Soon] \n");
			printf("\nThere may be more things comming in the future!\n\n");
			
		}	
		if(addressArrayLength != addressArrayNamesLength)
		{
			printf("addressArrayLength != addressArrayNamesLength \n");
			printf("error \n");
			exit(1);
		}
		else
		{
			if(DEBUG){printf("length is fine \n");}
		}
		fflush(stdin);
		char choose;
		char choose3;
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
		char choose33;
		do
		{
			if(choose33 == 'Y' || choose33 == 'N')
				break;
			printf("Do you want to see the location list? Y/N \n");
			scanf("%c",&choose33);
		}while(choose33 != 'Y' || choose33 != 'N');
		if(choose33 == 'Y')
		{
			printf("\n");
			for(int i = 0; i < addressArrayLength; i++)
			{
				printf("%s",AddressArrayNames[i]);printf("\n");
			}
			printf("\n");
		}
		int xseed = 0;
		struct addressList arr_addressList[addressArrayLength];
		for(int i = 0; i < addressArrayLength; i++)
		{
			arr_addressList[i].addressOffset = AddressArray[i];
			arr_addressList[i].name = AddressArrayNames[i];
		}
		int ItemArrayLength = *(&ItemArray + 1) - ItemArray;
		int DropRateAddressLenth = *(&DropRateAddress + 1) - DropRateAddress;
		int PotMMChecksLength = *(&PotMMChecks + 1) - PotMMChecks;
		int LocksChecksLength = *(&LocksChecks + 1) - LocksChecks;
		if(ItemArrayLength > addressArrayLength)
		{
			printf("Not all items can be written! \n");
			exit(0);
		}
		if(ItemArrayLength != addressArrayLength)
		{
			printf("ItemArrayLength != addressArrayLength \n");
			printf("%d",ItemArrayLength);printf(" vs. ");printf("%d",addressArrayLength);printf("\n");
			printf("error \n");
			exit(1);
		}

		int randValue = 0;
		unsigned char newByte = 0xED;
		FILE* fp;
		FILE* fptr;
		//get user input for seed
		printf("Type a seed number: \n");
		scanf("%d",&xseed);
		printf("Seed will be: %d", xseed); printf("\n");
		
		srand(xseed);
		fflush(stdin);
		printf("Type the filepath to the NTSC-U iso version of Castlevania: Lament of Innocence \n(Spaces are NOT allowed; including the filename)\n *If your filename has spaces in it you will need to rename the file to not have spaces before using it here* \n(This is case sensitive) \n");
		char choose2[128];
		scanf("%s",&choose2);
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
		void dropRateEdit()
		{
			for(int i = 0; i < DropRateAddressLenth; i++)
			{
				for(int var = 0; var < 4; var++)
				{
					fseek(fp,DropRateAddress[i]+var,SEEK_SET);
					newByte = 0x00;
					if(var == 3)
					{
						newByte = 0x3F;
					}
					if(var == 2)
					{
						if(i >= 29 && i <= 35)
						{
							newByte = 0x80;
						}							
					}
					fwrite(&newByte,sizeof(newByte),1,fp);
				}
			}
			for(int i = addressArrayLength-1; i > addressArrayLength-6; i--)
			{
				newByte = 0x01;
				fseek(fp,AddressArray[i],SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
				fseek(fp,AddressArray[i]+1,SEEK_SET);
				newByte = 0x00;
				fwrite(&newByte,sizeof(newByte),1,fp);
			}
			//Boss Drop Rates (MUST BE 100%)
			//GolemDrop [Length - 12]; SuccubusDrop [Length - 6];
			for(int i = addressArrayLength-6; i > addressArrayLength-12; i--)
			{
				newByte = 0x2A;
				fseek(fp,AddressArray[i],SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
			}
		}
		void shopEdit()
		{
			const int shopPotionPrice = 0x003DAC08 + 0x44;
			const int shopHighPotionPriceStart = 0x003DAC58 + 0x44;
			const int shopSerumPriceStart = 0x003DAD98 + 0x44;
			const int shopUncursePotionPriceStart = 0x003DADE8 + 0x44;
			const int shopMagicalTicketPriceStart = 0x003DAE38 + 0x44;
			const int shopMemorialTicketPriceStart = 0x003DAE88 + 0x44;
			
			const int shopDiamondPriceStart = 0x003DB1A8 + 0x44;
			const int shopRubyPriceStart = 0x003DB1F8 + 0x44;
			const int shopSapphirePriceStart = 0x003DB248 + 0x44;
			const int shopOpalPriceStart = 0x003DB298 + 0x44;
			const int shopTurquisePriceStart = 0x003DB2E8 + 0x44;
			const int shopZirconPriceStart = 0x003DB338 + 0x44;	
			
			const int shopMobiusBroochPriceStart = 0x003DA2C0 + 0x44;
			const int shopSacrificialDollPriceStart = 0x003DA3C8 + 0x44;
			const int shopJadeMaskPriceStart = 0x003DA7E8 + 0x44;
			
			const int shopEarthPlacePriceStart = 0x003D9978 + 0x44;
			const int shopMeteorPlatePriceStart = 0x003D99FC + 0x44;
			const int shopMoonlightPlatePriceStart = 0x003D9A80 + 0x44;
			const int shopSolarPlatePriceStart = 0x003D9B04 + 0x44;
			
			const int shopWhiteBishopPriceStart = 0x003DC8F0 + 0x40;
			
			// const int shopMusicBoxPriceStart;
			
			int shopAddressArray[] = 
			{
				shopPotionPrice,shopHighPotionPriceStart,shopSerumPriceStart,shopUncursePotionPriceStart,shopMagicalTicketPriceStart,shopMemorialTicketPriceStart,
				shopDiamondPriceStart,shopRubyPriceStart,shopSapphirePriceStart,shopOpalPriceStart,shopTurquisePriceStart,shopZirconPriceStart,
				shopMobiusBroochPriceStart,shopSacrificialDollPriceStart,shopJadeMaskPriceStart,
				shopEarthPlacePriceStart,shopMeteorPlatePriceStart,shopMoonlightPlatePriceStart,shopSolarPlatePriceStart,
				shopWhiteBishopPriceStart
			};
			int shopAddressArrayLength = *(&shopAddressArray + 1) - shopAddressArray;
			
			for(int i = 0; i < shopAddressArrayLength; i++)
			{
				newByte = 0x00;
				fseek(fp,shopAddressArray[i],SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
				fseek(fp,shopAddressArray[i]+1,SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
				fseek(fp,shopAddressArray[i]+2,SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
				fseek(fp,shopAddressArray[i]+3,SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
			}
 		}
		void changeORBSandWHIPS(_Bool crazyPlayer)
		{
			const int startOrbsWhipsTable = 0x0044F590;
			//const int endOrbsWhipsTable = 0x0044F5BF;
			newByte = 0x01;
			for(int i = 0; i < 12; i++)
			{
				if(crazyPlayer != false)
				{
					if(i % 4 == 0)
					{
						newByte = 0x01;
					}
					else if (i % 4 == 1)
					{
						newByte = 0x05;
					}
					else if(i % 4 == 2)
					{
						newByte = 0x04;
					}
					else if(i % 4 == 3)
					{
						newByte = 0x84;
					}
				}
				fseek(fp, startOrbsWhipsTable + (4*i) ,SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
			}
		}
		void attemptEncyclopediafix()
		{
			int moneySprites[] = {
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
		_Bool storeModified = false;
		int shopBuyPriceAddress[] = 
			{
				//relics
				0x003DC780, //Wolf's Foot [0]
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
				//0x003DBCD0, //Music Box
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
		void ActivateShop()
		{
			storeModified = true;
			
			int n;
			int SIZE = 1;
			unsigned char buffer[SIZE];
			int itemToFind = 0x01;
			for(int i = 0; i < shopBuyPriceAddressLength; i++)
			{
				newByte = 0x00;
				fseek(fp,shopBuyPriceAddress[i],SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
				fseek(fp,shopBuyPriceAddress[i]+1,SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
				fseek(fp,shopBuyPriceAddress[i]+2,SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
				fseek(fp,shopBuyPriceAddress[i]+3,SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);				
			}
			for(int i = 0; i < shopBuyPriceAddressLength; i++)
			{
				if( i != 34 || i != 24 || i != 27 || i != 8)
				{
					randValue = rand() % 10;
					if(randValue == 0)
					{
						if((i >= 35 && i <= 60))
						{
							randValue = rand() % 0x80;
							newByte = randValue;
							fseek(fp,shopBuyPriceAddress[i]+1,SEEK_SET);
							fwrite(&newByte,sizeof(newByte),1,fp);
							randValue = rand() % 0xFF;
							newByte = randValue += 1;
							fseek(fp,shopBuyPriceAddress[i],SEEK_SET);
							fwrite(&newByte,sizeof(newByte),1,fp);
							newByte = 0x00;
							fseek(fp,shopBuyPriceAddress[i]+2,SEEK_SET);
							fwrite(&newByte,sizeof(newByte),1,fp);
							fseek(fp,shopBuyPriceAddress[i]+3,SEEK_SET);
							fwrite(&newByte,sizeof(newByte),1,fp);
						}
						else
						{
							randValue = rand() % 0xFF;
							newByte = randValue += 1;
							fseek(fp,shopBuyPriceAddress[i],SEEK_SET);
							fwrite(&newByte,sizeof(newByte),1,fp);
							randValue = rand() % 0x100;
							newByte = randValue;
							fseek(fp,shopBuyPriceAddress[i]+1,SEEK_SET);
							fwrite(&newByte,sizeof(newByte),1,fp);
							randValue = rand() % 0x0E;
							newByte = randValue;
							fseek(fp,shopBuyPriceAddress[i]+2,SEEK_SET);
							fwrite(&newByte,sizeof(newByte),1,fp);
							newByte = 0x00;
							fseek(fp,shopBuyPriceAddress[i]+3,SEEK_SET);
							fwrite(&newByte,sizeof(newByte),1,fp);
						}
						if((i >= 35 && i <= 60) && i != 50)
						{
							//mostly usable items (not replacing them on the ground)
						}
						else if( i == 34 || i == 24 || i == 27 || i == 8)
						{
							//34: Jade mask: shouldn't need completed save file
							//24: Mobius Brooch: shouldn't need completed crazy file
							//27: Magnetic Necklace: ??? (doesn't work?)
							//8: Svarog Statue: Not Randomized
						}
						else
						{
							if(i >=0 && i <= 9)
							{
								itemToFind = i + 0x86;
							}
							else if(i >= 10 && i <= 34)
							{
								itemToFind = i;
							}
							else if(i == 50)
							{
								itemToFind = 0x42;
							}
							else if(i >= 61 && i <= 64)
							{
								itemToFind = i + 24;
							}
							else if(i == 65)
							{
								itemToFind = 0x5F;
							}
							else if(i >= 66 && i <= 72)
							{
								itemToFind = i + 31;
							}
							else if(i >= 73 && i <= 82)
							{
								itemToFind = i + 33;
							}
						}
						for(int t = 0; t < addressArrayLength; t++)
						{
							fseek(fp,AddressArray[t],SEEK_SET);
							n = fread(buffer,sizeof(buffer),1,fp);
							if(buffer[0] == itemToFind)
							{
								randValue = rand() % 8;
								switch(randValue)
								{
									case 0:
										newByte = 0x95;
									break;
									case 1:
										newByte = 0x96;
									break;
									case 2:
										newByte = 0x97;
									break;
									case 3:
										newByte = 0x98;
									break;
									case 4:
										newByte = 0x99;
									break;
									case 5:
										newByte = 0xA2;
									break;
									case 6:
										newByte = 0xA3;
									break;
									case 7:
										newByte = 0xA4;
									break;
									default:
									break;
								}
								fseek(fp,AddressArray[t],SEEK_SET);
								fwrite(&newByte,sizeof(newByte),1,fp);
							}
						}
					}
				}
			}
		}
		void writeHints(int Writeaddress, int WriteitemID, int RandomizedAddressID)
		{
			int lengthStr = 0;
			newByte = 0x0A;
			for(int i = 0; itemNames[WriteitemID][i] != '\0'; i++)
			{
				fseek(fp,Writeaddress + i,SEEK_SET);
				fwrite(&itemNames[WriteitemID][i],sizeof(itemNames[WriteitemID][i]),1,fp);
				lengthStr = i;
			}
			fseek(fp,Writeaddress + lengthStr + 2,SEEK_SET);
			fwrite(&newByte,sizeof(newByte),1,fp);
			for(int i = 0; AddressArrayNames[RandomizedAddressID][i] != '\0'; i++)
			{
				fseek(fp,Writeaddress+lengthStr+3+i,SEEK_SET);
				fwrite(&AddressArrayNames[RandomizedAddressID][i],sizeof(AddressArrayNames[RandomizedAddressID][i]),1,fp);
			}
		}
		_Bool extendHintsBool = false;
		void extendedHints()
		{
			extendHintsBool = true;
			int extendedHintArray[] =
			{
				0x1195850,	0x119588D,
				0x1195800,	0x119584B,
				0x11957B0,	0x11957FC,
				0x1195768,	0x11957AC,
					
				0x1195F68,	0x1195F96,
				0x1195F28,	0x1195F65,
				0x1195EE8,	0x1195F26,
				0x1195EB8,	0x1195EE6,
				0x1195E70,	0x1195EB4,
				0x1195E40,	0x1195E6B,
				0x1195DF8,	0x1195E38,
				0x1195DB8,	0x1195DF2,
				0x1195D78,	0x1195DB1,
				0x1195D38,	0x1195D74,
				0x1195CF8,	0x1195D33,
				0x1195CB0,	0x1195CF1,
				0x1195C68,	0x1195CAB,
				0x1195C30,	0x1195C62,
				0x1195BE8,	0x1195C28,
				0x11959E8,	0x1195A16,
				0x1195B98,	0x1195BE1,
				0x1195B40,	0x1195B8F,
				0x1195B10,	0x1195B37,
				0x1195AC0,	0x1195B0E,
				0x1195A70,	0x1195AB8,
				0x1195A40,	0x1195A6C,
				0x1195A18,	0x1195A3A,
				0x11959C0,	0x11959E6,
				0x1195968,	0x11959B9,
					
					
				0x1195170,	0x11951A7,
				0x1195130,	0x119516B,
					
				0x11950D0,	0x1195115,
				0x1195078,	0x11950CD,
				0x1195020,	0x1195076,
				0x1194F90,	0x1194FD0,
				0x1194F48,	0x1194F8E,
				0x1194F00,	0x1194F43
			};
			int randValue = 0;
			int extendedHintsArrayLength = *(&extendedHintArray + 1) - extendedHintArray;
			newByte = 0x20;
			int SIZE = 1;
			int n;
			unsigned char buffer[SIZE];
			for(int i = 0; i <= extendedHintsArrayLength - 2; i += 2)
			{
				for(int n = 0; n <= (extendedHintArray[i+1] - extendedHintArray[i]); n++)
				{
					fseek(fp,extendedHintArray[i]+n,SEEK_SET);
					fwrite(&newByte,sizeof(newByte),1,fp);
				}
			}
			for(int i = 0; i <= extendedHintsArrayLength - 2; i += 2)
			{
				randValue = rand() % addressArrayLength;
				fseek(fp,AddressArray[randValue],SEEK_SET);
				n = fread(buffer,sizeof(buffer),1,fp);
				writeHints(extendedHintArray[i],buffer[0],randValue);
			}
		}
		void hints()
		{
			/*Blue Orb, Red Orb, Green Orb, Purple Orb, Yellow Orb, 
			  Dragon Crest, Vampire Killer
			  Blue Dragon Key, Red Phoenix Key, Yellow Dragon Key, White Tiger Key, Black Turtle Key
			  Whip of Flames, Whip of Ice
			  Wolf's Foot
			*/
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
				CurtainTimeBellDesc, endCurtainTimeBellDesc 
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
			int n;
			int SIZE = 1;
			newByte = 0x0A;
			unsigned char buffer[SIZE];
			for(int i = 0; i < addressArrayLength; i++)
			{
				fseek(fp,AddressArray[i],SEEK_SET);
				n = fread(buffer,sizeof(buffer),1,fp);
				switch(buffer[0])
				{
					//orb
					case 0x7F: //red orb
						writeHints(ASMLMapDesc, buffer[0], i);
						break;
					case 0x80: //blue orb
						writeHints(HoSRMapDesc, buffer[0], i);						
						break;
					case 0x81: //yellow orb
						writeHints(TheatreMapDesc, buffer[0], i);
						break;
					case 0x82: //green orb
						writeHints(DPoWMapDesc,buffer[0],i);
						break;
					case 0x83: //purple orb
						writeHints(GFbTMapDesc,buffer[0],i);
						break;
					case 0x84: //white orb
						writeHints(AncientText3Desc,buffer[0],i);
						break;
					case 0x85: //black orb
						writeHints(AncientText4Desc,buffer[0],i);
						break;
					//whip
					case 0x05: //vampire killer
						writeHints(UnlockJewelDesc,buffer[0],i);
						break;
					case 0x03: //whip of ice
						writeHints(CurtainTimeBellDesc,buffer[0],i);
						break;
					case 0x02: //whip of flames
						writeHints(ETabletDesc,buffer[0],i);
						break;
					//
					case 0x86: //wolf's foot
						writeHints(AncientText1Desc,buffer[0],i);
						break;
					case 0x42: //Curtain Time Bell
						writeHints(AncientText2Desc,buffer[0],i);
						break;
					//keys
					case 0x59: //White Tiger Key
						writeHints(MarkerStone4Desc,buffer[0],i);
						break;
					case 0x5A: //Blue Dragon Key
						writeHints(MarkerStone2Desc,buffer[0],i);
						break;
					case 0x5B: //Red Phoenix Key
						writeHints(MarkerStone1Desc,buffer[0],i);
						break;
					case 0x5C: //Black Turtle Key
						writeHints(MarkerStone5Desc,buffer[0],i);
						break;					
					case 0x5D: //Yellow Dragon Key
						writeHints(MarkerStone3Desc,buffer[0],i);
						break;
					case 0x56: //vi tablet
						writeHints(MarkerStone6Desc,buffer[0],i);
						break;
					case 0x5E: //dragon crest
						writeHints(MarkerStone7Desc,buffer[0],i);
						break;
					case 0x55: //e tablet
						writeHints(MarkerStone8Desc,buffer[0],i);
						break;
					default:
						break;
				}
			}
			fflush(stdin);
			char choose18;
			do
			{
				if(choose18 == 'Y' || choose18 == 'N')
					break;
				printf("Do you want to add extended random hints? Y/N \n");
				scanf("%c",&choose18);
			}while(choose18 != 'Y' || choose18 != 'N');
			if(choose18 == 'Y')
			{
				extendedHints();
			}
		}
		void switchItemsAround(int AddressofDoorItem, int ItemID, int calls, int doorsLeft[])
		{
			if(calls >= 4)
			{
				return;
			}
			int SIZE = 1;
			unsigned char buffer[SIZE];
			int n; 
			randValue = rand() % 5;
			unsigned char temp;
			//int oldAddress;
			newByte = ItemID;
			int nextDoor = randValue;
			fseek(fp, AddressofDoorItem, SEEK_SET);
			n = fread(buffer, sizeof(buffer), 1, fp);
			temp = buffer[0];
			for(int i = 0; i < addressArrayLength; i++)
			{
				fseek(fp, AddressArray[i], SEEK_SET);
				n = fread(buffer, sizeof(buffer), 1, fp);
				if(buffer[0] == ItemID && (AddressArray[i] != AddressofDoorItem))
				{
					fseek(fp,AddressArray[i],SEEK_SET);
					fwrite(&temp,sizeof(temp),1,fp);
					fseek(fp,AddressofDoorItem,SEEK_SET);
					fwrite(&newByte,sizeof(newByte),1,fp);
				}
				else if ((AddressArray[i] == AddressofDoorItem) && buffer[0] == ItemID)
				{
					for(int t = 0; t < addressArrayLength; t++)
					{
						fseek(fp,AddressArray[t],SEEK_SET);
						n = fread(buffer, sizeof(buffer), 1, fp);
						if(buffer[0] == 0x56 && (AddressArray[t] != AddressofDoorItem))
						{
							fseek(fp,AddressArray[t],SEEK_SET);
							fwrite(&temp,sizeof(temp),1,fp);
							fseek(fp,AddressofDoorItem,SEEK_SET);
							fwrite(&newByte,sizeof(newByte),1,fp);
						}							
					}
				}
			}
			do
			{
				nextDoor = rand() % 5;
			} while(doorsLeft[nextDoor] == 0);
			doorsLeft[nextDoor] = 0;
			int nextItem;
			switch(AddressofDoorItem)
			{
				case 0x0AB0E960:
					nextItem = 0x5B;
					break;
				case 0x14467A60:
					nextItem = 0x5A;
					break;
				case 0x08CFE360:
					nextItem = 0x5D;
					break;
				case 0x13A638E0:
					nextItem = 0x5C;
					break;
				case 0x22EB31E0:
					nextItem = 0x59;
					break;
				default:
					break;
			}
			switch(nextDoor)
			{
				case 0: //red Door
					switchItemsAround(WolfsFoot, nextItem, calls+1, doorsLeft);
					break;
				case 1: //blue door
					switchItemsAround(ToolBag, nextItem, calls+1, doorsLeft);
					break;
				case 2: //yellow door
					switchItemsAround(BlackBishop, nextItem, calls+1, doorsLeft);
					break;
				case 3: //black door
					switchItemsAround(Draupnir, nextItem, calls+1, doorsLeft);
					break;
				case 4: //white door
					switchItemsAround(HeartBrooch, nextItem, calls+1, doorsLeft);
					break;
				default:
					break;
			}
		}
	_Bool OrbandWhipShuffels = false;
	_Bool BlackOrbIncluded = false;
		void OrbandWhipShuffler()
		{
			//_Bool OrbWhipShuffel = false;
			_Bool blackOrbUsed = false;
			//Orb/Whip Shuffel : shuffel the orbs and whips (excluding Vampire Killer and Whip of Alchemy) amongst themselves, 
				//and overwrite the other randomized ones with 'potion'
					//doesn't include white orb (could potentially include black orb)
			int OrbsWhipArray[] = {0x7F, 0x80, 0x81, 0x82, 0x83, 0x00, 0x02, 0x03, 0x04};
			char choose104;
			fflush(stdin);
			do
			{
				if(choose104 == 'Y' || choose104 == 'N')
					break;
				printf("Do you want to include the black orb? Y/N \n");
				scanf("%c",&choose104);
			}while(choose104 != 'Y' || choose104 != 'N');
			if(choose104 == 'Y')
			{
				BlackOrbIncluded = true;
				blackOrbUsed = true;
				OrbsWhipArray[5] = 0x85;
			}
			int OrbsWhipArrayLength = *(&OrbsWhipArray + 1) - OrbsWhipArray;
			const int startOrbsWhipsTable = 0x0044F590;
			//const int endOrbsWhipsTable = 0x0044F5BF;
			newByte = 0x01;
			for(int i = 0; i < 12; i++)
			{
				if(((i < 5 || i > 7) || (i == 6 && blackOrbUsed)) && (i != 11))
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
			int SIZE = 1;
			unsigned char buffer[SIZE];
			int n;
			for(int i = 0; i < addressArrayLength; i++)
			{
				fseek(fp, AddressArray[i], SEEK_SET);
				n = fread(buffer, sizeof(buffer), 1, fp);
				if(buffer[0] == 0x02 || buffer[0] == 0x03 || buffer[0] == 0x04 || buffer[0] == 0x7F || buffer[0] == 0x80 || buffer[0] == 0x81
					|| buffer[0] == 0x82 || buffer[0] == 0x83 || (blackOrbUsed && buffer[0] == 0x85))
				{
					fseek(fp, AddressArray[i], SEEK_SET);
					newByte = 0x2A;
					fwrite(&newByte,sizeof(newByte),1,fp);
				}
			}
		}
		_Bool DragonCrestbehindKeybool = false;
		void DragonCrestbehindKeys()
		{
			DragonCrestbehindKeybool = true;
			randValue = rand() % 5;
			//int callsFirst = 0;
			int doors[5] = {1,2,3,4,5};
			doors[randValue] = 0;
			switch(randValue)
			{
				case 0: //red Door
					switchItemsAround(WolfsFoot, 0x5E, 0, doors);
					break;
				case 1: //blue door
					switchItemsAround(ToolBag, 0x5E, 0, doors);
					break;
				case 2: //yellow door
					switchItemsAround(BlackBishop, 0x5E, 0, doors);
					break;
				case 3: //black door
					switchItemsAround(Draupnir, 0x5E, 0, doors);
					break;
				case 4: //white door
					switchItemsAround(HeartBrooch, 0x5E, 0, doors);
					break;
				default:
					break;
			}
		}
		int PowerUpsChangesint = 0; 
		void PowerUpsChanged(int type)
		{
			_Bool RepeatablePowerUp = true;
			_Bool ALLPowerUps = true;
			_Bool toPotion = true;
			if(type == 1)
				ALLPowerUps = false;
			if(type == 2)
				RepeatablePowerUp = false;
			fflush(stdin);
			char choose14;
			do
			{
				if(choose14 == 'P' || choose14 == 'I')
					break;
				printf("Press P to make all power ups into Potions \n");
				printf("Press I to change HPups into Potions, MPups into Mana Prisms, and Heartups into Heart Repairs \n"); 
				scanf("%c",&choose14);
			}while (choose14 != 'P' || choose14 != 'I');
			if(choose14 == 'I')
			{
				toPotion = false;
			}
			if(RepeatablePowerUp == false)
			{
				PowerUpsChangesint = 2;
				if(!toPotion)
					PowerUpsChangesint += 3;
				int SIZE = 1;
				unsigned char buffer[SIZE];
				int n;
				newByte = 0x2A;
				for(int i = addressArrayLength-1; i >= 111; i--)
				{
					fseek(fp, AddressArray[i], SEEK_SET);
					n = fread(buffer, sizeof(buffer), 1, fp);
					if(buffer[0] == 0x90 || buffer[0] == 0x91 || buffer[0] == 0x92)
					{
						fseek(fp, AddressArray[i], SEEK_SET);
						newByte = 0x2A;
						if(!toPotion)
						{
							switch(buffer[0])
							{
								case 0x90:
								newByte = 0x2A;
								break;
								case 0x91:
								newByte = 0x2D;
								break;
								case 0x92:
								newByte = 0x2E;
								break;
								default:
								break;
							}
						}
						fwrite(&newByte,sizeof(newByte),1,fp);
					}
				}
			}
			//Turn off ALL Power Ups : Any Power up that is shuffeled is overwitten with 'Potion'
			if(ALLPowerUps == false)
			{
				PowerUpsChangesint = 1;
				if(!toPotion)
					PowerUpsChangesint += 3;
				int SIZE = 1;
				unsigned char buffer[SIZE];
				int n;
				newByte = 0x2A;
				for(int i = addressArrayLength-1; i >= 0; i--)
				{
					fseek(fp, AddressArray[i], SEEK_SET);
					n = fread(buffer, sizeof(buffer), 1, fp);
					if(buffer[0] == 0x90 || buffer[0] == 0x91 || buffer[0] == 0x92)
					{
						fseek(fp, AddressArray[i], SEEK_SET);
						newByte = 0x2A;
						if(!toPotion)
						{
							switch(buffer[0])
							{
								case 0x90:
								newByte = 0x2A;
								break;
								case 0x91:
								newByte = 0x2D;
								break;
								case 0x92:
								newByte = 0x2E;
								break;
								default:
								break;
							}
						}
						fwrite(&newByte,sizeof(newByte),1,fp);
					}
				}
			}
		}
		char modelReplaced = 'U';
		void ReplaceModelsfunc()
		{
			modelReplaced = 'S';
			int ModelsArray[] =
			{
				//relics
				0x003DC764,
				0x003DC7AC,
				0x003DC7F4,
				0x003DC83C,
				0x003DC884,
				0x003DC8CC,
				0x003DC914,
				0x003DC95C,
				0x003DC9A4,
				0x003DC9EC,
				//Accessories
				0x003D9BAC,
				0x003D9C30,
				0x003D9CB4,
				0x003D9D38,
				0x003D9DBC,
				0x003D9E40,
				0x003D9EC4,
				0x003D9F48,
				0x003D9FCC,
				0x003DA050,
				0x003DA0D4,
				0x003DA158,
				0x003DA1DC,
				0x003DA260,
				0x003DA2E4,
				0x003DA368,
				0x003DA3EC,
				0x003DA470,
				0x003DA4F4,
				0x003DA578,
				0x003DA5FC,
				0x003DA680,
				0x003DA704,
				0x003DA788,
				0x003DA80C,
				//items
				0x003DAC2C,
				0x003DAC7C,
				0x003DACCC,
				0x003DAD1C,
				0x003DAD6C,
				0x003DADBC,
				0x003DAE0C,
				0x003DAE5C,
				0x003DAEAC,
				0x003DAEFC,
				0x003DAF4C,
				0x003DAF9C,
				0x003DAFEC,
				0x003DB03C,
				0x003DB08C,
				0x003DB0DC,
				0x003DB12C,
				0x003DB17C,
				0x003DB1CC,
				0x003DB21C,
				0x003DB26C,
				0x003DB2BC,
				0x003DB30C,
				0x003DB35C,
				0x003DB3AC,
				0x003DB3FC,
				0x003DB44C,
				0x003DB49C,
				0x003DB4EC,
				0x003DB53C,
				0x003DB58C,
				0x003DB5DC,
				0x003DB62C,
				0x003DB67C,
				0x003DB6CC,
				//orbs
				0x003DC56C,
				0x003DC5B4,
				0x003DC5FC,
				0x003DC644,
				0x003DC68C,
				0x003DC6D4,
				0x003DC71C,
				//money
				0x003DCB9C,
				0x003DCBE4,
				0x003DCC2C,
				0x003DCC74,
				0x003DCCBC,
				0x003DCD04,
				0x003DCD4C,
				0x003DCD94,
				0x003DCDDC,
				0x003DCE24,
				0x003DCE6C,
				0x003DCEB4,
				0x003DCEFC,
				0x003DCF44,
				0x003DCF8C,
				0x003DCFD4,
				//event items
				0x003DB99C,
				0x003DB9E4,
				0x003DBA2C,
				0x003DBA74,
				0x003DBABC,
				0x003DBB04,
				0x003DBB4C,
				0x003DBB94,
				0x003DBBDC,
				0x003DBC24,
				0x003DBC6C,
				0x003DBCB4,
				0x003DBCFC,
				0x003DBD44,
				0x003DBD8C,
				0x003DBDD4,
				0x003DBE1C,
				0x003DBE64,
				0x003DBEAC,
				0x003DBEF4,
				0x003DBF3C,
				0x003DBF84,
				0x003DBFCC,
				0x003DC014,
				0x003DC05C,
				0x003DC0A4,
				0x003DC0EC,
				0x003DC134,
				0x003DC17C,
				0x003DC1C4,
				0x003DC20C,
				//sub-weapons
				0x003DD184,
				0x003DD1CC,
				0x003DD214,
				0x003DD25C,
				0x003DD2A4,
				0x003DD2EC,
				//hearts
				0x003DCB0C,
				0x003DCB54,
				//power ups
				0x003DCA34,
				0x003DCA7C,
				0x003DCAC4,
				//armors
				0x003D999C,
				0x003D9A20,
				0x003D9AA4,
				0x003D9B28
			};
			int modelArrayLength = *(&ModelsArray + 1) - ModelsArray;
			fflush(stdin);
			char choose16;
			do
			{
				if(choose16 == 'R' || choose16 == 'S')
					break;
				printf("Do you want random models (R) or all the same (S) ? \n");
				scanf("%c",&choose16);
			}while (choose16 != 'R' || choose16 != 'S');
			_Bool setRand = false;
			if(choose16 == 'R')
			{
				modelReplaced = 'R';
				setRand = true;
			}
			newByte = 0x21;
			unsigned char modelUsed = 0x28;
			for(int i = 0; i < modelArrayLength; i++)
			{ 
				fseek(fp,ModelsArray[i],SEEK_SET);
				if(setRand)
				{
					randValue = (rand() % 53) + 1;
					if(randValue != 30)
					{
						newByte = randValue;
					}
					fwrite(&newByte,sizeof(newByte),1,fp);
				}
				else
				{
					fwrite(&modelUsed,sizeof(modelUsed),1,fp);
				}
			}
		}
		void MPCostRandomization()
		{
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
		void HeartCostRandomization()
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
			int HeartCostsLength = *(&HeartCosts + 1) - HeartCosts;
			for(int i = 0; i < HeartCostsLength; i++)
			{
				randValue = rand() % 0x3;
				fseek(fp,HeartCosts[i]+1,SEEK_SET);
				newByte = randValue + 0xC0;
				fwrite(&newByte,sizeof(newByte),1,fp);
				randValue = rand() % 0xFF;
				newByte = randValue;
				fseek(fp,HeartCosts[i],SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
			}				
		}
		void modeSelection()
		{
			char choose9;
			char choose10;
			fflush(stdin);
			do
			{
				if(choose9 == 'Y' || choose9 == 'N')
					break;
				printf("do you want to change the powerups? Y/N \n");
				scanf("%c",&choose9);
			}while (choose9 != 'Y' || choose9 != 'N');
			if(choose9 == 'Y')
			{
				fflush(stdin);
				char choose39;
				do{
					if(choose39 == 'Y' || choose39 == 'N')
						break;
					printf("Change how much of an increase power-ups are? Y/N \n");
					scanf("%c",&choose39);
				}while(choose39 != 'Y' || choose39 != 'N');
				if(choose39 == 'Y')
				{
					const int HPUpIncreaseAddress = 0x3DCA4E;
					const int MPUpIncreaseAddress = 0x3DCADE;
					const int HeartUpIncreaseAddress = 0x3DCA96;
					fflush(stdin);
					char choose40;
					do
					{
						if(choose40 == 'R' || choose40 == 'N' || choose40 == 'Z')
							break;
						printf("Do you want them Random '8-31' (R), Negative 1 '-1' (N), or Zero '0' (Z) \n");
						scanf("%c",&choose40);
					}while(choose40 != 'R' || choose40 != 'N' || choose40 != 'Z'); 
					if(choose40 == 'R')
					{
						fflush(stdin);
						char choose41;
						do
						{
							if(choose41 == 'S' || choose41 == 'D')
								break;
							printf("Do you want them to have the same increases (S) or different increases (D)? \n");
							scanf("%c",&choose41);
						}while(choose41 != 'S' || choose41 != 'D');
						if(choose41 == 'S')
						{
							randValue = rand() % 0xF9;
							newByte = randValue;
							fseek(fp,HPUpIncreaseAddress,SEEK_SET);
							fwrite(&newByte,sizeof(newByte),1,fp);
							fseek(fp,HeartUpIncreaseAddress,SEEK_SET);
							fwrite(&newByte,sizeof(newByte),1,fp);
							fseek(fp,MPUpIncreaseAddress,SEEK_SET);
							fwrite(&newByte,sizeof(newByte),1,fp);						
						}
						else if(choose41 == 'D')
						{
							randValue = rand() % 0xF9;
							newByte = randValue;
							fseek(fp,HPUpIncreaseAddress,SEEK_SET);
							fwrite(&newByte,sizeof(newByte),1,fp);
							
							randValue = rand() % 0xF9;
							newByte = randValue;
							fseek(fp,HeartUpIncreaseAddress,SEEK_SET);
							fwrite(&newByte,sizeof(newByte),1,fp);
							
							randValue = rand() % 0xF9;
							newByte = randValue;
							fseek(fp,MPUpIncreaseAddress,SEEK_SET);
							fwrite(&newByte,sizeof(newByte),1,fp);
						}
						
					}
					else if(choose40 == 'N')
					{
						newByte = 0x80;
						fseek(fp,HPUpIncreaseAddress,SEEK_SET);
						fwrite(&newByte,sizeof(newByte),1,fp);
						fseek(fp,HeartUpIncreaseAddress,SEEK_SET);
						fwrite(&newByte,sizeof(newByte),1,fp);
						fseek(fp,MPUpIncreaseAddress,SEEK_SET);
						fwrite(&newByte,sizeof(newByte),1,fp);
						
						newByte = 0xBF;
						fseek(fp,HPUpIncreaseAddress+1,SEEK_SET);
						fwrite(&newByte,sizeof(newByte),1,fp);	
						fseek(fp,HeartUpIncreaseAddress+1,SEEK_SET);
						fwrite(&newByte,sizeof(newByte),1,fp);
						fseek(fp,MPUpIncreaseAddress+1,SEEK_SET);
						fwrite(&newByte,sizeof(newByte),1,fp);
						
					}
					else if(choose40 == 'Z')
					{
						newByte = 0x00;
						fseek(fp,HPUpIncreaseAddress,SEEK_SET);
						fwrite(&newByte,sizeof(newByte),1,fp);
						fseek(fp,HPUpIncreaseAddress+1,SEEK_SET);
						fwrite(&newByte,sizeof(newByte),1,fp);
						fseek(fp,HeartUpIncreaseAddress,SEEK_SET);
						fwrite(&newByte,sizeof(newByte),1,fp);
						fseek(fp,HeartUpIncreaseAddress+1,SEEK_SET);
						fwrite(&newByte,sizeof(newByte),1,fp);
						fseek(fp,MPUpIncreaseAddress,SEEK_SET);
						fwrite(&newByte,sizeof(newByte),1,fp);
						fseek(fp,MPUpIncreaseAddress+1,SEEK_SET);
						fwrite(&newByte,sizeof(newByte),1,fp);
					}
				}
				fflush(stdin);
				do
				{
					if(choose10 == 'A' || choose10 == 'R' || choose10 == 'N')
						break;
					printf("(A) for All powerups; (R) for Repeatable powerups; (N) for don't get rid of power ups. \n");
					scanf("%c",&choose10);
				}while(choose10 != 'A' || choose10 != 'R' || choose10 != 'N');
			}
			if(choose10 == 'A')
				PowerUpsChanged(1);
			if(choose10 == 'R')
				PowerUpsChanged(2);
			fflush(stdin);
			char choose44;
			do
			{	
				if(choose44 == 'Y' || choose44 == 'N')
					break;
				printf("Do you want the food items to have a chance to heal MP or Hearts instead of HP? Y/N \n");
				scanf("%c",&choose44);
			}while(choose44 != 'Y' || choose44 != 'N');
			if(choose44 == 'Y')
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
				int foodItemTypeLen = *(&foodItemType + 1) - foodItemType;
				
				for(int i = 0; i < foodItemTypeLen; i++)
				{
					fseek(fp,foodItemType[i],SEEK_SET);
					randValue = rand() % 3;
					randValue += 1;
					newByte = randValue;
					fwrite(&newByte,sizeof(newByte),1,fp);
				}
			}
			fflush(stdin);
			char choose42;
			do
			{	
				if(choose42 == 'Y' || choose42 == 'N')
					break;
				printf("Change the starting Max HP? Y/N \n");
				scanf("%c",&choose42);
			}while(choose42 != 'Y' || choose42 != 'N');
			if(choose42 == 'Y')
			{
				int startMaxHPAddress = 0x3E1E20;
				randValue = rand() % 0x3;
				randValue += 0x41;
				newByte = randValue;
				fseek(fp,startMaxHPAddress+3,SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
				randValue = rand() % 0x100;
				newByte = randValue;
				fseek(fp,startMaxHPAddress+2,SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
				randValue = rand() % 0x100;
				newByte = randValue;				
				fseek(fp,startMaxHPAddress+1,SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
				randValue = rand() % 0x100;
				newByte = randValue;				
				fseek(fp,startMaxHPAddress,SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
			}
			fflush(stdin);
			char choose34;
			do
			{
				if(choose34 == 'Y' || choose34 == 'N')
					break;
				printf("are you intending to play on Crazy Mode? Y/N \n");
				printf("The goal of this is to make the whip / orb that appears after the boss one that 'doesn't' give extra benefit but will still allow you to leave the room due to the reduced carry capacity \n");
				printf("Note: the checker in this program is not made with Crazy mode as an option \n");
				scanf("%c",&choose34);
			}while(choose34 != 'Y' || choose34 != 'N');
			if(choose34 == 'Y')
			{
				_Bool crazyPlayer = true;
				changeORBSandWHIPS(crazyPlayer);
			}
			fflush(stdin);
			char choose17;
			do
			{
				if(choose17 == 'Y' || choose17 == 'N')
					break;
				printf("do you want to replace the item models in the game? Y/N \n");
				scanf("%c",&choose17);
			}while(choose17 != 'Y' || choose17 != 'N');
			if(choose17 == 'Y')
			{
				ReplaceModelsfunc();
			}
			char choose12;
			fflush(stdin);
			do
			{
				if(choose12 == 'Y' || choose12 == 'N')
					break;
				printf("Place the Dragon Crest behind all the key doors? Y/N \n");
				scanf("%c",&choose12);
			} while (choose12 != 'Y' || choose12 != 'N');
			if(choose12 == 'Y')
			{
				DragonCrestbehindKeys();
			}
			char choose13;
			fflush(stdin);
			do
			{
				if(choose13 == 'Y' || choose13 == 'N')
					break;
				printf("Turn on Orb and Whip Shuffle? Y/N \n[This Replaces where they were randomized to, to start with, with Potions] \n[This refers to having a non-whip of alchemy spawn after the boss fights] \n");
				scanf("%c",&choose13);
			} while (choose13 != 'Y' || choose13 != 'N');
			if(choose13 == 'Y')
			{
				OrbandWhipShuffels = true;
				OrbandWhipShuffler();
			}
			char choose19;
			fflush(stdin);
			do
			{
				if(choose19 == 'Y' || choose19 == 'N')
					break;
				printf("Do you want to activate the store? Y/N \n");
				scanf("%c",&choose19);
			}while(choose19 != 'Y' || choose19 != 'N');
			if(choose19 == 'Y')
			{
				ActivateShop();
			}
			fflush(stdin);
			char choose23;
			do
			{
				if(choose23 == 'Y' || choose23 == 'N')
					break;
				printf("Randomize MP costs? Y/N \n");
				scanf("%c",&choose23);
			}while(choose23 != 'Y' || choose23 != 'N');
			if(choose23 == 'Y')
			{
				MPCostRandomization();
			}
			fflush(stdin);
			char choose25;
			do
			{
				if(choose25 == 'Y' || choose25 == 'N')
					break;
				printf("Do you want to start with a random sub-weapon? Y/N \n");
				scanf("%c",&choose25);
			}while(choose25 != 'Y' || choose25 != 'N');
			if(choose25 == 'Y')
			{
				randValue = rand() % 5;
				newByte = randValue + 0xAA;
				int startingSubWeapon = 0x3E1E4C;
				fseek(fp,startingSubWeapon,SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
			}
			fflush(stdin);
			char choose26;
			do
			{
				if (choose26 == 'Y' || choose26 == 'N')
					break;
				printf("Do you want to randomize Sub-weapon heart costs? Y/N \n");
				scanf("%c",&choose26);
			}while(choose26 != 'Y' || choose26 != 'N');
			if(choose26 == 'Y')
			{
				HeartCostRandomization();
			}
			fflush(stdin);
			char choose38;
			do
			{
				if(choose38 == 'Y' || choose38 == 'N')
					break;
				printf("Do you want to randomize the starting Max Hearts value Y/N \n");
				scanf("%c",&choose38);
			}while(choose38 != 'Y' || choose38 != 'N');
			if(choose38 == 'Y')
			{
				const int startingMaxHearts = 0x3E1E24;
				fseek(fp, startingMaxHearts+2, SEEK_SET);
				randValue = rand() % 0x100;
				newByte = randValue;
				fwrite(&newByte,sizeof(newByte),1,fp);
			}
			fflush(stdin);
			char choose27;
			do
			{
				if(choose27 == 'Y' || choose27 == 'N')
					break;
				printf("Do you want to have hearts at the start of the game? Y/N \n");
				scanf("%c",&choose27);
			}while(choose27 != 'Y' || choose27 != 'N');
			if(choose27 == 'Y')
			{
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
			}
			fflush(stdin);
			char choose28;
			do
			{
				if(choose28 == 'Y' || choose28 == 'N')
					break;
				printf("Do you want to start with Gold? Y/N \n");
				scanf("%c",&choose28);
			}while(choose28 != 'Y' || choose28 != 'N');
			if(choose28 == 'Y')
			{
				const int StartingGoldAddress = 0x3E1E40;
				randValue = rand() % 0xFF;
				randValue += 1;
				newByte = randValue;
				fseek(fp,StartingGoldAddress,SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
				randValue = rand() % 0x100;
				newByte = randValue;
				fseek(fp,StartingGoldAddress+1,SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
				randValue = rand() % 0x0E;
				newByte = randValue;
				fseek(fp,StartingGoldAddress+2,SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
			}
			fflush(stdin);
			char choose30;
			do
			{
				if(choose30 == 'Y' || choose30 == 'N')
					break;
				printf("Do you want randomized Max MP to start? Y/N \n");
				scanf("%c",&choose30);
			}while(choose30 != 'Y' || choose30 != 'N');
			if(choose30 == 'Y')
			{
				const int startingMaxMP = 0x3E1E64;
				randValue = rand() % 0x100;
				newByte = randValue + 20;
				fseek(fp,startingMaxMP,SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
			}
			fflush(stdin);
			char choose29;
			do
			{
				if(choose29 == 'Y' || choose29 == 'N')
					break;
				printf("Do you want to start with MP? Y/N \n");
				scanf("%c",&choose29);
			}while(choose29 != 'Y' || choose29 != 'N');
			if (choose29 == 'Y')
			{
				const int startingMP = 0x3E1E68;
				randValue = rand() % 0x100;
				newByte = randValue;
				fseek(fp,startingMP,SEEK_SET);
				fwrite(&newByte,sizeof(newByte),1,fp);
			}
			if(!OrbandWhipShuffels)
			{
				fflush(stdin);
				char choose31;
				do
				{
					if(choose31 == 'Y' || choose31 == 'N')
						break;
					printf("Make Forgotten One have a copy of Vampire Killer? Y/N \n");
					scanf("%c",&choose31);
				}while(choose31 != 'Y' || choose31 != 'N');
				if(choose31 == 'Y')
				{
					const int ForgottenOneOrb = 0x44F5A8;
					newByte = 0x05;
					fseek(fp,ForgottenOneOrb,SEEK_SET);
					fwrite(&newByte,sizeof(newByte),1,fp);
				}
			}
			fflush(stdin);
			char choose32;
			do
			{
				if(choose32 == 'Y' || choose32 == 'N')
					break;
				printf("Do you want to have a random orb equipped at the start? Y/N \n");
				printf("If you have one equipped you will not be able to re-equip it after removing it until you find a new copy of it \n");
				scanf("%c",&choose32);
			}while(choose32 != 'Y' || choose32 != 'N');
			if(choose32 == 'Y')
			{
				const int startingOrb = 0x3E1E50;
				randValue = rand() % 8;
				newByte = randValue + 0x7F;
				if(newByte != (7+0x7F))
				{
					fseek(fp,startingOrb,SEEK_SET);
					fwrite(&newByte,sizeof(newByte),1,fp);
				}
			}
			fflush(stdin);
			char choose20;
			do
			{
				if(choose20 == 'Y' || choose20 == 'N')
					break;
				printf("Have Wolf's Foot in shop for $-1 Y/N \n");
				scanf("%c",&choose20);
			}while(choose20 != 'Y' || choose20 != 'N');
			if(choose20 == 'Y')
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
			}
			fflush(stdin);
			char choose21;
			do
			{
				if(choose21 == 'Y' || choose21 == 'N')
					break;
				printf("Make the Wolf's foot drain MP slowly? Y/N \n");
				scanf("%c",&choose21);
			}while(choose21 != 'Y' || choose21 != 'N');
			if(choose21 == 'Y')
			{
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
			fflush(stdin);
			char choose35;
			do			
			{
				if(choose35 == 'Y' || choose35 == 'N')
					break;
				printf("Do you want to turn on Boss Load Zone Shuffler? Y/N \n");
				printf("Note: Undead Parasite's Boss Room acts a bit weirdly \n");
				printf("recommendation: don't use this in combination with the orb and whip shuffle \n");
				scanf("%c",&choose35);
			}while(choose35 != 'Y' || choose35 != 'N');
			if(choose35 == 'Y')
			{
				//TODO: Boss Zone Shuffler
				struct BossData {
					int EntranceAddress;
					int ExitAddress;
					int EntranceData0;
					int EntranceData2;
					int ExitData0;
					int ExitData2;
					char used;
				};
				int BossEntranceAddress[] = {
					0x82C30F0,
					0xB147570,
					0xD5410F0,
					0x13551370,
					0x133F0270,
					0x19B5A1F0,
					0x1A05E870,
					0x237F94F0
				};
				int BossExitAdress[] = {
					0x2182CF0,
					0xB480070,
					0xA865DF0,
					0x1188D3F0,
					0xEDB7570,
					0x19D54DF0,
					0x19EE0570,
					0x23672EF0,
				};
				int BossEntranceData[] = {
					0x00, 0x00, 0x48, 0x00,
					0x01, 0x00, 0x16, 0x00,
					0x01, 0x00, 0x0E, 0x00,
					0x02, 0x00, 0x33, 0x00,
					0x02, 0x00, 0x2C, 0x00,
					0x03, 0x00, 0x37, 0x00,
					0x03, 0x00, 0x3C, 0x00,
					0x08, 0x00, 0x12, 0x00
				};
				int BossExitData[] = {
					0x00, 0x00, 0x3E, 0x00,
					0x01, 0x00, 0x14, 0x00,
					0x01, 0x00, 0x2B, 0x00,
					0x02, 0x00, 0x32, 0x00,
					0x02, 0x00, 0x31, 0x00,
					0x03, 0x00, 0x38, 0x00,
					0x03, 0x00, 0x3B, 0x00,
					0x08, 0x00, 0x11, 0x00
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
				}
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
					
					/*
					fseek(fp,arr_BossData[randValue].ExitAddress,SEEK_SET);
					fwrite(&arr_BossData[i].ExitData0,sizeof(arr_BossData[i].ExitData0),1,fp);
					fseek(fp,arr_BossData[randValue].ExitAddress+2,SEEK_SET);
					fwrite(&arr_BossData[i].ExitData2,sizeof(arr_BossData[i].ExitData2),1,fp);
					*/
					
					arr_BossData[randValue].used = 'U';
				}
			}
			fflush(stdin);
			char choose45;
			do
			{
				if(choose45 == 'Y' || choose45 == 'N')
					break;
				printf("Do you want the bosses to have random between half, normal, or triple HP ? Y/N \n");
				scanf("%c",&choose45);
			}while(choose45 != 'Y' || choose45 != 'N');
			if(choose45 == 'Y')
			{
				const int GolemHP = 0x006FC998 - 0x04;
				const int FlameElementalHP = 0x006EFA98 - 0x04;
				//const int MedusaDrop = 0x006FD868; 
				//const int ThunderElementalDrop = 0x007767E8; 
				const int UndeadParasiteHP = 0x006EC898 - 0x04;
				//const int BlueDoppelDrop
				const int FrostElementalHP = 0x006F0818 - 0x04;
				const int JoachimHP = 0x006FCE98 - 0x04;
				const int SuccubusHP = 0x00776198 - 0x04;
				
				//Golem, Joachim, Frost Elemental, Flame Elemental: 1500 : 0x0080BB44; 4500: 0x00A08C45; 750: 0x00803B44;
				//Succubus: 1000 : 0x00007A44; 3000: 0x00803B45; 500: 0x0000FA43;
				//Undead Parasite: 1200: 0x00009644; 3600: 0x00006145; 600: 0x00001644;
				
				//Golem
				randValue = rand() % 3;
				if(randValue == 1)
				{
					//triple
					const int golemName = 0x006FC998;
					int golemHP = golemName - 4;
					fseek(fp,golemHP+1,SEEK_SET);
					newByte = 0xA0;
					fwrite(&newByte,sizeof(newByte),1,fp);	
					fseek(fp,golemHP+2,SEEK_SET);
					newByte = 0x8C;
					fwrite(&newByte,sizeof(newByte),1,fp);
					fseek(fp,golemHP+3,SEEK_SET);
					newByte = 0x45;
					fwrite(&newByte,sizeof(newByte),1,fp);	
				}
				else if(randValue == 2)
				{
					//half
					const int golemName = 0x006FC998;
					int golemHP = golemName - 4;
					fseek(fp,golemHP+1,SEEK_SET);
					newByte = 0x80;
					fwrite(&newByte,sizeof(newByte),1,fp);	
					fseek(fp,golemHP+2,SEEK_SET);
					newByte = 0x3B;
					fwrite(&newByte,sizeof(newByte),1,fp);
					fseek(fp,golemHP+3,SEEK_SET);
					newByte = 0x44;
					fwrite(&newByte,sizeof(newByte),1,fp);	
				}
				
				//Joachim
				randValue = rand() % 3;
				if(randValue == 1)
				{
					fseek(fp,JoachimHP+1,SEEK_SET);
					newByte = 0xA0;
					fwrite(&newByte,sizeof(newByte),1,fp);	
					fseek(fp,JoachimHP+2,SEEK_SET);
					newByte = 0x8C;
					fwrite(&newByte,sizeof(newByte),1,fp);
					fseek(fp,JoachimHP+3,SEEK_SET);
					newByte = 0x45;
					fwrite(&newByte,sizeof(newByte),1,fp);	
					
				}
				else if(randValue == 2) 
				{
					fseek(fp,JoachimHP+1,SEEK_SET);
					newByte = 0x80;
					fwrite(&newByte,sizeof(newByte),1,fp);	
					fseek(fp,JoachimHP+2,SEEK_SET);
					newByte = 0x3B;
					fwrite(&newByte,sizeof(newByte),1,fp);
					fseek(fp,JoachimHP+3,SEEK_SET);
					newByte = 0x44;
					fwrite(&newByte,sizeof(newByte),1,fp);	
				}
				
				//Frost Elemental
				randValue = rand() % 3;
				if(randValue == 1)
				{
					fseek(fp,FrostElementalHP+1,SEEK_SET);
					newByte = 0xA0;
					fwrite(&newByte,sizeof(newByte),1,fp);	
					fseek(fp,FrostElementalHP+2,SEEK_SET);
					newByte = 0x8C;
					fwrite(&newByte,sizeof(newByte),1,fp);
					fseek(fp,FrostElementalHP+3,SEEK_SET);
					newByte = 0x45;
					fwrite(&newByte,sizeof(newByte),1,fp);	
				}
				else if(randValue == 2)
				{
					fseek(fp,FrostElementalHP+1,SEEK_SET);
					newByte = 0x80;
					fwrite(&newByte,sizeof(newByte),1,fp);	
					fseek(fp,FrostElementalHP+2,SEEK_SET);
					newByte = 0x3B;
					fwrite(&newByte,sizeof(newByte),1,fp);
					fseek(fp,FrostElementalHP+3,SEEK_SET);
					newByte = 0x44;
					fwrite(&newByte,sizeof(newByte),1,fp);	
				}
				
				//Flame Elemental
				randValue = rand() % 3;
				if(randValue == 1)
				{
					fseek(fp,FlameElementalHP+1,SEEK_SET);
					newByte = 0xA0;
					fwrite(&newByte,sizeof(newByte),1,fp);	
					fseek(fp,FlameElementalHP+2,SEEK_SET);
					newByte = 0x8C;
					fwrite(&newByte,sizeof(newByte),1,fp);
					fseek(fp,FlameElementalHP+3,SEEK_SET);
					newByte = 0x45;
					fwrite(&newByte,sizeof(newByte),1,fp);	
				}
				else if(randValue == 2)
				{
					fseek(fp,FlameElementalHP+1,SEEK_SET);
					newByte = 0x80;
					fwrite(&newByte,sizeof(newByte),1,fp);	
					fseek(fp,FlameElementalHP+2,SEEK_SET);
					newByte = 0x3B;
					fwrite(&newByte,sizeof(newByte),1,fp);
					fseek(fp,FlameElementalHP+3,SEEK_SET);
					newByte = 0x44;
					fwrite(&newByte,sizeof(newByte),1,fp);	
				}
				
				//succubus
				randValue = rand() % 3;
				if(randValue == 1)
				{
					fseek(fp,SuccubusHP+1,SEEK_SET);
					newByte = 0x80;
					fwrite(&newByte,sizeof(newByte),1,fp);	
					fseek(fp,SuccubusHP+2,SEEK_SET);
					newByte = 0x3B;
					fwrite(&newByte,sizeof(newByte),1,fp);
					fseek(fp,SuccubusHP+3,SEEK_SET);
					newByte = 0x45;
					fwrite(&newByte,sizeof(newByte),1,fp);	
					
				}
				else if(randValue == 2) 
				{
					fseek(fp,SuccubusHP+1,SEEK_SET);
					newByte = 0x00;
					fwrite(&newByte,sizeof(newByte),1,fp);	
					fseek(fp,SuccubusHP+2,SEEK_SET);
					newByte = 0xFA;
					fwrite(&newByte,sizeof(newByte),1,fp);
					fseek(fp,SuccubusHP+3,SEEK_SET);
					newByte = 0x43;
					fwrite(&newByte,sizeof(newByte),1,fp);	
				}
				
				//undead parasite
				randValue = rand() % 3;
				if(randValue == 1)
				{
					fseek(fp,UndeadParasiteHP+1,SEEK_SET);
					newByte = 0x00;
					fwrite(&newByte,sizeof(newByte),1,fp);	
					fseek(fp,UndeadParasiteHP+2,SEEK_SET);
					newByte = 0x61;
					fwrite(&newByte,sizeof(newByte),1,fp);
					fseek(fp,UndeadParasiteHP+3,SEEK_SET);
					newByte = 0x45;
					fwrite(&newByte,sizeof(newByte),1,fp);	
					
				}
				else if(randValue == 2) 
				{
					fseek(fp,UndeadParasiteHP+1,SEEK_SET);
					newByte = 0x00;
					fwrite(&newByte,sizeof(newByte),1,fp);	
					fseek(fp,UndeadParasiteHP+2,SEEK_SET);
					newByte = 0x16;
					fwrite(&newByte,sizeof(newByte),1,fp);
					fseek(fp,UndeadParasiteHP+3,SEEK_SET);
					newByte = 0x44;
					fwrite(&newByte,sizeof(newByte),1,fp);	
				}
			}
			fflush(stdin);
			char choose43;
			do
			{
				if(choose43 == 'Y' || choose43 == 'N')
					break;
				printf("Do you want to replace some of the standard sub-weapons with their Pumpkin versions? Y/N \n");
				scanf("%c",&choose43);
			}while(choose43 != 'Y' || choose43 != 'N');
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
						//0x411A20,
						0x411A4C,
						0x411A78,
						0x411AA4,
						0x411AD0
					};
					int subWeaponAbilityIDs[] = {
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
						//0x2B,
						0x2C,
						0x2D,
						0x2E,
						0x2F
					};
					int subWeaponAbilityAddressLength = *(&subWeaponAbilityAddresses + 1) - subWeaponAbilityAddresses;
					int iteration = 0;
			if(choose43 == 'Y')
			{
				for(int i = 0; i < subWeaponAbilityAddressLength; i++)
				{
					if(i == 0 || i == 5 || i == 6 || i == 7 || i == 25 || i == 34)
					{
						fseek(fp,subWeaponAbilityAddresses[i],SEEK_SET);
						newByte = 0x28 + iteration;
						switch(i)
						{
							case 0:
							newByte = 0x28;
							break;
							case 5:
							newByte = 0x2D;
							break;
							case 6:
							newByte = 0x2E;
							break;
							case 7:
							newByte = 0x2F;
							break;
							case 25:
							newByte = 0x2A;
							break;
							case 34:
							newByte = 0x2B;
							break;
							default:
							break;
						}
						fwrite(&newByte,sizeof(newByte),1,fp);
					}
				}
				fseek(fp,0x4114CC,SEEK_SET);
				newByte = 0x2C;
				fwrite(&newByte,sizeof(newByte),1,fp);
			}
			if(choose43 == 'N')
			{
				fflush(stdin);
				char choose36;
				do
				{
					if(choose36 == 'Y' || choose36 == 'N')
						break;
					printf("Do you want to shuffle Sub-weapon abilities? Y/N \n");
					scanf("%c",&choose36);
				}while(choose36 != 'Y' || choose36 != 'N');
				if(choose36 == 'Y')
				{

					for(int i = 0; i < subWeaponAbilityAddressLength; i++)
					{
						do
						{
							randValue = rand() % subWeaponAbilityAddressLength;
						}while(subWeaponAbilityIDs[randValue] == 0xFF);
						fseek(fp,subWeaponAbilityAddresses[i],SEEK_SET);
						newByte = subWeaponAbilityIDs[randValue];
						fwrite(&newByte,sizeof(newByte),1,fp);
						subWeaponAbilityIDs[randValue] = 0xFF;
					}
				}
			}
		}
		void randomize()
		{
			shopEdit();
			dropRateEdit();
			attemptEncyclopediafix();
			changeORBSandWHIPS(false);
			for(int i = 0; i < addressArrayLength; i++) 
			{
				fseek(fp, AddressArray[i],SEEK_SET);
				do
				{
					randValue = rand() % ItemArrayLength;
					newByte = ItemArray[randValue];
				} while (ItemArray[randValue] == 0x0);
				fwrite(&newByte, sizeof(newByte), 1, fp);
				ItemArray[randValue] = 0x0;
			}
			fflush(stdin);
			char choose8;
			do
			{
				if(choose8 == 'Y' || choose8 == 'N')
					break;
				printf("do you want to add customizations? Y/N \n");
				scanf("%c",&choose8);
			} while (choose8 != 'Y' || choose8 != 'N');
			if(choose8 == 'Y')
			{
				modeSelection();
			}
		}
		void createlog(char logName[])
		{
			fptr = fopen(logName,"w");
			char seedToString[128];
			int SIZE = 1;
			unsigned char buffer[SIZE];
			int n;
			_Bool skipping = false;
			if(fptr == NULL)
			{
				char choose998;
				do
				{
					if(choose998 == 'c' || choose998 == 'r')
						break;
					printf("could not open file \n"); printf("press c to exit \npress s to skip making log \n");
					scanf("%c",&choose998);
				}while(true);
				if(choose == 'c')
				{
					exit(1); //failed to open file
				}
				else if(choose998 == 'r')
				{
					char choose4[128];
					printf("Where do you want the log saved? \n(Spaces are NOT allowed) \n(This MUST be a .txt file (and have a filepath (including filename) of less than 128 characters)) \n");
					printf("Only attempt to retry once (this can cause a memory leak); \n");
					scanf("%s",&choose4);
					createlog(choose4);
				}
				else if(choose998 == 's')
				{
					skipping = true;
				}
			}
			if(!skipping)
			{
				sprintf(seedToString,"%d",xseed);
				fprintf(fptr,"Castlevania: Lament of Innocence RANDOMIZER! \n \n");
				fprintf(fptr,"Seed: ");fprintf(fptr,seedToString);fprintf(fptr,"\n \n");
				switch(PowerUpsChangesint)
				{
					case 1:
					fprintf(fptr, "All Powerups changed to potions \n");
					break;
					case 2: 
					fprintf(fptr, "Repeatable Powerups changed to potions \n");
					break;
					case 4:
					fprintf(fptr, "All Powerups changed to restoring items \n");
					break;
					case 5:
					fprintf(fptr, "Repeatable Powerups changed to restoring items \n");
					break;
					default:
					break;
				}
				if(OrbandWhipShuffels)
				{
					if(BlackOrbIncluded)
					{
						fprintf(fptr,"Orb / Whip Shuffle ON (Black Orb included) \n");
					}
					else
					{
						fprintf(fptr,"Orb / Whip Shuffle ON (Black Orb excluded) \n");
					}
				}	
				if(DragonCrestbehindKeybool)
					fprintf(fptr,"Dragon Crest locked behind key doors \n");
				if(modelReplaced != 'U')
				{
					if(modelReplaced == 'R')
					{
						fprintf(fptr, "Random models used \n");
					}
					else if(modelReplaced == 'S')
					{
						fprintf(fptr, "models replaced same used \n");
					}
				}
				if(storeModified)
				{
					fprintf(fptr,"Store Modified \n");
				}
				fprintf(fptr,"\n \n \n");
				for(int i = 0; i<addressArrayLength; i++)
				{
					//read mem
					fseek(fp, AddressArray[i], SEEK_SET);
					n = fread(buffer, sizeof(buffer),1,fp);
					fprintf(fptr, arr_addressList[i].name);fprintf(fptr,": ");
					for(int t = 0; t < SIZE; t++)
					{
						fprintf(fptr,itemNames[buffer[t]]);fprintf(fptr,"\n");
					}
				}
				if (OrbandWhipShuffels)
				{
					const int startOrbsWhipsTable = 0x0044F590;
					//const int endOrbsWhipsTable = 0x0044F5BF;
					int n;
					for(int i = 0; i < 12; i++)
					{
						fseek(fp, startOrbsWhipsTable + (4*i) ,SEEK_SET);
						n = fread(buffer, sizeof(buffer),1,fp);
						switch(i)
						{
							case 0:
							fprintf(fptr,"Red Orb: ");fprintf(fptr,itemNames[buffer[0]]);fprintf(fptr,"\n");
							break;
							case 1:
							fprintf(fptr,"Blue Orb: ");fprintf(fptr,itemNames[buffer[0]]);fprintf(fptr,"\n");
							break;
							case 2:
							fprintf(fptr,"Yellow Orb: ");fprintf(fptr,itemNames[buffer[0]]);fprintf(fptr,"\n");
							break;
							case 3:
							fprintf(fptr,"Green Orb: ");fprintf(fptr,itemNames[buffer[0]]);fprintf(fptr,"\n");
							break;
							case 4:
							fprintf(fptr,"Purple Orb: ");fprintf(fptr,itemNames[buffer[0]]);fprintf(fptr,"\n");
							break;
							case 6:
							fprintf(fptr,"Black Orb: ");fprintf(fptr,itemNames[buffer[0]]);fprintf(fptr,"\n");
							break;
							case 8:
							fprintf(fptr,"Whip of Flames: ");fprintf(fptr,itemNames[buffer[0]]);fprintf(fptr,"\n");
							break;
							case 9:
							fprintf(fptr,"Whip of Ice: ");fprintf(fptr,itemNames[buffer[0]]);fprintf(fptr,"\n");
							break;
							case 10:
							fprintf(fptr,"Whip of Lightning: ");fprintf(fptr,itemNames[buffer[0]]);fprintf(fptr,"\n");
							break;
							default:
							break;
						}
					}
				}
				if(storeModified)
				{
					fprintf(fptr,"\n");
					int itemToFind = 0x01;
					//int price[] = {0x00,0x00,0x00};
					for(int i = 0; i < shopBuyPriceAddressLength; i++)
					{
						fseek(fp,shopBuyPriceAddress[i],SEEK_SET);
						n = fread(buffer,sizeof(buffer),1,fp);
						if(buffer[0] != 0)
						{
							if(i >=0 && i <= 9)
							{
								itemToFind = i + 0x86;
							}
							else if(i >= 10 && i <= 34)
							{
								itemToFind = i;
							}
							else if(i >= 35 && i <= 43)
							{
								itemToFind = i + 7;
							}
							else if(i >= 44 && i <= 49)
							{
								itemToFind = i + 16;
							}
							else if(i == 50)
							{
								itemToFind = 0x42;
							}
							else if(i >= 51 && i <= 60)
							{
								itemToFind = i + 16;
							}
							else if(i >= 61 && i <= 64)
							{
								itemToFind = i + 24;
							}
							else if(i == 65)
							{
								itemToFind = 0x5F;
							}
							else if(i >= 66 && i <= 72)
							{
								itemToFind = i + 31;
							}
							else if(i >= 73 && i <= 82)
							{
								itemToFind = i + 33;
							}
							fprintf(fptr,itemNames[itemToFind]);fprintf(fptr," ");
							fprintf(fptr,"in the shop \n");
						}
						
					}
					
				}
				printf("spoiler log made \n");
				fclose(fptr);
			}
		}
		randomize();
		printf("File is now randomized! \n");
		_Bool inPagoda(int itemID)
		{
			fseek(fp, 0, SEEK_SET);
			int SIZE = 1;
			unsigned char buffer[SIZE];
			int n = fread(buffer, sizeof(buffer), 1, fp);
			for(int i = 0; i < PotMMChecksLength; i++)
			{
				fseek(fp, PotMMChecks[i],SEEK_SET);
				n = fread(buffer,sizeof(buffer), 1, fp);
				if(buffer[0] == itemID)
					return true;
			}
			return false;
		}
		void lockedChecks(int itemID)
		{		
			fseek(fp, 0, SEEK_SET);
			int SIZE = 1;
			unsigned char buffer[SIZE];
			int n = fread(buffer, sizeof(buffer), 1, fp);
			for (int i = 0; i < LocksChecksLength; i++)
			{
				fseek(fp, LocksChecks[i], SEEK_SET);
				n = fread(buffer,sizeof(buffer), 1, fp);
				if(buffer[0] == itemID)
				{
					switch (i + 1) 
					{
						case 1:
						if(inPagoda(0x86)) //wolf's foot
						{
							printf("Wolf's foot required from Pagoda \n");
							return;
						}
						if(itemID != 0x86)
						{
							lockedChecks(0x86);
						}
						else
						{
							printf("Wolf's foot requires Wolf's foot \n");
							return;
						}
						break;
						case 2:
						if(inPagoda(0x59)) //white tiger key
						{
							printf("White Tiger Key required from Pagoda \n");
							return;
						}
						lockedChecks(0x59);
						break;
						case 3:
						if(inPagoda(0x02)) //whip of flames
						{
							printf("Whip of Flames required from Pagoda \n");
							return;
						}
						if(itemID != 0x02)
						{
							lockedChecks(0x02);	
						}
						else
						{
							printf("Whip of Flames requires Whip of Flames \n");
							return;
						}
						break;
						case 4:
						if(inPagoda(0x5D)) //yellow dragon key
						{
							printf("Yellow Dragon Key required from Pagoda \n");
							return;
						}
						lockedChecks(0x5D);
						break;
						case 5:
						if(inPagoda(0x86)) //wolf's foot
						{
							printf("Wolf's foot required from Pagoda \n");
							return;
						}
						if(itemID != 0x86)
						{
							lockedChecks(0x86);
						}
						else
						{
							printf("Wolf's foot requires Wolf's foot \n");
							return;
						}
						break;
						case 6:
						if(inPagoda(0x5A)) //blue dragon key
						{
							printf("Blue Dragon Key from Pagoda \n");
							return;
						}
						lockedChecks(0x5A);
						break;
						case 7:
						if(inPagoda(0x03)) //whip of ice
						{
							printf("Whip of Ice required from Pagoda \n");
							return;
						}
						lockedChecks(0x03);
						if(inPagoda(0x02)) //whip of flames
						{
							printf("Whip of Flames required from Pagoda \n");
							return;
						}
						if(itemID != 0x02)
						{
							lockedChecks(0x02);
						}
						else
						{
							printf("Whip of Flames requires Whip of Flames \n");
							return;
						}
						break;
						case 8:
						if(inPagoda(0x5C)) //black turtle key
						{
							printf("Black Turtle Key required from Pagoda \n");
							return;
						}
						lockedChecks(0x5C);
						break;
						case 9:
						if(inPagoda(0x5B)) //red phoenix key
						{
							printf("Red Phoenix Key required from Pagoda \n");
							return;
						}
						lockedChecks(0x5B);
						break;
						case 10:
						lockedChecks(0x56);
						break;
						case 11:
						if(inPagoda(0x42))
						{
							printf("Curtain Time Bell required from Pagoda \n");
							return;
						}
						if(itemID != 0x42)
						{
							lockedChecks(0x42);
						}
						else
						{
							printf("Curtain Time Bell requires Curtain Time Bell \n");
							return;
						}
						break;
						case 12:
						if(inPagoda(0x55))
						{
							printf("E Tablet required from Pagoda \n");
							return;
						}
						if(itemID != 0x55)
						{
							lockedChecks(0x55);
						}
						else
						{
							printf("E Tablet requires E Tablet \n");
							return;
						}
						break;
						default:
							break;
					}
				}
			}
					/* int LocksChecks[] = {
					Megingjord,HeartBrooch,LittleHammer,BlackBishop,SaiseiIncense,ToolBag,WhiteOrb,Draupnir,WolfsFoot,DragonCrest,SuccubusDrop,GolemDrop
					}; */
		}
		fflush(stdin);
		char choose7;
		do
		{
			printf("Do you want to add Hints? Y/N \n");
			scanf("%c",&choose7);
			if(choose7 == 'Y' || choose7 == 'N')
				break;
		} while (choose7 != 'Y' || choose7 != 'N'); 
		if(choose7 == 'Y')
		{
			hints();
			printf("Hints are on! \n");
		}
		else
		{
			printf("Hints will not be added \n");
		}
		fflush(stdin);
		char choose5;
		do
		{
			printf("check seed ? Y/N \n");
			scanf("%c",&choose5);
			if(choose5 == 'Y' || choose5 == 'N')
				break;			
		} while (choose5 != 'Y' || choose5 != 'N');
		if(choose5 == 'Y')
		{
			for(int i = 0x7F; i < 0x84; i++)
			{
				if(inPagoda(i))
				{
					printf("Orb in Pagoda \n");
					break;
				}
				lockedChecks(i);
			}
			lockedChecks(0x5E); //Dragon Crest
			lockedChecks(0x05); //Vampire Killer
			if(OrbandWhipShuffels)
			{
				//0x42 = Curtain Time Bell, 0x55 = E Tablet, 0x58 = Unlock Jewel
				const int startOrbsWhipsTable = 0x0044F590;
				//const int endOrbsWhipsTable = 0x0044F5BF;
				int n;
				int SIZE = 1;
				unsigned char buffer[SIZE];
				for(int t = 0; t < 12; t++)
				{
					fseek(fp, startOrbsWhipsTable + (4*t) ,SEEK_SET);
					n = fread(buffer,sizeof(buffer),1,fp);
					if(t == 0 || t == 2 || t == 6)
					{
						fseek(fp, startOrbsWhipsTable + (4*t) ,SEEK_SET);
						n = fread(buffer,sizeof(buffer),1,fp);
						if(buffer[0] >= 0x7F && buffer[0] <= 0x83)
						{
							switch(t)
							{
								case 0:
									if(inPagoda(0x55))
									{
										printf("E Tablet required from Pagoda \n");
									}
									lockedChecks(0x55);
									break;
								case 2:
									if(inPagoda(0x42))
									{
										printf("Curtain Time Bell required from Pagoda \n");
									}
									lockedChecks(0x42);
									break;
								case 6:
									if(inPagoda(0x58))
									{
										printf("Unlock Jewel required from Pagoda \n");
									}
									lockedChecks(0x58);
									break;
								default:
									break;
							}
						}
					}
				}
			}
			printf("if no warnings are/were shown: seed is likely beatable \n");
		}	
		fflush(stdin);
		do
		{
			if(choose3 == 'N' || choose3 == 'Y')
				break;
			printf("do you want to make a log? Y/N \n");
			scanf("%c",&choose3);
		}
		while(choose3 != 'Y' || choose3 != 'N');
		if(choose3 == 'Y')
		{
			char choose4[128];
			printf("Where do you want the log saved? \n(Spaces are NOT allowed) \n(This MUST be a .txt file) \n");
			scanf("%s",&choose4);
			createlog(choose4);
		}
		else
		{
			printf("no log will be made \n");
		}	
		printf("finished \n");
		fclose(fp);
		fflush(stdin);
		char choose6;
		do
		{
			printf("press c to close \n");
			scanf("%c",&choose6);
			if(choose6 == 'c')
				break;
		} while(choose6 != 'c');
		return 0;
}
