#include <Geode/Geode.hpp>
#include <Geode/modify/EffectGameObject.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/HardStreak.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Windows.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <cctype>


// ============== EMBEDDED PATTERNS ==============
struct ReplayInput {
    int frame;
    bool down;
};

// GRIEF - 1376 inputs
inline const ReplayInput GRIEF_PATTERN[] = {
    {40, true}, {178, false}, {231, true}, {274, false}, {318, true}, {348, false}, 
    {374, true}, {398, false}, {422, true}, {451, false}, {474, true}, {496, false}, 
    {518, true}, {542, false}, {573, true}, {604, false}, {635, true}, {635, false}, 
    {649, true}, {656, false}, {657, true}, {691, false}, {734, true}, {779, false}, 
    {820, true}, {902, false}, {951, true}, {997, false}, {998, true}, {998, false}, 
    {1007, true}, {1022, false}, {1062, true}, {1082, false}, {1094, true}, {1109, false}, 
    {1132, true}, {1137, false}, {1147, true}, {1189, false}, {1201, true}, {1212, false}, 
    {1220, true}, {1228, false}, {1236, true}, {1249, false}, {1256, true}, {1261, false}, 
    {1313, true}, {1331, false}, {1380, true}, {1420, false}, {1464, true}, {1510, false}, 
    {1555, true}, {1593, false}, {1640, true}, {1679, false}, {1791, true}, {1809, false}, 
    {1823, true}, {1836, false}, {1877, true}, {1891, false}, {1921, true}, {1939, false}, 
    {1947, true}, {1967, false}, {1986, true}, {1994, false}, {2067, true}, {2187, false}, 
    {2318, true}, {2451, false}, {2584, true}, {2757, false}, {2820, true}, {2855, false}, 
    {2904, true}, {2921, false}, {2934, true}, {2974, false}, {3006, true}, {3032, false}, 
    {3045, true}, {3081, false}, {3139, true}, {3166, false}, {3188, true}, {3214, false}, 
    {3239, true}, {3257, false}, {3281, true}, {3307, false}, {3328, true}, {3351, false}, 
    {3375, true}, {3400, false}, {3420, true}, {3547, false}, {3636, true}, {3719, false}, 
    {3832, true}, {3883, false}, {3916, true}, {3937, false}, {3962, true}, {3980, false}, 
    {3998, true}, {4016, false}, {4035, true}, {4054, false}, {4075, true}, {4094, false}, 
    {4108, true}, {4123, false}, {4169, true}, {4194, false}, {4247, true}, {4318, false}, 
    {4422, true}, {4526, false}, {4559, true}, {4602, false}, {4633, true}, {4674, false}, 
    {4724, true}, {4740, false}, {4749, true}, {4766, false}, {4778, true}, {4789, false}, 
    {4802, true}, {4812, false}, {4865, true}, {4932, false}, {4977, true}, {5021, false}, 
    {5065, true}, {5107, false}, {5152, true}, {5208, false}, {5237, true}, {5240, false}, 
    {5242, true}, {5259, false}, {5287, true}, {5313, false}, {5356, true}, {5426, false}, 
    {5492, true}, {5532, false}, {5553, true}, {5584, false}, {5633, true}, {5685, false}, 
    {5722, true}, {5744, false}, {5777, true}, {5820, false}, {5853, true}, {5869, false}, 
    {5891, true}, {5914, false}, {5934, true}, {5953, false}, {5971, true}, {5990, false}, 
    {6002, true}, {6040, false}, {6060, true}, {6081, false}, {6093, true}, {6112, false}, 
    {6155, true}, {6189, false}, {6219, true}, {6254, false}, {6313, true}, {6356, false}, 
    {6368, true}, {6377, false}, {6424, true}, {6455, false}, {6471, true}, {6549, false}, 
    {6622, true}, {6646, false}, {6682, true}, {6755, false}, {6800, true}, {6841, false}, 
    {6900, true}, {6968, false}, {7016, true}, {7043, false}, {7073, true}, {7096, false}, 
    {7105, true}, {7111, false}, {7136, true}, {7162, false}, {7187, true}, {7211, false}, 
    {7232, true}, {7252, false}, {7276, true}, {7297, false}, {7319, true}, {7339, false}, 
    {7360, true}, {7382, false}, {7409, true}, {7432, false}, {7455, true}, {7484, false}, 
    {7511, true}, {7528, false}, {7544, true}, {7572, false}, {7601, true}, {7629, false}, 
    {7775, true}, {7806, false}, {8271, true}, {8412, false}, {8497, true}, {8625, false}, 
    {8669, true}, {8713, false}, {8840, true}, {8922, false}, {8968, true}, {9009, false}, 
    {9054, true}, {9100, false}, {9181, true}, {9267, false}, {9395, true}, {9519, false}, 
    {9650, true}, {9783, false}, {9917, true}, {9959, false}, {10001, true}, {10041, false}, 
    {10084, true}, {10132, false}, {10173, true}, {10302, false}, {10390, true}, {10522, false}, 
    {10693, true}, {10822, false}, {10953, true}, {11118, false}, {11200, true}, {11245, false}, 
    {11332, true}, {11373, false}, {11415, true}, {11504, false}, {11553, true}, {11597, false}, 
    {11680, true}, {11811, false}, {11940, true}, {11978, false}, {12024, true}, {12067, false}, 
    {12113, true}, {12198, false}, {12281, true}, {12368, false}, {12453, true}, {12497, false}, 
    {12538, true}, {12582, false}, {12626, true}, {12669, false}, {12711, true}, {12813, false}, 
    {12848, true}, {12890, false}, {12994, true}, {13055, false}, {13128, true}, {13222, false}, 
    {13275, true}, {13302, false}, {13331, true}, {13357, false}, {13386, true}, {13411, false}, 
    {13439, true}, {13472, false}, {13529, true}, {13609, false}, {13664, true}, {13721, false}, 
    {13797, true}, {13819, false}, {13864, true}, {13911, false}, {13933, true}, {13978, false}, 
    {14023, true}, {14071, false}, {14141, true}, {14210, false}, {14287, true}, {14416, false}, 
    {14457, true}, {14496, false}, {14542, true}, {14587, false}, {14632, true}, {14674, false}, 
    {14780, true}, {14815, false}, {14886, true}, {15024, false}, {15057, true}, {15092, false}, 
    {15126, true}, {15161, false}, {15232, true}, {15267, false}, {15355, true}, {15440, false}, 
    {15468, true}, {15495, false}, {15525, true}, {15556, false}, {15578, true}, {15604, false}, 
    {15634, true}, {15659, false}, {15685, true}, {15712, false}, {15743, true}, {15773, false}, 
    {15797, true}, {15819, false}, {15842, true}, {15867, false}, {15889, true}, {15910, false}, 
    {15933, true}, {15955, false}, {15976, true}, {16050, false}, {16071, true}, {16096, false}, 
    {16118, true}, {16142, false}, {16163, true}, {16187, false}, {16230, true}, {16321, false}, 
    {16555, true}, {16579, false}, {16701, true}, {16717, false}, {16768, true}, {16784, false}, 
    {16799, true}, {16814, false}, {16838, true}, {16852, false}, {16964, true}, {16981, false}, 
    {17087, true}, {17101, false}, {17108, true}, {17120, false}, {17236, true}, {17253, false}, 
    {17330, true}, {17475, false}, {18316, true}, {18386, false}, {18435, true}, {18500, false}, 
    {18554, true}, {18598, false}, {18620, true}, {18644, false}, {18737, true}, {18805, false}, 
    {18855, true}, {18987, false}, {19033, true}, {19047, false}, {19060, true}, {19075, false}, 
    {19096, true}, {19108, false}, {19142, true}, {19181, false}, {19227, true}, {19260, false}, 
    {19285, true}, {19335, false}, {19380, true}, {19451, false}, {19482, true}, {19542, false}, 
    {19600, true}, {19650, false}, {19700, true}, {19747, false}, {19794, true}, {19845, false}, 
    {20007, true}, {20026, false}, {20054, true}, {20069, false}, {20096, true}, {20120, false}, 
    {20138, true}, {20162, false}, {20180, true}, {20208, false}, {20224, true}, {20256, false}, 
    {20298, true}, {20333, false}, {20393, true}, {20403, false}, {20417, true}, {20430, false}, 
    {20448, true}, {20455, false}, {20488, true}, {20500, false}, {20518, true}, {20528, false}, 
    {20543, true}, {20564, false}, {20584, true}, {20593, false}, {20615, true}, {20632, false}, 
    {20660, true}, {20666, false}, {20682, true}, {20692, false}, {20725, true}, {20835, false}, 
    {20839, true}, {20872, false}, {20907, true}, {20919, false}, {20945, true}, {20962, false}, 
    {20975, true}, {20988, false}, {21005, true}, {21017, false}, {21035, true}, {21047, false}, 
    {21062, true}, {21078, false}, {21139, true}, {21162, false}, {21181, true}, {21192, false}, 
    {21207, true}, {21219, false}, {21249, true}, {21256, false}, {21369, true}, {21377, false}, 
    {21527, true}, {21537, false}, {21620, true}, {21629, false}, {21708, true}, {21718, false}, 
    {22426, true}, {22464, false}, {22488, true}, {22510, false}, {22530, true}, {22552, false}, 
    {22576, true}, {22598, false}, {22621, true}, {22642, false}, {22670, true}, {22693, false}, 
    {22717, false}, {22718, true}, {22740, false}, {22761, true}, {22784, false}, {22807, true}, 
    {22830, false}, {22854, true}, {22877, false}, {22902, true}, {22922, false}, {22948, true}, 
    {22971, false}, {22996, true}, {23020, false}, {23042, true}, {23064, false}, {23086, true}, 
    {23109, false}, {23131, true}, {23153, false}, {23178, true}, {23198, false}, {23225, true}, 
    {23246, false}, {23269, true}, {23289, false}, {23314, true}, {23339, false}, {23360, true}, 
    {23381, false}, {23406, true}, {23498, false}, {23551, true}, {23670, false}, {23689, true}, 
    {23721, false}, {23764, true}, {23770, false}, {23799, true}, {23853, false}, {23939, true}, 
    {23952, false}, {23973, true}, {23984, false}, {24003, true}, {24014, false}, {24026, true}, 
    {24036, false}, {24055, true}, {24066, false}, {24078, true}, {24093, false}, {24124, true}, 
    {24135, false}, {24150, true}, {24162, false}, {24178, true}, {24192, false}, {24224, true}, 
    {24236, false}, {24263, true}, {24277, false}, {24305, true}, {24320, false}, {24342, true}, 
    {24353, false}, {24527, true}, {24593, false}, {24663, true}, {24728, false}, {24747, true}, 
    {24782, false}, {24805, true}, {24814, false}, {24830, true}, {24848, false}, {24891, true}, 
    {24937, false}, {24988, true}, {25022, false}, {25033, true}, {25048, false}, {25123, true}, 
    {25193, false}, {25241, true}, {25287, false}, {25332, true}, {25482, false}, {25506, true}, 
    {25528, false}, {25552, true}, {25575, false}, {25600, true}, {25620, false}, {25647, true}, 
    {25668, false}, {25689, true}, {25712, false}, {25737, true}, {25760, false}, {25783, true}, 
    {25806, false}, {25829, true}, {25850, false}, {25874, true}, {25895, false}, {25921, true}, 
    {25945, false}, {25966, true}, {25987, false}, {26017, true}, {26039, false}, {26060, true}, 
    {26082, false}, {26108, true}, {26128, false}, {26150, true}, {26173, false}, {26195, true}, 
    {26217, false}, {26241, true}, {26267, false}, {26290, true}, {26310, false}, {26343, true}, 
    {26404, false}, {26773, true}, {26892, false}, {27010, true}, {27014, false}, {27050, true}, 
    {27104, false}, {27150, true}, {27187, false}, {27229, true}, {27275, false}, {27320, true}, 
    {27366, false}, {27369, true}, {27416, false}, {27508, true}, {27530, false}, {27546, true}, 
    {27570, false}, {27602, true}, {27646, false}, {27794, true}, {27842, false}, {27920, true}, 
    {27930, false}, {27993, true}, {28002, false}, {28276, true}, {28285, false}, {28633, true}, 
    {28691, false}, {28751, true}, {28803, false}, {28844, true}, {28869, false}, {28896, true}, 
    {28935, false}, {28957, true}, {28982, false}, {29024, true}, {29063, false}, {29085, true}, 
    {29120, false}, {29155, true}, {29175, false}, {29198, true}, {29220, false}, {29241, true}, 
    {29264, false}, {29295, true}, {29320, false}, {29342, true}, {29365, false}, {29388, true}, 
    {29412, false}, {29446, true}, {29488, false}, {29759, true}, {29766, false}, {29785, true}, 
    {29790, false}, {29945, true}, {29952, false}, {30021, true}, {30025, false}, {30223, true}, 
    {30229, false}, {30249, true}, {30256, false}, {30296, true}, {30303, false}, {30575, true}, 
    {30586, false}, {30622, true}, {30637, false}, {30640, true}, {30652, false}, {30688, true}, 
    {30706, false}, {30727, true}, {30742, false}, {30760, true}, {30770, false}, {30782, true}, 
    {30789, false}, {30828, true}, {30836, false}, {30994, true}, {31022, false}, {31051, true}, 
    {31069, false}, {31092, true}, {31103, false}, {31132, true}, {31162, false}, {31201, true}, 
    {31214, false}, {31261, true}, {31268, false}, {31310, true}, {31313, false}, {31315, true}, 
    {31318, false}, {31320, true}, {31323, false}, {31326, true}, {31329, false}, {31332, true}, 
    {31335, true}, {31335, false}, {31338, false}, {31342, true}, {31345, false}, {31348, true}, 
    {31351, false}, {31355, true}, {31357, false}, {31361, true}, {31363, false}, {31366, true}, 
    {31369, false}, {31371, true}, {31375, true}, {31375, false}, {31378, false}, {31382, true}, 
    {31385, false}, {31389, true}, {31391, false}, {31394, true}, {31398, false}, {31401, true}, 
    {31405, false}, {31406, true}, {31409, false}, {31411, true}, {31414, false}, {31417, true}, 
    {31420, false}, {31423, true}, {31426, false}, {31429, true}, {31432, false}, {31435, true}, 
    {31438, false}, {31441, true}, {31444, false}, {31447, true}, {31449, false}, {31463, true}, 
    {31463, false}, {31517, true}, {31523, false}, {31669, true}, {31702, false}, {31731, true}, 
    {31757, false}, {31788, true}, {31812, false}, {31825, true}, {31858, false}, {31992, true}, 
    {32114, false}, {32133, true}, {32154, false}, {32258, true}, {32292, false}, {32337, true}, 
    {32348, false}, {32374, true}, {32386, false}, {32397, true}, {32421, false}, {32437, true}, 
    {32461, false}, {32478, true}, {32498, false}, {32519, true}, {32531, false}, {32547, true}, 
    {32560, false}, {32574, true}, {32593, false}, {32603, true}, {32621, false}, {32660, true}, 
    {32745, false}, {32853, true}, {32904, false}, {32944, true}, {32985, false}, {33076, true}, 
    {33192, false}, {33302, true}, {33363, false}, {33409, true}, {33454, false}, {33636, true}, 
    {33640, false}, {33764, true}, {33772, false}, {33942, true}, {33948, false}, {34107, true}, 
    {34114, false}, {34244, true}, {34249, false}, {34410, true}, {34416, false}, {35662, true}, 
    {35674, false}, {35706, true}, {35726, false}, {35754, true}, {35766, false}, {35788, true}, 
    {35797, false}, {35814, true}, {35880, false}, {35942, true}, {35945, false}, {35965, true}, 
    {35981, false}, {35996, true}, {36002, false}, {36089, true}, {36131, false}, {36166, true}, 
    {36200, false}, {36218, true}, {36230, false}, {36245, true}, {36254, false}, {36269, true}, 
    {36281, false}, {36299, true}, {36314, false}, {36326, true}, {36340, false}, {36356, true}, 
    {36364, false}, {36402, true}, {36417, false}, {36433, true}, {36444, false}, {36455, true}, 
    {36472, false}, {36479, true}, {36488, false}, {36514, true}, {36521, false}, {36599, true}, 
    {36652, false}, {36662, true}, {36680, false}, {36689, true}, {36698, false}, {36710, true}, 
    {36716, false}, {36736, true}, {36778, false}, {36813, true}, {36827, false}, {36852, true}, 
    {36885, false}, {36899, true}, {36924, false}, {37040, true}, {37083, false}, {37267, true}, 
    {37288, false}, {37300, true}, {37313, false}, {37346, true}, {37443, false}, {37521, true}, 
    {37602, false}, {37787, true}, {37796, false}, {37876, true}, {37882, false}, {38060, true}, 
    {38068, false}, {38105, true}, {38111, false}, {38175, true}, {38184, false}, {38226, true}, 
    {38229, false}, {38232, true}, {38235, false}, {38237, true}, {38241, false}, {38242, true}, 
    {38246, true}, {38246, false}, {38248, false}, {38403, true}, {38408, false}, {38502, true}, 
    {38509, false}, {38760, true}, {38772, false}, {38820, true}, {38832, false}, {38866, true}, 
    {38883, false}, {38931, true}, {39029, false}, {39079, true}, {39092, false}, {39179, true}, 
    {39203, false}, {39317, true}, {39441, false}, {39470, true}, {39474, false}, {39496, true}, 
    {39501, false}, {39686, true}, {39692, false}, {39918, true}, {39937, false}, {40072, true}, 
    {40080, false}, {40117, true}, {40147, false}, {40196, true}, {40214, false}, {40378, true}, 
    {40411, false}, {40437, true}, {40464, false}, {40497, true}, {40514, false}, {40539, true}, 
    {40554, false}, {40572, true}, {40584, false}, {40612, true}, {40623, false}, {40644, true}, 
    {40663, false}, {40671, true}, {40677, false}, {40791, true}, {40804, false}, {40903, true}, 
    {40912, false}, {40974, true}, {40978, false}, {41149, true}, {41230, false}, {41370, true}, 
    {41419, false}, {41440, true}, {41461, false}, {41480, true}, {41497, false}, {41522, true}, 
    {41536, false}, {41549, true}, {41585, false}, {41613, true}, {41654, false}, {41847, true}, 
    {41851, false}, {41894, true}, {41899, false}, {42030, true}, {42035, false}, {42124, true}, 
    {42129, false}, {42232, true}, {42236, false}, {42476, true}, {42476, false}, {42476, true}, 
    {42476, false}, {42756, true}, {42782, false}, {42801, true}, {42818, false}, {42837, true}, 
    {42858, false}, {42873, true}, {42893, false}, {42982, true}, {43006, false}, {43027, true}, 
    {43040, false}, {43057, true}, {43078, false}, {43095, true}, {43113, false}, {43257, true}, 
    {43288, false}, {43417, true}, {43430, false}, {43445, true}, {43459, false}, {43471, true}, 
    {43483, false}, {43497, true}, {43528, false}, {43590, true}, {43621, false}, {43666, true}, 
    {43722, false}, {43746, true}, {43790, false}, {43847, true}, {43922, false}, {43998, true}, 
    {44074, false}, {44148, true}, {44181, false}, {44184, true}, {44231, false}, {44327, true}, 
    {44348, false}, {44366, true}, {44431, false}, {44438, true}, {44464, false}, {44475, true}, 
    {44494, false}, {44512, true}, {44524, false}, {44544, true}, {44561, false}, {44572, true}, 
    {44579, false}, {44598, true}, {44625, false}, {44645, true}, {44650, false}, {44721, true}, 
    {44766, false}, {44823, true}, {44837, false}, {44903, true}, {44909, false}, {44992, true}, 
    {45001, false}, {45075, true}, {45079, false}, {45111, true}, {45115, false}, {45209, true}, 
    {45219, false}, {45318, true}, {45327, false}, {45484, true}, {45514, false}, {45550, true}, 
    {45576, false}, {45596, true}, {45628, false}, {45662, true}, {45677, false}, {45691, true}, 
    {45706, false}, {45725, true}, {45740, false}, {45754, true}, {45790, false}, {45843, true}, 
    {45849, false}, {45972, true}, {45978, false}, {46093, true}, {46120, false}, {46224, true}, 
    {46236, false}, {46844, true}, {46867, false}, {46882, true}, {46898, false}, {46908, true}, 
    {46918, false}, {46944, true}, {46958, false}, {46983, true}, {46994, false}, {47034, true}, 
    {47041, false}, {47089, true}, {47094, false}, {47142, true}, {47150, false}, {47258, true}, 
    {47265, false}, {47367, true}, {47370, false}, {47434, true}, {47440, false}, {47483, true}, 
    {47555, false}, {47578, true}, {47587, false}, {47611, true}, {47634, false}, {47646, true}, 
    {47688, false}, {47740, true}, {47773, false}, {47857, true}, {47917, false}, {47954, true}, 
    {47978, false}, {48012, true}, {48034, false}, {48091, true}, {48118, false}, {48141, true}, 
    {48199, false}, {48220, true}, {48254, false}, {48323, true}, {48346, false}, {48363, true}, 
    {48414, false}, {48467, true}, {48503, false}, {48563, true}, {48630, false}, {48715, true}, 
    {48800, false}, {48864, true}, {48867, false}, {48885, true}, {48917, false}, {48948, true}, 
    {49046, false}, {49068, true}, {49091, false}, {49129, true}, {49133, false}, {49193, true}, 
    {49242, false}, {49253, true}, {49398, false}, {49419, true}, {49440, false}, {49444, true}, 
    {49478, false}, {49500, true}, {49524, false}, {49552, true}, {49578, false}, {49632, true}, 
    {49679, false}, {49699, true}, {49717, false}, {49735, true}, {49760, false}, {49803, true}, 
    {49822, false}, {49839, true}, {49866, false}, {49948, true}, {49963, false}, {49971, true}, 
    {49988, false}, {50009, true}, {50025, false}, {50084, true}, {50118, false}, {50136, true}, 
    {50148, false}, {50170, true}, {50176, false}, {50248, true}, {50262, false}, {50284, true}, 
    {50298, false}, {50314, true}, {50323, false}, {50419, true}, {50462, false}, {50477, true}, 
    {50508, false}, {50543, true}, {50566, false}, {50588, true}, {50610, false}, {50645, true}, 
    {50668, false}, {50704, true}, {50723, false}, {50746, true}, {50769, false}, {50786, true}, 
    {50802, false}, {50820, true}, {50837, false}, {50853, true}, {50864, false}, {51102, true}, 
    {51109, false}, {51221, true}, {51224, false}, {51481, true}, {51486, false}, {51528, true}, 
    {51533, false}, {51605, true}, {51606, false}, {51657, true}, {51657, false}, {51747, true}, 
    {51753, false}, {51984, true}, {52007, false}, {52087, true}, {52096, false}, {52183, true}, 
    {52192, false}, {52269, true}, {52307, false}, {52645, true}, {52672, false}, {52702, true}, 
    {52762, false}, {52794, true}, {52849, false}, {52945, true}, {53050, false}, {53151, true}, 
    {53187, false}, {53231, true}, {53271, false}, {53317, true}, {53374, false}, {53416, true}, 
    {53487, false}, {53527, true}, {53586, false}, {53673, true}, {53779, false}, {53848, true}, 
    {53867, false}, {53901, true}, {53964, false}, {54023, true}, {54039, false}, {54057, true}, 
    {54069, false}, {54117, true}, {54143, false}, {54146, true}, {54178, false}, {54183, true}, 
    {54216, false}, {54280, true}, {54296, false}, {54334, true}, {54346, false}, {54366, true}, 
    {54382, false}, {54402, true}, {54417, false}, {54440, true}, {54451, false}, {54470, true}, 
    {54477, false}, {54564, true}, {54625, false}, {54667, true}, {54717, false}, {54839, true}, 
    {54875, false}, {54889, true}, {54935, false}, {55091, true}, {55099, false}, {55210, true}, 
    {55220, false}, {55308, true}, {55317, false}, {55507, true}, {55514, false}, {55673, true}, 
    {55688, false}, {55807, true}, {55814, false}, {55927, true}, {56012, false}, {56134, true}, 
    {56190, false}, {56258, true}, {56311, false}, {56426, true}, {56552, false}, {56623, true}, 
    {56665, false}, {56730, true}, {56746, false}, {56765, true}, {56775, false}, {56799, true}, 
    {56816, false}, {56838, true}, {56851, false}, {56880, true}, {56895, false}, {56960, true}, 
    {57023, false}, {57052, true}, {57101, false}, {57150, true}, {57192, false}, {57234, true}, 
    {57279, false}, {57324, true}, {57369, false}, {57443, true}, {57507, false}, {57616, true}, 
    {57726, false}, {57793, true}, {57838, true}, {57854, false}, {57948, true}, {58011, false}, 
    {58041, true}, {58098, false}, {58155, true}, {58178, false}, {58208, true}, {58236, false}, 
    {58289, true}, {58331, false}, {58372, true}, {58430, false}, {58460, true}, {58534, false}, 
    {58609, true}, {58662, false}, {58702, true}, {58740, false}, {58783, true}, {58841, false}, 
    {58900, true}, {58938, false}, {58959, true}, {58964, false}, {58991, true}, {59017, false}, 
    {59036, true}, {59058, false}, {59080, true}, {59102, false}, {59128, true}, {59173, false}, 
    {59208, true}, {59230, false}
};
inline constexpr size_t GRIEF_PATTERN_SIZE = 1376;

