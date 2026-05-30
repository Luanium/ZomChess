#ifndef SPLASHSCREEN_H
#define SPLASHSCREEN_H

#include <SFML/Graphics.hpp>
#include <cmath>
#include <string>
#include <vector>
#include <random>

// ============================================================
//  ZomChess Welcome Flash Screen
//  Pure SFML rendering — no ImGui dependency
// ============================================================

class SplashScreen {
public:
    // Returns true while the splash is still running, false when done
    bool update(float dt) {
        elapsed += dt;

        // Handle skip on any key/click
        if (skip_requested) {
            fade_out_timer += dt;
            if (fade_out_timer >= FADE_OUT_DURATION) return false;
        } else {
            if (elapsed >= DISPLAY_DURATION) skip_requested = true;
        }

        // Particle update
        for (auto& p : particles) {
            p.x += p.vx * dt;
            p.y += p.vy * dt;
            p.life -= dt;
            p.vy -= 18.0f * dt; // slight upward drift
            if (p.life <= 0.0f) respawn_particle(p);
        }

        return true;
    }

    void handle_event(const sf::Event& e) {
        if (e.type == sf::Event::KeyPressed ||
            e.type == sf::Event::MouseButtonPressed ||
            e.type == sf::Event::JoystickButtonPressed) {
            skip_requested = true;
        }
    }

