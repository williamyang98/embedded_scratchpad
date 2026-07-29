#pragma once

class Page {
public:
    void mark_for_full_rerender() = delete;
    void render_all() = delete;
};