// ANDROMEDA - 522 inputs
inline const ReplayInput ANDROMEDA_PATTERN[] = {
    {78, true}, {88, false}, {120, true}, {130, false}, {172, true}, {184, false}, 
    {222, true}, {236, false}, {264, true}, {272, false}, {323, true}, {334, false}, 
    {361, true}, {377, false}, {402, true}, {414, false}, {476, true}, {482, false}, 
    {515, true}, {561, false}, {772, true}, {801, false}, {838, true}, {849, false}, 
    {900, true}, {946, false}, {962, true}, {988, false}, {1043, true}, {1060, false}, 
    {1141, true}, {1147, false}, {1170, true}, {1184, false}, {1295, true}, {1301, false}, 
    {1431, true}, {1448, false}, {1515, true}, {1525, false}, {1558, true}, {1566, false}, 
    {1604, true}, {1611, false}, {1646, true}, {1654, false}, {1717, true}, {1738, false}, 
    {1756, true}, {1761, false}, {1783, true}, {1820, false}, {1861, true}, {1872, false}, 
    {1896, true}, {1910, false}, {1920, true}, {1932, false}, {2008, true}, {2016, false}, 
    {2029, true}, {2043, false}, {2058, true}, {2071, false}, {2119, true}, {2131, false}, 
    {2160, true}, {2169, false}, {2220, true}, {2247, false}, {2271, true}, {2296, false}, 
    {2324, true}, {2344, false}, {2359, true}, {2388, false}, {2442, true}, {2449, false}, 
    {2477, true}, {2485, false}, {2518, true}, {2524, false}, {2551, true}, {2560, false}, 
    {2575, true}, {2588, false}, {2812, true}, {2824, false}, {2856, true}, {2868, false}, 
    {2882, true}, {2894, false}, {2928, true}, {2937, false}, {2991, true}, {3002, false}, 
    {3028, true}, {3039, false}, {3128, true}, {3152, false}, {3170, true}, {3192, false}, 
    {3218, true}, {3275, false}, {3472, true}, {3482, false}, {3540, true}, {3556, false}, 
    {3615, true}, {3632, false}, {3689, true}, {3699, false}, {3718, true}, {3723, false}, 
    {3807, true}, {3817, false}, {3853, true}, {3883, false}, {3899, true}, {3912, false}, 
    {3924, true}, {3936, false}, {3954, true}, {3963, false}, {3978, true}, {3987, false}, 
    {4135, true}, {4158, false}, {4189, true}, {4200, false}, {4221, true}, {4228, false}, 
    {4263, true}, {4273, false}, {4301, true}, {4312, false}, {4361, true}, {4383, false}, 
    {4415, true}, {4436, false}, {4475, true}, {4486, false}, {4515, true}, {4524, false}, 
    {4559, true}, {4573, false}, {4628, true}, {4637, false}, {4668, true}, {4690, false}, 
    {4771, true}, {4780, false}, {4836, true}, {4838, false}, {4858, true}, {4873, false}, 
    {4926, true}, {4932, false}, {4952, true}, {4960, false}, {5003, true}, {5014, false}, 
    {5039, true}, {5052, false}, {5067, true}, {5078, false}, {5131, true}, {5141, false}, 
    {5243, true}, {5250, false}, {5294, true}, {5301, false}, {5331, true}, {5340, false}, 
    {5364, true}, {5396, false}, {5510, true}, {5514, false}, {5546, true}, {5553, false}, 
    {5594, true}, {5601, false}, {5630, true}, {5643, false}, {5683, true}, {5691, false}, 
    {5732, true}, {5740, false}, {5780, true}, {5789, false}, {5864, true}, {5868, false}, 
    {5899, true}, {5906, false}, {5962, true}, {5974, false}, {6142, true}, {6160, false}, 
    {6183, true}, {6233, false}, {6248, true}, {6265, false}, {6281, true}, {6299, false}, 
    {6336, true}, {6349, false}, {6411, true}, {6461, false}, {6887, true}, {6895, false}, 
    {6935, true}, {6966, false}, {6987, true}, {6998, false}, {7016, true}, {7029, false}, 
    {7054, true}, {7069, false}, {7143, true}, {7160, false}, {7172, true}, {7188, false}, 
    {7236, true}, {7238, false}, {7261, true}, {7277, false}, {7357, true}, {7379, false}, 
    {7395, true}, {7422, false}, {7483, true}, {7495, false}, {7518, true}, {7528, false}, 
    {7588, true}, {7592, false}, {7618, true}, {7625, false}, {7710, true}, {7721, false}, 
    {7746, true}, {7756, false}, {7783, true}, {7798, false}, {7834, true}, {7846, false}, 
    {7880, true}, {7886, false}, {7939, true}, {7949, false}, {7964, true}, {7975, false}, 
    {8013, true}, {8025, false}, {8061, true}, {8068, false}, {8116, true}, {8120, false}, 
    {8230, true}, {8245, false}, {8271, true}, {8302, false}, {8357, true}, {8377, false}, 
    {8387, true}, {8480, false}, {8506, true}, {8532, false}, {8564, true}, {8586, false}, 
    {8629, true}, {8668, false}, {8760, true}, {8765, false}, {8890, true}, {8899, false}, 
    {8935, true}, {8946, false}, {8979, true}, {8990, false}, {9093, true}, {9100, false}, 
    {9157, true}, {9170, false}, {9221, true}, {9234, false}, {9264, true}, {9272, false}, 
    {9587, true}, {9610, false}, {9620, true}, {9630, false}, {9677, true}, {9691, false}, 
    {9717, true}, {9726, false}, {9764, true}, {9802, false}, {9874, true}, {9886, false}, 
    {9959, true}, {9971, false}, {10021, true}, {10031, false}, {10086, true}, {10094, false}, 
    {10123, true}, {10153, false}, {10178, true}, {10194, false}, {10256, true}, {10300, false}, 
    {10382, true}, {10395, false}, {10438, true}, {10450, false}, {10501, true}, {10513, false}, 
    {10551, true}, {10565, false}, {10613, true}, {10709, false}, {10775, true}, {10794, false}, 
    {10817, true}, {10839, false}, {10893, true}, {10904, false}, {10918, true}, {10950, false}, 
    {10974, true}, {10985, false}, {11021, true}, {11030, false}, {11045, true}, {11056, false}, 
    {11124, true}, {11132, false}, {11158, true}, {11164, false}, {11280, true}, {11296, false}, 
    {11325, true}, {11332, false}, {11354, true}, {11361, false}, {11396, true}, {11400, false}, 
    {11544, true}, {11572, false}, {11593, true}, {11608, false}, {11648, true}, {11668, false}, 
    {11691, true}, {11712, false}, {11753, true}, {11775, false}, {11796, true}, {11812, false}, 
    {12268, true}, {12280, false}, {12305, true}, {12321, false}, {12366, true}, {12376, false}, 
    {12402, true}, {12412, false}, {12453, true}, {12465, false}, {12491, true}, {12525, false}, 
    {12571, true}, {12578, false}, {12642, true}, {12674, false}, {12699, true}, {12706, false}, 
    {12736, true}, {12745, false}, {12810, true}, {12818, false}, {12846, true}, {12856, false}, 
    {12882, true}, {12893, false}, {12929, true}, {12939, false}, {12964, true}, {12975, false}, 
    {13076, true}, {13085, false}, {13141, true}, {13151, false}, {13217, true}, {13227, false}, 
    {13281, true}, {13290, false}, {13322, true}, {13338, false}, {13390, true}, {13400, false}, 
    {13456, true}, {13472, false}, {13489, true}, {13503, false}, {13515, true}, {13568, false}, 
    {13636, true}, {13648, false}, {13717, true}, {13728, false}, {13754, true}, {13766, false}, 
    {13823, true}, {13834, false}, {13880, true}, {13892, false}, {13931, true}, {13940, false}, 
    {13961, true}, {13968, false}, {13996, true}, {14006, false}, {14051, true}, {14062, false}, 
    {14110, true}, {14121, false}, {14183, true}, {14192, false}, {14221, true}, {14272, false}, 
    {14360, true}, {14370, false}, {14400, true}, {14411, false}, {14457, true}, {14465, false}, 
    {14481, true}, {14485, false}, {14546, true}, {14552, false}, {14593, true}, {14598, false}, 
    {14636, true}, {14648, false}, {15002, true}, {15010, false}, {15026, true}, {15048, false}, 
    {15094, true}, {15111, false}, {15126, true}, {15139, false}, {15199, true}, {15212, false}, 
    {15261, true}, {15271, false}, {15311, true}, {15321, false}, {15363, true}, {15419, false}, 
    {15429, true}, {15442, false}, {15470, true}, {15481, false}, {15501, true}, {15509, false}, 
    {15548, true}, {15560, false}, {15597, true}, {15609, false}, {15666, true}, {15680, false}, 
    {15713, true}, {15721, false}, {15745, true}, {15788, false}, {15837, true}, {15851, false}, 
    {15881, true}, {15891, false}, {15943, true}, {15953, false}, {15973, true}, {15984, false}, 
    {16015, true}, {16023, false}, {16345, true}, {16356, false}, {16391, true}, {16403, false}, 
    {16433, true}, {16446, false}, {16487, true}, {16499, false}, {16564, true}, {16569, false}, 
    {16639, true}, {16648, false}, {16676, true}, {16689, false}, {16744, true}, {16757, false}, 
    {16785, true}, {16800, false}, {16889, true}, {16901, false}, {16958, true}, {16970, false}, 
    {17023, true}, {17036, false}, {17075, true}, {17082, false}, {17135, true}, {17142, false}, 
    {17184, true}, {17196, false}, {17246, true}, {17256, false}, {17300, true}, {17318, false}
};
inline constexpr size_t ANDROMEDA_PATTERN_SIZE = 522;