    void draw(sf::RenderWindow& window, const sf::Font& font) {
        const float W = static_cast<float>(window.getSize().x);
        const float H = static_cast<float>(window.getSize().y);

        // ── Global alpha (fade in / fade out) ──────────────────────────
        float alpha = 1.0f;
        if (elapsed < FADE_IN_DURATION) {
            alpha = elapsed / FADE_IN_DURATION;
        } else if (skip_requested) {
            alpha = 1.0f - (fade_out_timer / FADE_OUT_DURATION);
        }
        alpha = std::max(0.0f, std::min(1.0f, alpha));
        sf::Uint8 a = static_cast<sf::Uint8>(alpha * 255);

        // ── Background ─────────────────────────────────────────────────
        // Deep dark with a subtle radial vignette drawn via layered rects
        sf::RectangleShape bg(sf::Vector2f(W, H));
        bg.setFillColor(sf::Color(10, 11, 14, a));
        window.draw(bg);

        // Radial glow behind title (large soft circle)
        draw_radial_glow(window, W * 0.5f, H * 0.42f, 340.0f, sf::Color(0, 200, 180, static_cast<sf::Uint8>(40 * alpha)), 8);

        // ── Floating particles ─────────────────────────────────────────
        for (const auto& p : particles) {
            float life_ratio = p.life / p.max_life;
            sf::Uint8 pa = static_cast<sf::Uint8>(life_ratio * p.base_alpha * alpha);
            sf::CircleShape dot(p.radius);
            dot.setPosition(p.x - p.radius, p.y - p.radius);
            dot.setFillColor(sf::Color(p.r, p.g, p.b, pa));
            window.draw(dot);
        }

        // ── Decorative horizontal lines ────────────────────────────────
        float line_alpha_f = 0.35f * alpha;
        draw_h_line(window, W, H * 0.30f, sf::Color(0, 220, 200, static_cast<sf::Uint8>(line_alpha_f * 255)));
        draw_h_line(window, W, H * 0.72f, sf::Color(0, 220, 200, static_cast<sf::Uint8>(line_alpha_f * 255)));

        // ── Title "ZomChess" — two-tone, rendered as one seamless string ─
        // We draw "Zom" and "Chess" side-by-side at the same baseline so
        // there is zero gap between them, each with its own pulsing color.
        {
            float t = elapsed;

            // Build both sf::Text objects first so we can measure widths
            float pulse_zom   = 0.85f + 0.15f * std::sin(t * 2.8f);
            float pulse_chess = 0.80f + 0.20f * std::sin(t * 3.1f + 1.2f);

            sf::Text zom, chess;
            zom.setFont(font);   zom.setString("Zom");
            chess.setFont(font); chess.setString("Chess");
            const unsigned int TITLE_SIZE = 110;
            zom.setCharacterSize(TITLE_SIZE);
            chess.setCharacterSize(TITLE_SIZE);
            zom.setStyle(sf::Text::Bold);
            chess.setStyle(sf::Text::Bold);

            // Measure widths (use advance-based width via bounds)
            float zom_w   = zom.getLocalBounds().left   + zom.getLocalBounds().width;
            float chess_w = chess.getLocalBounds().left + chess.getLocalBounds().width;
            float total_w = zom_w + chess_w;

            // Center the combined block
            float start_x = W * 0.5f - total_w * 0.5f;
            float title_y = H * 0.28f - TITLE_SIZE * 0.5f; // top-left y

            // Position each part
            zom.setPosition(start_x, title_y);
            chess.setPosition(start_x + zom_w, title_y);

            // Colors
            zom.setFillColor(sf::Color(
                static_cast<sf::Uint8>(220 * pulse_zom),
                static_cast<sf::Uint8>(30  * pulse_zom),
                static_cast<sf::Uint8>(30  * pulse_zom), a));
            chess.setFillColor(sf::Color(
                0,
                static_cast<sf::Uint8>(220 * pulse_chess),
                static_cast<sf::Uint8>(200 * pulse_chess), a));

            // Glow layers — draw behind both parts
            for (int layer = 3; layer >= 1; --layer) {
                float spread = layer * 5.0f;
                sf::Text gz = zom, gc = chess;
                gz.setFillColor(sf::Color(180, 0, 0,   static_cast<sf::Uint8>(16 * alpha)));
                gc.setFillColor(sf::Color(0, 180, 160, static_cast<sf::Uint8>(16 * alpha)));
                gz.setPosition(zom.getPosition().x   - spread, zom.getPosition().y   - spread * 0.4f);
                gc.setPosition(chess.getPosition().x - spread, chess.getPosition().y - spread * 0.4f);
                window.draw(gz); window.draw(gc);
                gz.setPosition(zom.getPosition().x   + spread, zom.getPosition().y   - spread * 0.4f);
                gc.setPosition(chess.getPosition().x + spread, chess.getPosition().y - spread * 0.4f);
                window.draw(gz); window.draw(gc);
            }
            window.draw(zom);
            window.draw(chess);
        }

        // ── Rainbow halo cycling around the full title ─────────────────
        draw_title_halo(window, font, W, H, alpha);

        // ── Tagline ────────────────────────────────────────────────────
        {
            float t = elapsed;
            float flicker = 0.75f + 0.25f * std::sin(t * 1.7f);
            sf::Uint8 ta = static_cast<sf::Uint8>(flicker * alpha * 200);

            sf::Text tag;
            tag.setFont(font);
            tag.setString("EVERY WRONG MOVE LEADS YOU TO DEATH!");
            tag.setCharacterSize(22);
            tag.setLetterSpacing(2.5f);
            tag.setFillColor(sf::Color(180, 180, 180, ta));
            center_text(tag, W, H * 0.52f);
            window.draw(tag);
        }

        // ── Separator ornament ─────────────────────────────────────────
        {
            float orn_alpha = alpha * 0.6f;
            sf::Text orn;
            orn.setFont(font);
            orn.setString("- - - - - - - - - - - - - - - - - - - -");
            orn.setCharacterSize(14);
            orn.setFillColor(sf::Color(0, 180, 160, static_cast<sf::Uint8>(orn_alpha * 255)));
            center_text(orn, W, H * 0.575f);
            window.draw(orn);
        }

        // ── "Created by" credit ────────────────────────────────────────
        {
            sf::Text credit;
            credit.setFont(font);
            credit.setString("Created by:  Phan Anh Luan");
            credit.setCharacterSize(26);
            credit.setStyle(sf::Text::Bold);
            // Soft gold color
            float t = elapsed;
            float shimmer = 0.88f + 0.12f * std::sin(t * 2.2f + 0.5f);
            credit.setFillColor(sf::Color(
                static_cast<sf::Uint8>(255 * shimmer),
                static_cast<sf::Uint8>(210 * shimmer),
                static_cast<sf::Uint8>(80  * shimmer),
                a));
            center_text(credit, W, H * 0.625f);
            window.draw(credit);
        }

        // ── Music credit ───────────────────────────────────────────────
        {
            sf::Text music_credit;
            music_credit.setFont(font);
            music_credit.setString("Music: Kevin MacLeod (incompetech.com) — CC BY 4.0");
            music_credit.setCharacterSize(14);
            music_credit.setFillColor(sf::Color(140, 140, 140, a));
            center_text(music_credit, W, H * 0.665f);
            window.draw(music_credit);
        }

        // ── Version badge ──────────────────────────────────────────────
        {
            sf::Text ver;
            ver.setFont(font);
            ver.setString("v1.3.0");
            ver.setCharacterSize(18);
            ver.setFillColor(sf::Color(100, 100, 100, a));
            ver.setPosition(W - 60.0f, H - 30.0f);
            window.draw(ver);
        }

        // ── "Press any key" prompt ─────────────────────────────────────
        {
            float blink = 0.5f + 0.5f * std::sin(elapsed * 3.5f);
            sf::Uint8 ba = static_cast<sf::Uint8>(blink * alpha * 200);
            sf::Text prompt;
            prompt.setFont(font);
            prompt.setString("Press any key to continue...");
            prompt.setCharacterSize(20);
            prompt.setFillColor(sf::Color(140, 140, 140, ba));
            center_text(prompt, W, H * 0.84f);
            window.draw(prompt);
        }

        // ── Corner skull ornaments ─────────────────────────────────────
        draw_corner_ornaments(window, font, W, H, a);
    }

