// The skybox catalog: the six-face skies shipped as presets.
//
// Generated once and kept as data - do not edit by hand.
//
// Every entry here is a sky someone confirmed against a live client, which is
// the whole point: the previous catalog came from a text file, most of its ids
// never resolved, and a preset that resolves to nothing is indistinguishable
// from a skybox feature that does not work.  Names are positional because the
// data does not name them.

#pragma once

namespace sky_presets
{
    struct preset_t
    {
        const char* name;
        const char* bk;     // rbxassetid://...
        const char* dn;
        const char* ft;
        const char* lf;
        const char* rt;
        const char* up;
        const char* sun;    // "" - the sources set no sun texture
        const char* moon;   // ""
        int         stars;  // -1 - the sources set no star count
        float       sun_ang;    // -1 - and no angular sizes
        float       moon_ang;
    };

    inline constexpr preset_t k_presets[] =
    {
		{ "Sky 1", "rbxassetid://12635309703", "rbxassetid://12635311686", "rbxassetid://12635312870", "rbxassetid://12635313718", "rbxassetid://12635315817", "rbxassetid://12635316856", "", "", -1, -1.0f, -1.0f },
		{ "Sky 2", "rbxassetid://12064107", "rbxassetid://12064152", "rbxassetid://12064121", "rbxassetid://12063984", "rbxassetid://12064115", "rbxassetid://12064131", "", "", -1, -1.0f, -1.0f },
		{ "Sky 3", "rbxassetid://271042516", "rbxassetid://271077243", "rbxassetid://271042556", "rbxassetid://271042310", "rbxassetid://271042467", "rbxassetid://271077958", "", "", -1, -1.0f, -1.0f },
		{ "Sky 4", "rbxassetid://1876545003", "rbxassetid://1876544331", "rbxassetid://1876542941", "rbxassetid://1876543392", "rbxassetid://1876543764", "rbxassetid://1876544642", "", "", -1, -1.0f, -1.0f },
		{ "Sky 5", "rbxassetid://116758234", "rbxassetid://116758314", "rbxassetid://116758367", "rbxassetid://116758446", "rbxassetid://116758478", "rbxassetid://116758496", "", "", -1, -1.0f, -1.0f },
		{ "Sky 6", "rbxassetid://1233158420", "rbxassetid://1233158838", "rbxassetid://1233157105", "rbxassetid://1233157640", "rbxassetid://1233157995", "rbxassetid://1233159158", "", "", -1, -1.0f, -1.0f },
		{ "Sky 7", "rbxassetid://1327358", "rbxassetid://1327359", "rbxassetid://1327355", "rbxassetid://1327357", "rbxassetid://1327356", "rbxassetid://1327360", "", "", -1, -1.0f, -1.0f },
		{ "Sky 8", "rbxassetid://570555736", "rbxassetid://570555964", "rbxassetid://570555800", "rbxassetid://570555840", "rbxassetid://570555882", "rbxassetid://570555929", "", "", -1, -1.0f, -1.0f },
		{ "Sky 9", "rbxassetid://95020137072033", "rbxassetid://92862258103959", "rbxassetid://107665368823185", "rbxassetid://126542804346203", "rbxassetid://103716549795832", "rbxassetid://131036626982613", "", "", -1, -1.0f, -1.0f },
		{ "Sky 10", "rbxassetid://169210090", "rbxassetid://169210108", "rbxassetid://169210121", "rbxassetid://169210133", "rbxassetid://169210143", "rbxassetid://169210149", "", "", -1, -1.0f, -1.0f },
		{ "Sky 11", "rbxassetid://47974894", "rbxassetid://47974690", "rbxassetid://47974821", "rbxassetid://47974776", "rbxassetid://47974859", "rbxassetid://47974909", "", "", -1, -1.0f, -1.0f },
    };

    inline constexpr int k_count = (int)(sizeof(k_presets) / sizeof(k_presets[0]));

    inline int index_of(const char* name)
    {
        if (!name || !name[0]) return -1;
        for (int i = 0; i < k_count; ++i)
        {
            const char* a = k_presets[i].name;
            const char* b = name;
            while (*a && *b && *a == *b) { ++a; ++b; }
            if (!*a && !*b) return i;
        }
        return -1;
    }
}