// SLAUGHTER - 406 inputs
inline const ReplayInput SLAUGHTER_PATTERN[] = {
    {291, false}, {333, true}, {342, false}, {442, false}, {486, false}, {512, true}, 
    {520, false}, {986, false}, {1049, true}, {1059, false}, {1588, true}, {1616, false}, 
    {1707, false}, {1720, true}, {1843, false}, {1900, false}, {1972, false}, {2026, true}, 
    {2033, false}, {2136, false}, {2173, true}, {2181, false}, {2614, false}, {2720, true}, 
    {2765, false}, {3348, true}, {3356, false}, {3372, false}, {3442, true}, {3449, false}, 
    {3496, true}, {3503, false}, {3524, false}, {3524, true}, {3524, false}, {3559, true}, 
    {3647, false}, {3669, true}, {3693, false}, {3711, true}, {3730, false}, {3748, true}, 
    {3765, false}, {3925, true}, {3933, false}, {4104, true}, {4114, false}, {4226, true}, 
    {4234, false}, {4592, true}, {4958, false}, {4998, false}, {5047, false}, {5089, true}, 
    {5216, false}, {5234, false}, {5268, true}, {5277, false}, {5498, true}, {5653, false}, 
    {5747, false}, {5785, true}, {5793, false}, {6426, false}, {6617, false}, {6652, true}, 
    {6660, false}, {6791, true}, {6801, false}, {6937, false}, {6971, true}, {7013, false}, 
    {7032, true}, {7045, false}, {7057, true}, {7069, false}, {7110, true}, {7151, false}, 
    {7176, true}, {7231, false}, {7248, false}, {7277, true}, {7286, false}, {7294, false}, 
    {7306, true}, {7367, false}, {7408, true}, {7432, false}, {7439, false}, {7462, true}, 
    {7503, false}, {7535, true}, {7549, false}, {7564, true}, {7580, false}, {7596, true}, 
    {7612, false}, {7627, true}, {7643, false}, {7659, true}, {7674, false}, {7688, true}, 
    {7704, false}, {7719, true}, {7735, false}, {7783, true}, {7872, false}, {7904, true}, 
    {7932, false}, {7953, false}, {7973, true}, {7983, false}, {8104, true}, {8123, false}, 
    {8148, true}, {8161, false}, {8173, false}, {8190, true}, {8242, false}, {8244, true}, 
    {8271, false}, {8370, true}, {8383, false}, {8411, true}, {8426, false}, {8449, true}, 
    {8465, false}, {8490, true}, {8506, false}, {8529, true}, {8546, false}, {8576, true}, 
    {8592, false}, {8613, true}, {8630, false}, {8658, true}, {8672, false}, {8696, true}, 
    {8711, false}, {8734, true}, {8748, false}, {8768, true}, {8783, false}, {8801, true}, 
    {8813, false}, {8837, true}, {8851, false}, {8871, true}, {8931, false}, {8991, true}, 
    {9008, false}, {9039, true}, {9054, false}, {9069, false}, {9088, true}, {9250, false}, 
    {9267, true}, {9283, false}, {9300, true}, {9313, false}, {9332, true}, {9344, false}, 
    {9362, true}, {9369, false}, {9419, true}, {9429, false}, {9484, true}, {9494, false}, 
    {9533, false}, {9562, true}, {9570, false}, {9645, true}, {9653, false}, {9727, true}, 
    {9736, false}, {9879, true}, {9912, false}, {9941, true}, {9953, false}, {9965, true}, 
    {9995, false}, {10066, true}, {10084, false}, {10102, false}, {10147, true}, {10169, false}, 
    {10189, true}, {10199, false}, {10218, false}, {10234, true}, {10260, false}, {10296, true}, 
    {10316, false}, {10393, true}, {10422, false}, {10428, false}, {10447, true}, {10466, false}, 
    {10481, true}, {10495, false}, {10507, true}, {10519, false}, {10531, true}, {10558, false}, 
    {10585, true}, {10616, false}, {10685, false}, {10691, true}, {10707, false}, {10729, true}, 
    {10737, false}, {10745, true}, {10768, false}, {10787, true}, {10833, false}, {10838, false}, 
    {10875, true}, {10883, false}, {10970, true}, {10978, false}, {10992, false}, {11071, true}, 
    {11081, false}, {11158, true}, {11167, false}, {11210, true}, {11219, false}, {11260, true}, 
    {11268, false}, {11335, true}, {11357, false}, {11378, true}, {11422, false}, {11435, true}, 
    {11455, false}, {11467, true}, {11481, false}, {11514, true}, {11550, false}, {11571, true}, 
    {11589, false}, {11620, true}, {11635, false}, {11651, true}, {11665, false}, {11678, true}, 
    {11693, false}, {11706, true}, {11720, false}, {11732, true}, {11745, false}, {11758, true}, 
    {11770, false}, {11783, true}, {11793, false}, {11908, true}, {11919, false}, {11942, true}, 
    {11958, false}, {11982, true}, {11995, false}, {12016, true}, {12030, false}, {12050, true}, 
    {12067, false}, {12088, true}, {12099, false}, {12129, true}, {12142, false}, {12157, true}, 
    {12170, false}, {12190, true}, {12204, false}, {12222, true}, {12237, false}, {12258, true}, 
    {12272, false}, {12294, true}, {12306, false}, {12327, true}, {12338, false}, {12361, true}, 
    {12377, false}, {12401, true}, {12415, false}, {12434, true}, {12449, false}, {12479, true}, 
    {12492, false}, {12510, true}, {12527, false}, {12549, true}, {12562, false}, {12581, true}, 
    {12592, false}, {12615, true}, {12627, false}, {12641, true}, {12654, false}, {12675, true}, 
    {12687, false}, {12700, true}, {12714, false}, {12727, false}, {12778, true}, {12823, false}, 
    {12837, true}, {12893, false}, {12955, true}, {12978, false}, {13027, true}, {13047, false}, 
    {13139, true}, {13156, false}, {13178, true}, {13191, false}, {13194, false}, {13206, true}, 
    {13224, false}, {13254, true}, {13263, false}, {13269, true}, {13294, false}, {13316, true}, 
    {13344, false}, {13350, false}, {13363, true}, {13375, false}, {13379, false}, {13445, false}, 
    {13471, true}, {13497, false}, {13514, true}, {13531, false}, {13542, true}, {13553, false}, 
    {13565, true}, {13577, false}, {13588, true}, {13597, false}, {13619, true}, {13645, false}, 
    {13712, true}, {13732, false}, {13750, true}, {13772, false}, {13792, true}, {13819, false}, 
    {13844, true}, {13881, false}, {13903, true}, {13920, false}, {13937, true}, {13952, false}, 
    {13969, true}, {13987, false}, {13997, false}, {14006, true}, {14020, false}, {14029, true}, 
    {14041, false}, {14080, true}, {14120, false}, {14166, true}, {14215, false}, {14293, false}, 
    {14316, true}, {14364, false}, {14377, false}, {14399, true}, {14442, false}, {14448, false}, 
    {14474, true}, {14485, false}, {14497, true}, {14513, false}, {14527, true}, {14536, false}, 
    {14552, true}, {14562, false}, {14575, true}, {14585, false}, {14592, true}, {14599, false}, 
    {14610, true}, {14622, false}, {14638, true}, {14644, false}, {14798, true}, {14807, false}, 
    {14941, true}, {14950, false}, {15030, false}, {15075, true}, {15085, false}, {15124, false}, 
    {15172, true}, {15180, false}, {15266, false}, {15320, true}, {15328, false}, {15539, false}, 
    {15605, true}, {15616, false}, {15690, true}, {15700, false}
};
inline constexpr size_t SLAUGHTER_PATTERN_SIZE = 406;