    // Call once before the splash loop to seed particles
    void init(unsigned int win_w, unsigned int win_h) {
        width  = static_cast<float>(win_w);
        height = static_cast<float>(win_h);
        std::mt19937 rng(42);
        std::uniform_real_distribution<float> rx(0.0f, width);
        std::uniform_real_distribution<float> ry(0.0f, height);
        std::uniform_real_distribution<float> rv(-12.0f, 12.0f);
        std::uniform_real_distribution<float> rlife(1.5f, 5.0f);
        std::uniform_real_distribution<float> rrad(1.0f, 3.5f);
        std::uniform_int_distribution<int>    rcol(0, 2);

        particles.resize(PARTICLE_COUNT);
        for (auto& p : particles) {
            p.x = rx(rng); p.y = ry(rng);
            p.vx = rv(rng); p.vy = rv(rng);
            float life = rlife(rng);
            p.life = life; p.max_life = life;
            p.radius = rrad(rng);
            int col = rcol(rng);
            if (col == 0) { p.r = 0;   p.g = 220; p.b = 200; p.base_alpha = 180; } // cyan
            else if (col == 1) { p.r = 200; p.g = 30;  p.b = 30;  p.base_alpha = 160; } // red
            else               { p.r = 255; p.g = 200; p.b = 60;  p.base_alpha = 140; } // gold
        }
        rng_engine = rng;
    }

private:
    // ── Timing constants ───────────────────────────────────────────────
    static constexpr float DISPLAY_DURATION  = 6.0f;  // seconds before auto-skip
    static constexpr float FADE_IN_DURATION  = 1.2f;
    static constexpr float FADE_OUT_DURATION = 0.8f;
    static constexpr int   PARTICLE_COUNT    = 80;

    float elapsed        = 0.0f;
    float fade_out_timer = 0.0f;
    bool  skip_requested = false;
    float width = 1400.0f, height = 658.0f;

    // ── Particle system ────────────────────────────────────────────────
    struct Particle {
        float x, y, vx, vy;
        float life, max_life, radius;
        sf::Uint8 r, g, b, base_alpha;
    };
    std::vector<Particle> particles;
    std::mt19937 rng_engine;

    void respawn_particle(Particle& p) {
        std::uniform_real_distribution<float> rx(0.0f, width);
        std::uniform_real_distribution<float> ry(0.0f, height);
        std::uniform_real_distribution<float> rv(-12.0f, 12.0f);
        std::uniform_real_distribution<float> rlife(1.5f, 5.0f);
        std::uniform_real_distribution<float> rrad(1.0f, 3.5f);
        std::uniform_int_distribution<int>    rcol(0, 2);
        p.x = rx(rng_engine); p.y = ry(rng_engine);
        p.vx = rv(rng_engine); p.vy = rv(rng_engine);
        float life = rlife(rng_engine);
        p.life = life; p.max_life = life;
        p.radius = rrad(rng_engine);
        int col = rcol(rng_engine);
        if (col == 0) { p.r = 0;   p.g = 220; p.b = 200; p.base_alpha = 180; }
        else if (col == 1) { p.r = 200; p.g = 30;  p.b = 30;  p.base_alpha = 160; }
        else               { p.r = 255; p.g = 200; p.b = 60;  p.base_alpha = 140; }
    }

    // ── Helpers ────────────────────────────────────────────────────────

    // Center text horizontally, with optional x offset from center
    void center_text_x(sf::Text& text, float W, float y, float x_offset = 0.0f) {
        sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin(bounds.left + bounds.width * 0.5f, bounds.top + bounds.height * 0.5f);
        text.setPosition(W * 0.5f + x_offset, y);
    }

    void center_text(sf::Text& text, float W, float y) {
        sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin(bounds.left + bounds.width * 0.5f, bounds.top + bounds.height * 0.5f);
        text.setPosition(W * 0.5f, y);
    }

