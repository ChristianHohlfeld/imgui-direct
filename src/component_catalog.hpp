#pragma once
// Visual browse-first demo: a native ImGui catalogue, not a CSS/DOM site.
// This is demo-only: it is not included in the embeddable UI library.
namespace gallery {
    // Return true when the user opened an interactive component inspector.
    // outIndex is always a valid review::component index on success.
    // Render desktop navigation outside the scrollable preview canvas.
    void draw_sidebar();
    bool draw_index(int* outIndex,bool sidebarExternal=false);
}