// SOLAR - 350 inputs
inline const ReplayInput SOLAR_PATTERN[] = {
    {179, true}, {188, false}, {231, true}, {238, false}, {331, true}, {342, false}, 
    {409, true}, {420, false}, {620, true}, {631, false}, {684, true}, {694, false}, 
    {763, true}, {774, false}, {1055, true}, {1068, false}, {1146, true}, {1182, false}, 
    {1238, true}, {1249, false}, {1358, true}, {1372, false}, {1432, true}, {1443, false}, 
    {1491, true}, {1516, false}, {1553, true}, {1566, false}, {1667, true}, {1676, false}, 
    {1737, true}, {1753, false}, {1827, true}, {1833, false}, {1975, true}, {1987, false}, 
    {2056, true}, {2067, false}, {2104, true}, {2114, false}, {2200, true}, {2216, false}, 
    {2341, true}, {2393, false}, {2434, true}, {2445, false}, {2493, true}, {2500, false}, 
    {2585, true}, {2703, false}, {2787, true}, {2793, false}, {2851, true}, {2862, false}, 
    {2905, true}, {2912, false}, {3014, true}, {3025, false}, {3279, true}, {3291, false}, 
    {3339, true}, {3350, false}, {3623, true}, {3659, false}, {3702, true}, {3713, false}, 
    {3802, true}, {3842, false}, {3868, true}, {3937, false}, {3964, true}, {3988, false}, 
    {4068, true}, {4093, false}, {4132, true}, {4151, false}, {4196, true}, {4261, false}, 
    {4303, true}, {4322, false}, {4356, true}, {4377, false}, {4413, true}, {4434, false}, 
    {4495, true}, {4573, false}, {4674, true}, {4738, false}, {4759, true}, {4800, false}, 
    {4834, true}, {4863, false}, {4919, true}, {4973, false}, {5023, true}, {5042, false}, 
    {5067, true}, {5080, false}, {5117, true}, {5139, false}, {5182, true}, {5213, false}, 
    {5240, true}, {5269, false}, {5331, true}, {5341, false}, {5481, true}, {5486, false}, 
    {5584, true}, {5590, false}, {5700, true}, {5710, false}, {5780, true}, {5789, false}, 
    {5820, true}, {5831, false}, {5886, true}, {5898, false}, {5998, true}, {6011, false}, 
    {6029, true}, {6053, false}, {6097, true}, {6107, false}, {6275, true}, {6287, false}, 
    {6367, true}, {6379, false}, {6443, true}, {6457, false}, {6550, true}, {6560, false}, 
    {6630, true}, {6641, false}, {6668, true}, {6684, false}, {6779, true}, {6790, false}, 
    {6812, true}, {6823, false}, {7127, true}, {7166, false}, {7199, true}, {7228, false}, 
    {7255, true}, {7298, false}, {7330, true}, {7362, false}, {7401, true}, {7434, false}, 
    {7487, true}, {7548, false}, {7575, true}, {7598, false}, {7642, true}, {7676, false}, 
    {7703, true}, {7727, false}, {7784, true}, {7813, false}, {7838, true}, {7866, false}, 
    {7896, true}, {7921, false}, {7945, true}, {7970, false}, {7991, true}, {8017, false}, 
    {8047, true}, {8077, false}, {8111, true}, {8134, false}, {8163, true}, {8200, false}, 
    {8223, true}, {8259, false}, {8307, true}, {8337, false}, {8371, true}, {8405, false}, 
    {8437, true}, {8460, false}, {8493, true}, {8507, false}, {8548, true}, {8578, false}, 
    {8599, true}, {8650, false}, {8672, true}, {8697, false}, {8715, true}, {8744, false}, 
    {8772, true}, {8796, false}, {8819, true}, {8841, false}, {8872, true}, {8922, false}, 
    {8960, true}, {8999, false}, {9022, true}, {9053, false}, {9103, true}, {9145, false}, 
    {9186, true}, {9251, false}, {9286, true}, {9304, false}, {9326, true}, {9354, false}, 
    {9402, true}, {9437, false}, {9484, true}, {9534, false}, {9547, true}, {9595, false}, 
    {9612, true}, {9650, false}, {9667, true}, {9707, false}, {9721, true}, {9756, false}, 
    {9824, true}, {9881, false}, {9930, true}, {9951, false}, {9989, true}, {10041, false}, 
    {10073, true}, {10101, false}, {10148, true}, {10199, false}, {10238, true}, {10278, false}, 
    {10322, true}, {10342, false}, {10697, true}, {10736, false}, {10749, true}, {10816, false}, 
    {10827, true}, {10867, false}, {10897, true}, {10924, false}, {10950, true}, {10976, false}, 
    {11001, true}, {11026, false}, {11050, true}, {11137, false}, {11163, true}, {11182, false}, 
    {11305, true}, {11337, false}, {11362, true}, {11387, false}, {11414, true}, {11438, false}, 
    {11462, true}, {11486, false}, {11577, true}, {11599, false}, {11623, true}, {11744, false}, 
    {11762, true}, {11796, false}, {11814, true}, {11842, false}, {11864, true}, {11899, false}, 
    {11917, true}, {11969, false}, {12032, true}, {12048, false}, {12063, true}, {12078, false}, 
    {12096, true}, {12159, false}, {12198, true}, {12227, false}, {12255, true}, {12282, false}, 
    {12317, true}, {12353, false}, {12463, true}, {12529, false}, {12573, true}, {12625, false}, 
    {12656, true}, {12691, false}, {12715, true}, {12763, false}, {12818, true}, {12874, false}, 
    {12928, true}, {12945, false}, {12985, true}, {13010, false}, {13027, true}, {13074, false}, 
    {13124, true}, {13148, false}, {13172, true}, {13224, false}, {13296, true}, {13354, false}, 
    {13391, true}, {13450, false}, {13503, true}, {13523, false}, {13544, true}, {13575, false}, 
    {13601, true}, {13628, false}, {13659, true}, {13675, false}, {13747, true}, {13811, false}, 
    {13855, true}, {13942, false}, {13980, true}, {13991, false}, {14011, true}, {14023, false}, 
    {14046, true}, {14059, false}, {14075, true}, {14090, false}, {14101, true}, {14119, false}, 
    {14129, true}, {14143, false}, {14152, true}, {14168, false}, {14287, true}, {14296, false}, 
    {14346, true}, {14360, false}, {14462, true}, {14469, false}, {14574, true}, {14587, false}, 
    {14854, true}, {14860, false}, {14920, true}, {14929, false}, {14972, true}, {14982, false}, 
    {15016, true}, {15027, false}
};
inline constexpr size_t SOLAR_PATTERN_SIZE = 350;