    // Soft radial glow via layered transparent circles
    void draw_radial_glow(sf::RenderWindow& window, float cx, float cy,
                          float max_r, sf::Color color, int layers) {
        for (int i = layers; i >= 1; --i) {
            float r = max_r * (static_cast<float>(i) / layers);
            sf::CircleShape circle(r);
            circle.setOrigin(r, r);
            circle.setPosition(cx, cy);
            sf::Uint8 layer_a = static_cast<sf::Uint8>(color.a * (1.0f - static_cast<float>(i - 1) / layers));
            circle.setFillColor(sf::Color(color.r, color.g, color.b, layer_a));
            window.draw(circle);
        }
    }

    void draw_h_line(sf::RenderWindow& window, float W, float y, sf::Color color) {
        sf::RectangleShape line(sf::Vector2f(W * 0.7f, 1.0f));
        line.setFillColor(color);
        line.setPosition(W * 0.15f, y);
        window.draw(line);
    }

    // Animated rainbow halo orbiting the title area
    void draw_title_halo(sf::RenderWindow& window, const sf::Font& /*font*/,
                         float W, float H, float alpha) {
        // Orbit an ellipse centered on the title block
        const int DOT_COUNT = 16;
        float cx = W * 0.5f;
        float cy = H * 0.36f;
        float rx = 310.0f;
        float ry = 80.0f;

        for (int i = 0; i < DOT_COUNT; ++i) {
            float angle = (static_cast<float>(i) / DOT_COUNT) * 2.0f * 3.14159f
                          + elapsed * 0.6f;
            float px = cx + rx * std::cos(angle);
            float py = cy + ry * std::sin(angle);

            // Hue cycling
            float hue = std::fmod(static_cast<float>(i) / DOT_COUNT + elapsed * 0.15f, 1.0f);
            sf::Color dot_color = hsv_to_rgb(hue, 1.0f, 1.0f);
            dot_color.a = static_cast<sf::Uint8>(alpha * 200);

            float dot_r = 3.5f + 1.5f * std::sin(elapsed * 4.0f + i);
            sf::CircleShape dot(dot_r);
            dot.setOrigin(dot_r, dot_r);
            dot.setPosition(px, py);
            dot.setFillColor(dot_color);
            window.draw(dot);

            // Small trail
            sf::Color trail = dot_color;
            trail.a = static_cast<sf::Uint8>(alpha * 60);
            sf::CircleShape trail_dot(dot_r * 0.6f);
            trail_dot.setOrigin(dot_r * 0.6f, dot_r * 0.6f);
            float trail_angle = angle - 0.25f;
            trail_dot.setPosition(cx + rx * std::cos(trail_angle),
                                  cy + ry * std::sin(trail_angle));
            trail_dot.setFillColor(trail);
            window.draw(trail_dot);
        }
    }

    void draw_corner_ornaments(sf::RenderWindow& window, const sf::Font& font,
                               float W, float H, sf::Uint8 a) {
        // Skull-like unicode char at each corner
        const char* skull = "\u2620"; // ☠
        const unsigned int sz = 28;
        float margin = 18.0f;

        auto make_skull = [&](float x, float y) {
            sf::Text t;
            t.setFont(font);
            t.setString(skull);
            t.setCharacterSize(sz);
            t.setFillColor(sf::Color(80, 80, 80, a));
            t.setPosition(x, y);
            window.draw(t);
        };

        make_skull(margin,          margin);
        make_skull(W - margin - sz, margin);
        make_skull(margin,          H - margin - sz);
        make_skull(W - margin - sz, H - margin - sz);
    }

    // Simple HSV → RGB conversion (h in [0,1], s/v in [0,1])
    static sf::Color hsv_to_rgb(float h, float s, float v) {
        float r = 0, g = 0, b = 0;
        int i = static_cast<int>(h * 6.0f);
        float f = h * 6.0f - i;
        float p = v * (1.0f - s);
        float q = v * (1.0f - f * s);
        float t = v * (1.0f - (1.0f - f) * s);
        switch (i % 6) {
            case 0: r = v; g = t; b = p; break;
            case 1: r = q; g = v; b = p; break;
            case 2: r = p; g = v; b = t; break;
            case 3: r = p; g = q; b = v; break;
            case 4: r = t; g = p; b = v; break;
            case 5: r = v; g = p; b = q; break;
        }
        return sf::Color(
            static_cast<sf::Uint8>(r * 255),
            static_cast<sf::Uint8>(g * 255),
            static_cast<sf::Uint8>(b * 255));
    }
};

#endif // SPLASHSCREEN_H
