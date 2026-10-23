#pragma once

namespace world_effects
{
    void run();

    void render(float screen_w, float screen_h);

    void update_rain_sound();

    
    
    
    
    
    bool lighting_found();

    bool atmosphere_found();

    
    
    
    
    
    
    
    
    bool invalidate_ready();

    // Anything that writes the Lighting or the Sky under it has to say so, or
    // the client keeps drawing the frame it built before the write: the world
    // tab and the skybox presets share this one invalidation rather than each
    // growing its own.
    void notify_lighting_changed();

    // The Sky the client draws is an environment it builds once and marks
    // current with a flag of its own, so a write into the Sky's fields is only
    // seen once that flag is cleared.  The world tab's own sky rows have always
    // done this; the preset table writes the same instance, so it says so here
    // too.
    void notify_sky_changed();

    // True while the atmosphere override is standing in through the Lighting's
    // fog because the place has no Atmosphere instance of its own.
    bool atmosphere_on_fog();
}