// VORTEX - 354 inputs
inline const ReplayInput VORTEX_PATTERN[] = {
    {57, true}, {62, false}, {280, true}, {285, false}, {337, true}, {386, false}, 
    {479, true}, {487, false}, {584, true}, {590, false}, {680, true}, {686, false}, 
    {793, true}, {889, false}, {893, true}, {899, false}, {914, true}, {1014, false}, 
    {1090, true}, {1095, false}, {1166, true}, {1264, false}, {1290, true}, {1360, false}, 
    {1397, true}, {1427, false}, {1473, true}, {1509, false}, {1773, true}, {1781, false}, 
    {1831, true}, {1836, false}, {1879, true}, {1886, false}, {1916, true}, {1923, false}, 
    {1982, true}, {2021, false}, {2081, true}, {2087, false}, {2183, true}, {2192, false}, 
    {2284, true}, {2291, false}, {2520, true}, {2526, false}, {2612, true}, {2623, false}, 
    {2677, true}, {2686, false}, {2713, true}, {2718, false}, {2777, true}, {2783, false}, 
    {2791, true}, {2796, false}, {2840, true}, {2847, false}, {2928, true}, {2934, false}, 
    {2961, true}, {2968, false}, {3036, true}, {3049, false}, {3089, true}, {3099, false}, 
    {3117, true}, {3141, false}, {3208, true}, {3216, false}, {3261, true}, {3267, false}, 
    {3367, true}, {3372, false}, {3430, true}, {3435, false}, {3456, true}, {3462, false}, 
    {3496, true}, {3501, false}, {3536, true}, {3543, false}, {3602, true}, {3610, false}, 
    {3646, true}, {3655, false}, {3690, true}, {3774, false}, {3829, true}, {3833, false}, 
    {3909, true}, {3916, false}, {3965, true}, {3969, false}, {4036, true}, {4040, false}, 
    {4225, true}, {4231, false}, {4439, true}, {4446, false}, {4507, true}, {4514, false}, 
    {4553, true}, {4558, false}, {4607, true}, {4613, false}, {4696, true}, {4702, false}, 
    {4735, true}, {4739, false}, {4804, true}, {4812, false}, {4848, true}, {4852, false}, 
    {4999, true}, {5004, false}, {5075, true}, {5082, false}, {5132, true}, {5138, false}, 
    {5176, true}, {5180, false}, {5284, true}, {5289, false}, {5350, true}, {5355, false}, 
    {5405, true}, {5411, false}, {5479, true}, {5486, false}, {5562, true}, {5568, false}, 
    {5601, true}, {5607, false}, {5659, true}, {5664, false}, {5705, true}, {5711, false}, 
    {5779, true}, {5785, false}, {5866, true}, {5871, false}, {5973, true}, {5979, false}, 
    {6038, true}, {6044, false}, {6130, true}, {6136, false}, {6213, true}, {6218, false}, 
    {6304, true}, {6309, false}, {6388, true}, {6395, false}, {6514, true}, {6521, false}, 
    {6551, true}, {6560, false}, {6578, true}, {6584, false}, {6973, true}, {6977, false}, 
    {7056, true}, {7062, false}, {7084, true}, {7089, false}, {7273, true}, {7279, false}, 
    {7351, true}, {7364, false}, {7379, true}, {7405, false}, {7470, true}, {7524, false}, 
    {7602, true}, {7611, false}, {7640, true}, {7666, false}, {7751, true}, {7788, false}, 
    {7832, true}, {7905, false}, {7953, true}, {8008, false}, {8021, true}, {8026, false}, 
    {8069, true}, {8080, false}, {8099, true}, {8167, false}, {8194, true}, {8215, false}, 
    {8227, true}, {8245, false}, {8259, true}, {8301, false}, {8359, true}, {8370, false}, 
    {8385, true}, {8410, false}, {8418, true}, {8426, false}, {8434, true}, {8448, false}, 
    {8462, true}, {8479, false}, {8502, true}, {8510, false}, {8530, true}, {8540, false}, 
    {8556, true}, {8645, false}, {8653, true}, {8672, false}, {8681, true}, {8703, false}, 
    {8712, true}, {8736, false}, {8746, true}, {8751, false}, {8779, true}, {8814, false}, 
    {8892, true}, {8910, false}, {9035, true}, {9075, false}, {9094, true}, {9121, false}, 
    {9141, true}, {9159, false}, {9214, true}, {9307, false}, {9344, true}, {9360, false}, 
    {9425, true}, {9460, false}, {9473, true}, {9498, false}, {9553, true}, {9569, false}, 
    {9611, true}, {9739, false}, {9769, true}, {9805, false}, {9830, true}, {9841, false}, 
    {9900, true}, {9926, false}, {9954, true}, {9976, false}, {10002, true}, {10028, false}, 
    {10049, true}, {10071, false}, {10096, true}, {10127, false}, {10152, true}, {10162, false}, 
    {10183, true}, {10187, false}, {10303, true}, {10307, false}, {10401, true}, {10404, false}, 
    {10515, true}, {10518, false}, {10622, true}, {10646, false}, {10666, true}, {10681, false}, 
    {10718, true}, {10773, false}, {10787, true}, {10801, false}, {10842, true}, {10849, false}, 
    {10869, true}, {10882, false}, {10895, true}, {10983, false}, {11033, true}, {11071, false}, 
    {11094, true}, {11123, false}, {11144, true}, {11168, false}, {11181, true}, {11194, false}, 
    {11254, true}, {11315, false}, {11357, true}, {11382, false}, {11452, true}, {11488, false}, 
    {11530, true}, {11561, false}, {11619, true}, {11648, false}, {11659, true}, {11685, false}, 
    {11720, true}, {11744, false}, {11758, true}, {11799, false}, {11874, true}, {11912, false}, 
    {11937, true}, {11959, false}, {11983, true}, {12048, false}, {12144, true}, {12311, false}, 
    {12354, true}, {12374, false}, {12411, true}, {12483, false}, {12525, true}, {12586, false}, 
    {12611, true}, {12638, false}, {12663, true}, {12690, false}, {12744, true}, {12775, false}, 
    {12783, true}, {12801, false}, {12881, true}, {12913, false}, {12939, true}, {12945, false}, 
    {13106, true}, {13137, false}, {13142, true}, {13201, false}, {13239, true}, {13264, false}, 
    {13314, true}, {13327, false}, {13342, true}, {13359, false}, {13380, true}, {13403, false}, 
    {13423, true}, {13438, false}, {13458, true}, {13555, false}, {13609, true}, {13726, false}, 
    {13743, true}, {13764, false}, {13830, true}, {13911, false}, {14050, true}, {14054, false}
};
inline constexpr size_t VORTEX_PATTERN_SIZE = 354;

