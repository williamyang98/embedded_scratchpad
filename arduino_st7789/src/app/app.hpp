#pragma once
#include "./weather_page.hpp"
#include "./landing_page.hpp"

constexpr uint8_t TOTAL_APP_PAGES = 2;
enum class AppPage: uint8_t {
    LANDING_SCREEN = 0,
    WEATHER_PAGE = 1,
};

struct App {
private:
    AppPage m_current_page = AppPage::LANDING_SCREEN;
    LandingPage m_landing_page;
    WeatherPage m_weather_page;
public:
    App() {}
    void render_all() {
        render_or_mark(m_landing_page, AppPage::LANDING_SCREEN);
        render_or_mark(m_weather_page, AppPage::WEATHER_PAGE);
    }
    void set_page(AppPage page) {
        m_current_page = page;
    }
    WeatherPage& get_weather_page() { return m_weather_page; }
private:
    template <typename P>
    void render_or_mark(P& page, const AppPage page_type) {
        if (page_type != m_current_page) {
            page.mark_for_full_rerender();
        } else {
            page.render_all();
        }
    }
};

extern App g_app;
