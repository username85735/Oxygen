#include <tice.h>
#include <graphx.h>
#include <keypadc.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "oxygen/oxygen.h"

// Chart data (now dynamic!)
#define MAX_BARS 12
static uint8_t num_bars = 6;
static char labels[MAX_BARS][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
static uint8_t data[MAX_BARS] = {45, 72, 38, 91, 55, 68, 60, 82, 48, 75, 65, 88};
static bool axis_break_enabled = false;

// Chart dimensions
#define CHART_X 30
#define CHART_Y 35
#define CHART_WIDTH 260
#define CHART_HEIGHT 160
#define BAR_SPACING 4

// Color scheme (TI-84 Plus CE palette indices)
#define COLOR_BACKGROUND 255    // White
#define COLOR_TITLE 0           // Black
#define COLOR_AXIS 1            // Dark gray
#define COLOR_LABEL 0           // Black
#define COLOR_GRID 252          // Light gray

// Pastel color palette for bars (distinct, vibrant pastels)
static const uint8_t bar_colors[] = {
    224,  // Light blue
    227,  // Coral/salmon
    151,  // Mint green
    220,  // Lavender
    228,  // Peach
    148   // Sky blue
};

static const uint8_t bar_outline_colors[] = {
    18,   // Dark blue
    19,   // Dark coral
    4,    // Dark green
    17,   // Dark purple
    20,   // Dark orange
    18    // Dark blue
};

static void randomize_data(void)
{
    // Generate random values between 10 and 100
    for (uint8_t i = 0; i < MAX_BARS; i++) {
        data[i] = 10 + (rand() % 91);
    }
}

static void draw_zigzag(uint16_t x, uint8_t y, uint8_t width)
{
    // Draw zigzag axis break indicator (proper zigzag pattern)
    gfx_SetColor(COLOR_AXIS);
    uint8_t step = 0;
    for (uint8_t i = 0; i < width; i += 2) {
        if (step % 2 == 0) {
            gfx_Line(x + i, y, x + i + 2, y + 4);  // Down-right
        } else {
            gfx_Line(x + i, y + 4, x + i + 2, y);  // Up-right
        }
        step++;
    }
}

static void draw_chart_frame(void)
{
    // Draw title
    gfx_SetTextFGColor(COLOR_TITLE);
    gfx_SetTextBGColor(COLOR_BACKGROUND);
    gfx_SetTextScale(2, 2);
    gfx_PrintStringXY("Sales Data", (LCD_WIDTH - 10 * 8 * 2) / 2, 8);
    gfx_SetTextScale(1, 1);

    // Draw axis lines
    gfx_SetColor(COLOR_AXIS);
    gfx_HorizLine(CHART_X, CHART_Y + CHART_HEIGHT, CHART_WIDTH);  // X-axis

    // Y-axis with optional break
    if (axis_break_enabled) {
        // Draw Y-axis in two segments with zigzag break
        uint8_t break_y = CHART_Y + 20;
        gfx_VertLine(CHART_X, CHART_Y, 15);  // Top segment
        gfx_VertLine(CHART_X, break_y + 10, CHART_HEIGHT - 30);  // Bottom segment
        draw_zigzag(CHART_X - 2, break_y, 5);  // Zigzag indicator
    } else {
        gfx_VertLine(CHART_X, CHART_Y, CHART_HEIGHT);  // Full Y-axis
    }

    // Draw horizontal grid lines
    gfx_SetColor(COLOR_GRID);
    for (uint8_t i = 1; i <= 4; i++) {
        uint8_t y = CHART_Y + (CHART_HEIGHT * i) / 5;
        // Skip grid line in break area if enabled
        if (!axis_break_enabled || y < CHART_Y + 20 || y > CHART_Y + 30) {
            for (uint16_t x = CHART_X + 1; x < CHART_X + CHART_WIDTH; x += 3) {
                gfx_SetPixel(x, y);
            }
        }
    }

    // Draw Y-axis labels
    gfx_SetTextFGColor(COLOR_LABEL);
    char buffer[4];
    if (axis_break_enabled) {
        // Compressed scale: 0-40, break, 60-100
        const uint8_t labels_vals[] = {100, 80, 40, 20, 0};
        for (uint8_t i = 0; i < 5; i++) {
            uint8_t y = CHART_Y + (CHART_HEIGHT * i) / 5;
            sprintf(buffer, "%d", labels_vals[i]);
            uint8_t label_x = (labels_vals[i] == 100) ? CHART_X - 24 : CHART_X - 16;
            gfx_PrintStringXY(buffer, label_x, y - 4);
        }
    } else {
        // Normal 0-100 scale
        for (uint8_t i = 0; i <= 5; i++) {
            uint8_t value = (5 - i) * 20;
            uint8_t y = CHART_Y + (CHART_HEIGHT * i) / 5;
            sprintf(buffer, "%d", value);
            uint8_t label_x = (value == 100) ? CHART_X - 24 : CHART_X - 16;
            gfx_PrintStringXY(buffer, label_x, y - 4);
        }
    }
}

static void draw_bars(void)
{
    uint8_t bar_width = (CHART_WIDTH - BAR_SPACING * (num_bars + 1)) / num_bars;
    uint8_t max_value = 100;

    for (uint8_t i = 0; i < num_bars; i++) {
        // Calculate bar dimensions
        uint16_t x = CHART_X + BAR_SPACING * (i + 1) + bar_width * i;
        uint8_t bar_height = (data[i] * CHART_HEIGHT) / max_value;
        uint8_t y = CHART_Y + CHART_HEIGHT - bar_height;

        // Draw bar with rounded top and flat bottom
        uint8_t corner_radius = 4;

        // Handle small bars that are shorter than the corner radius
        if (bar_height <= corner_radius) {
            corner_radius = bar_height - 1;
            if (corner_radius < 1) corner_radius = 1;
        }

        // STEP 1: Draw outline FIRST (forms the border)
        gfx_SetColor(bar_outline_colors[i % 6]);

        // Draw arc outlines
        oxy_Arc(x + corner_radius, y + corner_radius, corner_radius, 180, 270);
        oxy_Arc(x + bar_width - corner_radius - 1, y + corner_radius, corner_radius, 270, 360);

        // Draw straight edge outlines
        gfx_HorizLine(x + corner_radius, y, bar_width - 2 * corner_radius);
        if (bar_height > corner_radius) {
            gfx_VertLine(x, y + corner_radius, bar_height - corner_radius);
            gfx_VertLine(x + bar_width - 1, y + corner_radius, bar_height - corner_radius);
        }

        // STEP 2: Fill INSIDE the outline (1px smaller radius)
        gfx_SetColor(bar_colors[i % 6]);

        // Main rectangle body
        if (bar_height > corner_radius) {
            gfx_FillRectangle(x + 1, y + corner_radius, bar_width - 2, bar_height - corner_radius);
        }

        // Fill rounded top area - use radius-1 to stay inside outline
        uint8_t fill_radius = (corner_radius > 1) ? corner_radius - 1 : 0;
        if (fill_radius > 0) {
            gfx_FillCircle(x + corner_radius, y + corner_radius, fill_radius);
            gfx_FillCircle(x + bar_width - corner_radius - 1, y + corner_radius, fill_radius);
            gfx_FillRectangle(x + corner_radius, y + 1, bar_width - 2 * corner_radius, fill_radius);
        }

        // No bottom line - it would overlap with x-axis creating double thickness

        // Draw value on top of bar
        char value_str[4];
        sprintf(value_str, "%d", data[i]);
        uint8_t text_width = strlen(value_str) * 8;
        gfx_SetTextFGColor(COLOR_TITLE);
        gfx_SetTextBGColor(COLOR_BACKGROUND);

        if (bar_height > 15) {
            gfx_PrintStringXY(value_str, x + (bar_width - text_width) / 2, y - 10);
        } else {
            gfx_PrintStringXY(value_str, x + (bar_width - text_width) / 2, y + 2);
        }

        // Draw X-axis label with smart positioning to avoid overlap
        uint8_t label_width = strlen(labels[i]) * 8;

        // Use signed int to prevent underflow when bar_width < label_width
        int16_t label_x;
        if (bar_width >= label_width) {
            label_x = x + (bar_width - label_width) / 2;
        } else {
            // Bar is narrower than label, center on bar position
            label_x = x + bar_width / 2 - label_width / 2;
        }

        // Ensure label doesn't go off left edge
        if (label_x < 0) label_x = 0;

        // If bars are crowded (more than 8), use alternating vertical positions
        uint8_t label_y;
        if (num_bars > 8) {
            // Alternate labels up and down for diagonal effect
            label_y = CHART_Y + CHART_HEIGHT + 5 + ((i % 2) * 10);
        } else {
            label_y = CHART_Y + CHART_HEIGHT + 5;
        }

        // Draw guide line from x-axis to label
        gfx_SetColor(COLOR_GRID);
        uint16_t bar_center = x + bar_width / 2;
        gfx_VertLine(bar_center, CHART_Y + CHART_HEIGHT + 1, label_y - (CHART_Y + CHART_HEIGHT + 1) - 1);

        gfx_PrintStringXY(labels[i], (uint16_t)label_x, label_y);
    }
}

static void draw_legend(void)
{
    // Draw interactive controls
    gfx_SetTextFGColor(COLOR_LABEL);
    gfx_SetTextBGColor(COLOR_BACKGROUND);
    gfx_PrintStringXY("[+] Add [(-)] Remove [ENTER] Random", 10, LCD_HEIGHT - 20);
    gfx_PrintStringXY("[GRAPH] Toggle Break  [CLEAR] Exit", 10, LCD_HEIGHT - 10);
}

int main(void)
{
    bool needs_redraw = true;

    // Seed random number generator
    srand(rtc_Time());

    // Initialize graphics
    gfx_Begin();
    gfx_SetDrawBuffer();

    // Main event loop
    while (!(kb_Data[6] & kb_Clear)) {
        kb_Scan();

        // Handle + key (add bar)
        if (kb_IsDown(kb_KeyAdd)) {
            if (num_bars < MAX_BARS) {
                num_bars++;
                needs_redraw = true;
                while (kb_IsDown(kb_KeyAdd)) kb_Scan();
            }
        }

        // Handle - key (remove bar)
        if (kb_IsDown(kb_KeyChs)) {
            if (num_bars > 1) {
                num_bars--;
                needs_redraw = true;
                while (kb_IsDown(kb_KeyChs)) kb_Scan();
            }
        }

        // Handle ENTER key (randomize data)
        if (kb_Data[6] & kb_Enter) {
            randomize_data();
            needs_redraw = true;
            while (kb_Data[6] & kb_Enter) kb_Scan();
        }

        // Handle GRAPH key (toggle axis break)
        if (kb_Data[1] & kb_Graph) {
            axis_break_enabled = !axis_break_enabled;
            needs_redraw = true;
            while (kb_Data[1] & kb_Graph) kb_Scan();
        }

        // Redraw chart if needed
        if (needs_redraw) {
            // Clear screen with background color
            gfx_FillScreen(COLOR_BACKGROUND);

            // Draw chart components
            draw_chart_frame();
            draw_bars();
            draw_legend();

            // Swap buffer to display
            gfx_SwapDraw();

            needs_redraw = false;
        }
    }

    // Clean up
    gfx_End();

    return 0;
}