// SOCIETY - 484 inputs
inline const ReplayInput SOCIETY_PATTERN[] = {
    {39, true}, {49, false}, {214, true}, {215, false}, {311, true}, {316, false}, 
    {444, true}, {449, false}, {729, true}, {751, false}, {879, true}, {911, false}, 
    {990, true}, {997, false}, {1048, true}, {1051, false}, {1142, true}, {1146, false}, 
    {1214, true}, {1218, false}, {1330, true}, {1334, false}, {1464, true}, {1469, false}, 
    {1535, true}, {1540, false}, {1658, true}, {1659, false}, {1703, true}, {1710, false}, 
    {1791, true}, {1815, false}, {1830, true}, {1836, false}, {1893, true}, {1899, false}, 
    {2002, true}, {2008, false}, {2071, true}, {2071, false}, {2113, true}, {2117, false}, 
    {2226, true}, {2230, false}, {2346, true}, {2350, false}, {2475, true}, {2479, false}, 
    {2623, true}, {2629, false}, {2741, true}, {2745, false}, {2855, true}, {2887, false}, 
    {2945, true}, {2949, false}, {3245, true}, {3316, false}, {3400, true}, {3507, false}, 
    {3581, true}, {3747, false}, {3881, true}, {3905, false}, {3993, true}, {4011, false}, 
    {4071, true}, {4142, false}, {4193, true}, {4213, false}, {4247, true}, {4305, false}, 
    {4347, true}, {4425, false}, {4526, true}, {4607, false}, {4634, true}, {4712, false}, 
    {4852, true}, {5043, false}, {5103, true}, {5108, false}, {5181, true}, {5249, false}, 
    {5300, true}, {5379, false}, {5488, true}, {5547, false}, {5610, true}, {5644, false}, 
    {5684, true}, {5698, false}, {5718, true}, {5735, false}, {5786, true}, {5790, false}, 
    {5857, true}, {5861, false}, {5925, true}, {5930, false}, {6016, true}, {6022, false}, 
    {6155, true}, {6158, false}, {6294, true}, {6299, false}, {6379, true}, {6385, false}, 
    {6449, true}, {6454, false}, {6599, true}, {6602, false}, {6625, true}, {6629, false}, 
    {6744, true}, {6748, false}, {6884, true}, {6887, false}, {6952, true}, {6955, false}, 
    {7074, true}, {7078, false}, {7229, true}, {7233, false}, {7296, true}, {7299, false}, 
    {7364, true}, {7367, false}, {7475, true}, {7478, false}, {7626, true}, {7631, false}, 
    {7669, true}, {7673, false}, {7723, true}, {7775, false}, {7861, true}, {7865, false}, 
    {7949, true}, {7970, false}, {8024, true}, {8028, false}, {8124, true}, {8160, false}, 
    {8260, true}, {8336, false}, {8399, true}, {8401, false}, {8522, true}, {8526, false}, 
    {8633, true}, {8654, false}, {8738, true}, {8742, false}, {8835, true}, {8838, false}, 
    {8930, true}, {8932, false}, {9016, true}, {9021, false}, {9042, true}, {9049, false}, 
    {9065, true}, {9070, false}, {9206, true}, {9210, false}, {9306, true}, {9407, false}, 
    {9563, true}, {9598, false}, {9686, true}, {9691, false}, {9798, true}, {9804, false}, 
    {9922, true}, {9929, false}, {9993, true}, {10005, false}, {10064, true}, {10099, false}, 
    {10168, true}, {10173, false}, {10294, true}, {10300, false}, {10411, true}, {10415, false}, 
    {10515, true}, {10633, false}, {10680, true}, {10683, false}, {10781, true}, {10815, false}, 
    {10916, true}, {10919, false}, {11010, true}, {11015, false}, {11577, true}, {11586, false}, 
    {11606, true}, {11633, false}, {11648, true}, {11681, false}, {11698, true}, {11725, false}, 
    {11750, true}, {11767, false}, {11783, true}, {11814, false}, {11829, true}, {11860, false}, 
    {11901, true}, {11913, false}, {11942, true}, {11965, false}, {12008, true}, {12041, false}, 
    {12179, true}, {12191, false}, {12204, true}, {12218, false}, {12261, true}, {12274, false}, 
    {12289, true}, {12331, false}, {12368, true}, {12392, false}, {12422, true}, {12448, false}, 
    {12464, true}, {12479, false}, {12551, true}, {12553, false}, {12650, true}, {12654, false}, 
    {12780, true}, {12812, false}, {12890, true}, {12917, false}, {13001, true}, {13051, false}, 
    {13113, true}, {13116, false}, {13259, true}, {13317, false}, {13352, true}, {13354, false}, 
    {13487, true}, {13528, false}, {13566, true}, {13580, false}, {13609, true}, {13622, false}, 
    {13648, true}, {13679, false}, {13699, true}, {13738, false}, {13764, true}, {13769, false}, 
    {13791, true}, {13811, false}, {13817, true}, {13834, false}, {13880, true}, {13920, false}, 
    {13935, true}, {13963, false}, {14056, true}, {14076, false}, {14151, true}, {14167, false}, 
    {14178, true}, {14210, false}, {14242, true}, {14262, false}, {14288, true}, {14308, false}, 
    {14409, true}, {14571, false}, {14632, true}, {14670, false}, {14791, true}, {14820, false}, 
    {14906, true}, {14931, false}, {14955, true}, {14982, false}, {15044, true}, {15045, false}, 
    {15138, true}, {15178, false}, {15197, true}, {15210, false}, {15253, true}, {15280, false}, 
    {15292, true}, {15322, false}, {15366, true}, {15367, false}, {15488, true}, {15491, false}, 
    {15599, true}, {15615, false}, {15631, true}, {15646, false}, {15659, true}, {15672, false}, 
    {15704, true}, {15718, false}, {15747, true}, {15779, false}, {15797, true}, {15821, false}, 
    {15850, true}, {15865, false}, {15878, true}, {15890, false}, {15905, true}, {15939, false}, 
    {15958, true}, {15977, false}, {16020, true}, {16028, false}, {16049, true}, {16096, false}, 
    {16123, true}, {16128, false}, {16180, true}, {16236, false}, {16284, true}, {16314, false}, 
    {16332, true}, {16346, false}, {16360, true}, {16408, false}, {16434, true}, {16445, false}, 
    {16503, true}, {16535, false}, {16548, true}, {16579, false}, {16591, true}, {16604, false}, 
    {16616, true}, {16630, false}, {16661, true}, {16672, false}, {16700, true}, {16723, false}, 
    {16734, true}, {16742, false}, {16766, true}, {16769, false}, {16848, true}, {16849, false}, 
    {16874, true}, {16876, false}, {16920, true}, {16923, false}, {17022, true}, {17026, false}, 
    {17069, true}, {17070, false}, {17131, true}, {17133, false}, {17152, true}, {17154, false}, 
    {17191, true}, {17192, false}, {17371, true}, {17396, false}, {17404, true}, {17416, false}, 
    {17427, true}, {17466, false}, {17492, true}, {17511, false}, {17536, true}, {17559, false}, 
    {17658, true}, {17659, false}, {17688, true}, {17691, false}, {17815, true}, {17845, false}, 
    {17906, true}, {17939, false}, {17992, true}, {18010, false}, {18017, true}, {18038, false}, 
    {18091, true}, {18093, false}, {18145, true}, {18200, false}, {18255, true}, {18280, false}, 
    {18319, true}, {18353, false}, {18392, true}, {18396, false}, {18467, true}, {18540, false}, 
    {18564, true}, {18602, false}, {18629, true}, {18656, false}, {18760, true}, {18797, false}, 
    {18818, true}, {18834, false}, {18848, true}, {18870, false}, {19004, true}, {19035, false}, 
    {19065, true}, {19084, false}, {19113, true}, {19128, false}, {19140, true}, {19161, false}, 
    {19185, true}, {19210, false}, {19249, true}, {19255, false}, {19499, true}, {19505, false}, 
    {19625, true}, {19636, false}, {19706, true}, {19712, false}, {19926, true}, {19932, false}, 
    {20068, true}, {20072, false}, {20176, true}, {20190, false}, {20341, true}, {20349, false}, 
    {20719, true}, {20724, false}, {20933, true}, {20977, false}, {21026, true}, {21046, false}, 
    {21082, true}, {21090, false}, {21297, true}, {21307, false}, {21322, true}, {21334, false}, 
    {21388, true}, {21398, false}, {21456, true}, {21478, false}, {21529, true}, {21537, false}, 
    {21621, true}, {21688, false}, {21803, true}, {21828, false}, {21846, true}, {21862, false}, 
    {21977, true}, {21983, false}, {22044, true}, {22053, false}, {22190, true}, {22197, false}, 
    {22273, true}, {22279, false}, {22480, true}, {22480, false}, {22586, true}, {22590, false}, 
    {22634, true}, {22636, false}, {22713, true}, {22749, false}, {22848, true}, {22851, false}, 
    {22939, true}, {22979, false}, {23014, true}, {23016, false}
};
inline constexpr size_t SOCIETY_PATTERN_SIZE = 484;

struct PatternInfo {
    const char* keyword;
    const ReplayInput* data;
    size_t size;
};

inline const PatternInfo ALL_PATTERNS[] = {
    { "grief", GRIEF_PATTERN, GRIEF_PATTERN_SIZE },
    { "andromeda", ANDROMEDA_PATTERN, ANDROMEDA_PATTERN_SIZE },
    { "slaughter", SLAUGHTER_PATTERN, SLAUGHTER_PATTERN_SIZE },
    { "solar", SOLAR_PATTERN, SOLAR_PATTERN_SIZE },
    { "vortex", VORTEX_PATTERN, VORTEX_PATTERN_SIZE },
    { "society", SOCIETY_PATTERN, SOCIETY_PATTERN_SIZE },
};
inline constexpr size_t ALL_PATTERNS_COUNT = 6;

// ============== END PATTERNS ==============


using namespace geode::prelude;

namespace {
    constexpr int HOOK_PRIORITY = -1000000;
    constexpr float TICK = 0.25f;
    constexpr int STAGE_COUNT = 5;

    constexpr uint64_t typeBit(GameObjectType type) {
        return 1ull << static_cast<int>(type);
    }

    constexpr uint64_t COLLIDING_TYPES =
        typeBit(GameObjectType::Solid) |
        typeBit(GameObjectType::Hazard) |
        typeBit(GameObjectType::Slope) |
        typeBit(GameObjectType::CollisionObject) |
        typeBit(GameObjectType::AnimatedHazard) |
        typeBit(GameObjectType::Modifier) |
        typeBit(GameObjectType::Special);

    struct Stage {
        bool enabled;
        int ticksAll;
        int ticksWave;
        char const* message;
    };

    constexpr Stage STAGES[STAGE_COUNT] = {
        { true,  2,  2, "[InputBuffer] F6 Cycle -> STAGE 1: 2 Ticks All | 2 Ticks Wave" },
        { true,  4,  4, "[InputBuffer] F6 Cycle -> STAGE 2: 4 Ticks All | 4 Ticks Wave" },
        { true,  8,  8, "[InputBuffer] F6 Cycle -> STAGE 3: 8 Ticks All | 8 Ticks Wave" },
        { true, 16,  8, "[InputBuffer] F6 Cycle -> STAGE 4: 16 Ticks All | 8 Ticks Wave" },
        { false, 0,  0, "[InputBuffer] F6 Cycle -> STAGE 5: OFF (Raw Input)" },
    };

    struct Buffer {
        PlayerObject* ghost1 = nullptr;
        PlayerObject* ghost2 = nullptr;
        bool simulating = false;
        bool injecting = false;
        bool ghostDied = false;
        bool realInput = false;
        bool appliedInput = false;
        bool isPlayer1 = true;
        bool enabled = true;
        int stage = 0;
        int ticksAll = 2;
        int ticksWave = 2;
        int pending = -1;
        bool pendingState = true;
        bool releaseQueued = false;
        float tickLength = TICK;
        int sinceInput = 999;
        int rapid = 0;

        // Level detection
        int levelID = 0;
        std::string levelName;
        std::string levelAuthor;

        // Guided replay mode
        bool guidedMode = false;
        const ReplayInput* pattern = nullptr;
        size_t patternSize = 0;
        size_t patternIndex = 0;
        int currentFrame = 0;
        const char* matchedKeyword = nullptr;
    } buf;

    std::string toLower(std::string s) {
        for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return s;
    }

    void resetPending() {
        buf.pending = -1;
        buf.releaseQueued = false;
    }

    void dropGhosts() {
        if (buf.ghost1) { buf.ghost1->release(); buf.ghost1 = nullptr; }
        if (buf.ghost2) { buf.ghost2->release(); buf.ghost2 = nullptr; }
        buf.simulating = false;
        buf.injecting = false;
        buf.rapid = 0;
        resetPending();
    }

    PlayerObject* makeGhost(PlayLayer* layer) {
        auto* ghost = PlayerObject::create(1, 1, layer, layer, true);
        ghost->retain();
        ghost->setPosition(CCPoint(0.f, 105.f));
        ghost->setVisible(false);
        layer->m_objectLayer->addChild(ghost);
        return ghost;
    }

    void sendButton(PlayLayer* layer, bool down) {
        buf.injecting = true;
        buf.appliedInput = down;
        layer->handleButton(down, 1, buf.isPlayer1);
        buf.injecting = false;
    }

    void pressGhost(PlayerObject* ghost, bool down) {
        if (down) ghost->pushButton(PlayerButton::Jump);
        else ghost->releaseButton(PlayerButton::Jump);
    }

    void copyState(PlayerObject* ghost, PlayerObject* real) {
        ghost->setVisible(false);
        ghost->copyAttributes(real);
        ghost->setScale(real->getScale());
        ghost->setContentSize(real->getContentSize());
        ghost->m_width = real->m_width;
        ghost->m_height = real->m_height;
        ghost->m_gravityMod = real->m_gravityMod;
        ghost->m_isOnGround = real->m_isOnGround;
        ghost->m_isUpsideDown = real->m_isUpsideDown;
        ghost->m_vehicleSize = real->m_vehicleSize;
        ghost->m_playerSpeed = real->m_playerSpeed;
    }

    void advanceGhost(PlayerObject* ghost, float dt) {
        ghost->update(dt);
        ghost->updateRotation(dt);
    }

    void checkGhost(PlayLayer* layer, PlayerObject* ghost, float dt) {
        ghost->m_collisionLogTop->removeAllObjects();
        ghost->m_collisionLogBottom->removeAllObjects();
        ghost->m_collisionLogLeft->removeAllObjects();
        ghost->m_collisionLogRight->removeAllObjects();
        layer->checkCollisions(ghost, dt, false);
    }

    bool isFlying(PlayerObject* player) {
        return player->m_isDart || player->m_isBird || player->m_isSwing || player->m_isShip;
    }

    bool isCube(PlayerObject* player) {
        return !player->m_isShip && !player->m_isBird && !player->m_isBall &&
               !player->m_isDart && !player->m_isRobot && !player->m_isSpider &&
               !player->m_isSwing;
    }

    // ===================== GUIDED MODE =====================
    void tryMatchPattern(const std::string& name) {
        buf.guidedMode = false;
        buf.pattern = nullptr;
        buf.patternSize = 0;
        buf.patternIndex = 0;
        buf.matchedKeyword = nullptr;

        std::string lower = toLower(name);
        for (size_t i = 0; i < ALL_PATTERNS_COUNT; ++i) {
            std::string key = ALL_PATTERNS[i].keyword;
            if (lower.find(key) != std::string::npos) {
                buf.guidedMode = true;
                buf.pattern = ALL_PATTERNS[i].data;
                buf.patternSize = ALL_PATTERNS[i].size;
                buf.patternIndex = 0;
                buf.matchedKeyword = ALL_PATTERNS[i].keyword;
                std::printf("[InputBuffer] *** GUIDED MODE ACTIVATED for keyword \"%s\" (%zu inputs) ***\n",
                    buf.matchedKeyword, buf.patternSize);
                return;
            }
        }
        std::puts("[InputBuffer] No matching recording found -> using normal predictive buffer");
    }

    void updateGuided(PlayLayer* layer) {
        if (!buf.guidedMode || !buf.pattern || buf.patternIndex >= buf.patternSize) return;

        // Approximate frame from time (most recordings are 240fps)
        // We use a simple counter that increments every update
        // For better accuracy we also look at m_time
        float time = layer->m_gameState.m_time;
        int approxFrame = static_cast<int>(time * 240.0f); // assume 240 as base

        while (buf.patternIndex < buf.patternSize) {
            const auto& inp = buf.pattern[buf.patternIndex];
            if (approxFrame < inp.frame) break;

            // Apply this input
            sendButton(layer, inp.down);
            std::printf("[Guided] Frame ~%d -> %s (recorded frame %d)\n",
                approxFrame, inp.down ? "PRESS" : "RELEASE", inp.frame);
            ++buf.patternIndex;
        }
    }

    // ===================== PREDICTIVE BUFFER (original + fixes) =====================
    int simulate(PlayLayer* layer, PlayerObject* p1, PlayerObject* p2, bool dual, int delay, bool down, int horizon, bool* diedBeforeInput) {
        *diedBeforeInput = false;
        if (!buf.ghost1) return horizon;
        buf.simulating = true;
        buf.ghostDied = false;

        bool second = dual && buf.ghost2 && p2;
        copyState(buf.ghost1, p1);
        if (second) copyState(buf.ghost2, p2);

        float dt = buf.tickLength;
        if (!(dt > 0.001f && dt < 2.0f)) dt = TICK;

        if (delay > 0) {
            pressGhost(buf.ghost1, !down);
            if (second) pressGhost(buf.ghost2, !down);
        }

        bool flying = isFlying(p1) || (dual && p2 && isFlying(p2));
        int total = delay + horizon;
        if (total <= 0) { buf.simulating = false; return 0; }

        int survived = 0, airTicks = 0, groundTicks = 0, tick = 0;
        bool died = false;

        for (; tick < total; ++tick) {
            if (tick == delay) {
                pressGhost(buf.ghost1, down);
                if (second) pressGhost(buf.ghost2, down);
            }

            advanceGhost(buf.ghost1, dt);
            if (second) advanceGhost(buf.ghost2, dt);
            if (buf.ghostDied) { died = true; break; }

            checkGhost(layer, buf.ghost1, dt);
            if (buf.ghostDied) { died = true; break; }
            if (second) {
                checkGhost(layer, buf.ghost2, dt);
                if (buf.ghostDied) { died = true; break; }
            }

            if (tick < delay) continue;
            ++survived;
            if (flying) continue;

            bool grounded = buf.ghost1->m_isOnGround && (!second || buf.ghost2->m_isOnGround);
            if (!grounded) { ++airTicks; groundTicks = 0; continue; }

            int minAir = isCube(p1) ? 3 : 6;
            if (airTicks < minAir) continue;
            if (groundTicks > 2) { buf.simulating = false; return horizon; }
            ++groundTicks;
        }

        if (died && tick < delay) *diedBeforeInput = true;
        buf.simulating = false;
        return survived;
    }

    bool intercept(GJBaseGameLayer* self, bool down, bool isPlayer1) {
        auto* layer = GameManager::get()->m_playLayer;
        if (!layer) return false;

        // In guided mode we ignore the predictive intercept
        if (buf.guidedMode) return false;

        bool dual = layer->m_gameState.m_isDualMode && layer->m_player2;
        auto* level = layer->m_level;
        if (level && !isPlayer1 && level->m_twoPlayerMode) return false;

        buf.isPlayer1 = isPlayer1;
        buf.realInput = down;
        if (!buf.enabled) { buf.appliedInput = down; return false; }

        auto* p1 = self->m_player1;
        auto* p2 = dual ? layer->m_player2 : nullptr;
        if (!p1 || p1->m_isDead || p1->m_isShip || (p2 && p2->m_isShip)) {
            resetPending();
            buf.appliedInput = down;
            return false;
        }

        if (buf.pending >= 0) {
            buf.releaseQueued = buf.pendingState != down;
            buf.sinceInput = 0;
            return true;
        }
        if (buf.pending == -2) {
            buf.pending = -1;
            buf.releaseQueued = false;
            bool opposite = !buf.pendingState;
            if (buf.appliedInput != opposite) sendButton(layer, opposite);
        }

        bool p1Wave = p1->m_isDart;
        bool anyWave = p1Wave || (p2 && p2->m_isDart);
        bool mini = (p1Wave && p1->m_vehicleSize < 1.f) || (p2 && p2->m_isDart && p2->m_vehicleSize < 1.f);
        bool cubeMode = isCube(p1) && (!p2 || isCube(p2));

        if (buf.sinceInput < 8) {
            int before = buf.rapid++;
            buf.sinceInput = 0;
            if (before >= 2 && anyWave) { buf.appliedInput = down; return false; }
        } else {
            buf.sinceInput = 0;
            buf.rapid = 0;
        }

        if (!anyWave) {
            bool robot = p1->m_isRobot || (p2 && p2->m_isRobot);
            if (robot && !down) { buf.appliedInput = false; return false; }
        }

        char const* name = down ? "PRESS" : "RELEASE";
        buf.releaseQueued = false;

        int horizon = mini ? 28 : (anyWave ? 38 : (cubeMode ? 52 : 48));
        int maxDelay = anyWave ? buf.ticksWave : buf.ticksAll;

        bool diedEarly = false;
        int base = simulate(layer, p1, p2, dual, 0, down, horizon, &diedEarly);
        if (base >= horizon || maxDelay <= 0) { buf.appliedInput = down; return false; }

        int saved = 0, landed = 0;

        if (!anyWave) {
            int target = std::min(horizon, base + (cubeMode ? 8 : 12));
            for (int k = 1; k <= maxDelay; ++k) {
                int ret = simulate(layer, p1, p2, dual, k, down, horizon, &diedEarly);
                if (diedEarly) break;
                if (ret >= target) { saved = k; landed = ret; break; }
            }
        } else {
            int target = base + 10;
            int gain = 6;
            for (int k = 1; k <= maxDelay; ++k) {
                int ret = simulate(layer, p1, p2, dual, k, down, horizon, &diedEarly);
                if (diedEarly) break;
                bool good = ret < horizon ? ret >= target : (ret - base >= gain || ret >= target);
                if (good) { saved = k; landed = ret; break; }
                target += 2; gain += 2;
            }
        }

        if (saved <= 0) { buf.appliedInput = down; return false; }

        std::printf("[InputBuffer] *** SAVED +%d TICKS %s! (Level: %s) ***\n",
            saved, name, buf.levelName.c_str());
        buf.pendingState = down;
        buf.pending = saved;
        return true;
    }

    void cycleStage() {
        buf.stage = (buf.stage + 1) % STAGE_COUNT;
        buf.pending = -1;
        buf.releaseQueued = false;
        auto const& stage = STAGES[buf.stage];
        buf.enabled = stage.enabled;
        buf.ticksAll = stage.ticksAll;
        buf.ticksWave = stage.ticksWave;
        std::puts(stage.message);
    }

    void deliverPending(PlayLayer* layer, PlayerObject* player) {
        if (buf.sinceInput <= 998) ++buf.sinceInput;

        bool dual = layer->m_gameState.m_isDualMode && layer->m_player2;
        if (player->m_isShip || (dual && layer->m_player2 && layer->m_player2->m_isShip)) {
            resetPending();
            if (buf.appliedInput != buf.realInput) sendButton(layer, buf.realInput);
            return;
        }

        if (buf.pending == 0) {
            sendButton(layer, buf.pendingState);
            if (buf.releaseQueued) { buf.pending = -2; buf.releaseQueued = false; }
            else buf.pending = -1;
        } else if (buf.pending > 0) {
            --buf.pending;
        } else if (buf.pending == -1) {
            if (buf.appliedInput != buf.realInput) sendButton(layer, buf.realInput);
        } else if (buf.pending == -2) {
            sendButton(layer, !buf.pendingState);
            buf.releaseQueued = false;
            buf.pending = -1;
        }
    }

    void detectLevel(GJGameLevel* level) {
        if (!level) {
            buf.levelID = 0;
            buf.levelName = "Unknown";
            buf.levelAuthor = "";
            buf.guidedMode = false;
            return;
        }

        buf.levelID = level->m_levelID;
        buf.levelName = level->m_levelName;
        buf.levelAuthor = level->m_creatorName;

        std::printf("============================================================\n");
        std::printf("[InputBuffer] LEVEL DETECTED: \"%s\" by %s (ID %d)\n",
            buf.levelName.c_str(),
            buf.levelAuthor.empty() ? "(none)" : buf.levelAuthor.c_str(),
            buf.levelID);
        std::printf("============================================================\n");

        tryMatchPattern(buf.levelName);
    }
}

$on_mod(Loaded) {
    AllocConsole();
    FILE* out = nullptr;
    freopen_s(&out, "CONOUT$", "w", stdout);
    SetConsoleTitleA("Geode Predictive Input Buffer + Guided Replays");
    std::puts("============================================================");
    std::puts(" [InputBuffer] v5.0 LOADED");
    std::puts(" Features:");
    std::puts("  - Level auto-detect + partial name matching");
    std::puts("  - Guided mode for: grief, andromeda, slaughter, solar, vortex, society");
    std::puts("  - Improved cube spike buffering");
    std::puts("  - F6 to cycle buffer stages");
    std::puts("============================================================");
}

class $modify(BufPlayLayer, PlayLayer) {
    static void onModify(auto& self) {
        (void)self.setHookPriority("PlayLayer::destroyPlayer", HOOK_PRIORITY);
        (void)self.setHookPriority("PlayLayer::playEndAnimationToPos", HOOK_PRIORITY);
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        if (buf.simulating && (player == buf.ghost1 || player == buf.ghost2)) {
            buf.ghostDied = true;
            return;
        }
        buf.pending = -1;
        buf.releaseQueued = false;
        buf.appliedInput = false;
        buf.rapid = 0;
        PlayLayer::destroyPlayer(player, object);
    }

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

        detectLevel(level);

        auto* layer = GameManager::get()->m_playLayer;
        if (!layer) return true;

        buf.ghost1 = makeGhost(layer);
        buf.ghost2 = makeGhost(layer);
        buf.pending = -1;
        buf.releaseQueued = false;
        buf.realInput = false;
        buf.appliedInput = false;
        buf.isPlayer1 = true;
        buf.simulating = false;
        buf.injecting = false;
        buf.sinceInput = 999;
        buf.rapid = 0;
        buf.currentFrame = 0;
        buf.patternIndex = 0;
        return true;
    }

    void onQuit() {
        PlayLayer::onQuit();
        dropGhosts();
        buf.levelID = 0;
        buf.levelName.clear();
        buf.guidedMode = false;
        buf.pattern = nullptr;
    }

    void playEndAnimationToPos(CCPoint position) {
        if (buf.simulating) return;
        PlayLayer::playEndAnimationToPos(position);
    }
};

class $modify(BufEditorLayer, LevelEditorLayer) {
    bool init(GJGameLevel* level, bool noUI) {
        bool ok = LevelEditorLayer::init(level, noUI);
        dropGhosts();
        return ok;
    }
};

class $modify(BufHardStreak, HardStreak) {
    void addPoint(CCPoint point) {
        if (buf.simulating) return;
        HardStreak::addPoint(point);
    }
};

class $modify(BufEffectGameObject, EffectGameObject) {
    void triggerObject(GJBaseGameLayer* layer, int uniqueID, gd::vector<int> const* remapKeys) {
        if (buf.simulating) return;
        EffectGameObject::triggerObject(layer, uniqueID, remapKeys);
    }
};

class $modify(BufGameObject, GameObject) {
    void playShineEffect() {
        if (buf.simulating) return;
        GameObject::playShineEffect();
    }
};

class $modify(BufPlayerObject, PlayerObject) {
    static void onModify(auto& self) {
        (void)self.setHookPriority("PlayerObject::playSpiderDashEffect", HOOK_PRIORITY);
        (void)self.setHookPriority("PlayerObject::incrementJumps", HOOK_PRIORITY);
        (void)self.setHookPriority("PlayerObject::ringJump", HOOK_PRIORITY);
    }

    void update(float dt) {
        if (!buf.simulating) {
            auto* layer = GameManager::get()->m_playLayer;
            if (layer && this == layer->m_player1) {
                if (GetAsyncKeyState(VK_F6) & 1) cycleStage();

                if (buf.guidedMode) {
                    updateGuided(layer);
                } else if (buf.enabled) {
                    deliverPending(layer, this);
                }
            }
        }

        PlayerObject::update(dt);

        auto* layer = GameManager::get()->m_playLayer;
        if (layer && !buf.simulating) {
            float warp = layer->m_gameState.m_timeWarp;
            if (warp > 0.f) dt /= warp;
            buf.tickLength = dt;
        }
    }

    void incrementJumps() {
        if (buf.simulating) return;
        PlayerObject::incrementJumps();
    }

    void playSpiderDashEffect(CCPoint from, CCPoint to) {
        if (buf.simulating) return;
        PlayerObject::playSpiderDashEffect(from, to);
    }

    void ringJump(RingObject* ring, bool skipCheck) {
        if (buf.simulating) return;
        PlayerObject::ringJump(ring, skipCheck);
    }
};

class $modify(BufBaseGameLayer, GJBaseGameLayer) {
    static void onModify(auto& self) {
        (void)self.setHookPriority("GJBaseGameLayer::canBeActivatedByPlayer", HOOK_PRIORITY);
        (void)self.setHookPriority("GJBaseGameLayer::handleButton", HOOK_PRIORITY);
        (void)self.setHookPriority("GJBaseGameLayer::collisionCheckObjects", HOOK_PRIORITY);
        (void)self.setHookPriority("GJBaseGameLayer::playerTouchedRing", HOOK_PRIORITY);
    }

    bool canBeActivatedByPlayer(PlayerObject* player, EffectGameObject* object) {
        if (buf.simulating) return false;
        return GJBaseGameLayer::canBeActivatedByPlayer(player, object);
    }

    void collisionCheckObjects(PlayerObject* player, gd::vector<GameObject*>* objects, int objectCount, float dt) {
        if (!buf.simulating) {
            GJBaseGameLayer::collisionCheckObjects(player, objects, objectCount, dt);
            return;
        }
        static gd::vector<GameObject*> relevant;
        relevant.clear();
        if (static_cast<int>(relevant.capacity()) < objectCount) relevant.reserve(objectCount);
        for (int i = 0; i < objectCount; ++i) {
            auto* object = objects->at(i);
            if (!object) continue;
            auto type = static_cast<unsigned>(object->m_objectType);
            if (type <= 60 && ((COLLIDING_TYPES >> type) & 1)) {
                relevant.push_back(object);
            } else if (object->m_objectID >= 39 && object->m_objectID <= 200) {
                relevant.push_back(object);
            }
        }
        GJBaseGameLayer::collisionCheckObjects(player, &relevant, static_cast<int>(relevant.size()), dt);
    }

    void handleButton(bool down, int button, bool isPlayer1) {
        if (!buf.simulating && button == 1 && !buf.injecting && intercept(this, down, isPlayer1)) {
            return;
        }
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }

    void playerTouchedRing(PlayerObject* player, RingObject* ring) {
        if (buf.simulating) return;
        GJBaseGameLayer::playerTouchedRing(player, ring);
    }
};
