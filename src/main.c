#include <tice.h>
#include <graphx.h>
#include <keypadc.h>
#include <string.h>
#include <stdio.h>

#include "oxygen/oxygen.h"

// Hard-coded chart data
#define NUM_BARS 6
static const char *labels[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun"};
static const uint8_t data[] = {45, 72, 38, 91, 55, 68};

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
    gfx_VertLine(CHART_X, CHART_Y, CHART_HEIGHT);                  // Y-axis

    // Draw horizontal grid lines
    gfx_SetColor(COLOR_GRID);
    for (uint8_t i = 1; i <= 4; i++) {
        uint8_t y = CHART_Y + (CHART_HEIGHT * i) / 5;
        for (uint16_t x = CHART_X + 1; x < CHART_X + CHART_WIDTH; x += 3) {
            gfx_SetPixel(x, y);
        }
    }

    // Draw Y-axis labels (0-100)
    gfx_SetTextFGColor(COLOR_LABEL);
    char buffer[4];
    for (uint8_t i = 0; i <= 5; i++) {
        uint8_t value = (5 - i) * 20;
        uint8_t y = CHART_Y + (CHART_HEIGHT * i) / 5;
        sprintf(buffer, "%d", value);
        // Adjust x position based on number of digits
        uint8_t label_x = (value == 100) ? CHART_X - 24 : CHART_X - 16;
        gfx_PrintStringXY(buffer, label_x, y - 4);
    }
}

static void draw_bars(void)
{
    uint8_t bar_width = (CHART_WIDTH - BAR_SPACING * (NUM_BARS + 1)) / NUM_BARS;
    uint8_t max_value = 100;

    for (uint8_t i = 0; i < NUM_BARS; i++) {
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

        // Fill main rectangle (flat bottom)
        gfx_SetColor(bar_colors[i]);
        if (bar_height > corner_radius) {
            gfx_FillRectangle(x, y + corner_radius, bar_width, bar_height - corner_radius);
        }

        // Fill rounded top
        gfx_FillCircle(x + corner_radius, y + corner_radius, corner_radius);
        gfx_FillCircle(x + bar_width - corner_radius - 1, y + corner_radius, corner_radius);
        gfx_FillRectangle(x + corner_radius, y, bar_width - 2 * corner_radius, corner_radius);

        // Draw outline with smooth arcs for rounded corners
        gfx_SetColor(bar_outline_colors[i]);

        // Left rounded corner arc (top-left quarter circle)
        oxy_Arc(x + corner_radius, y + corner_radius, corner_radius, 180, 270);

        // Right rounded corner arc (top-right quarter circle)
        oxy_Arc(x + bar_width - corner_radius - 1, y + corner_radius, corner_radius, 270, 360);

        // Top line (between the arcs)
        gfx_HorizLine(x + corner_radius + 1, y, bar_width - 2 * corner_radius - 2);

        // Side lines (only draw if bar extends below the rounded top)
        if (bar_height > corner_radius) {
            gfx_VertLine(x, y + corner_radius, bar_height - corner_radius);
            gfx_VertLine(x + bar_width - 1, y + corner_radius, bar_height - corner_radius);
        }

        // Bottom line (flat)
        gfx_HorizLine(x, y + bar_height - 1, bar_width);

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

        // Draw X-axis label
        uint8_t label_width = strlen(labels[i]) * 8;
        gfx_PrintStringXY(labels[i], x + (bar_width - label_width) / 2,
                         CHART_Y + CHART_HEIGHT + 5);
    }
}

static void draw_legend(void)
{
    // Draw a simple legend
    gfx_SetTextFGColor(COLOR_LABEL);
    gfx_SetTextBGColor(COLOR_BACKGROUND);
    gfx_PrintStringXY("Press [CLEAR] to exit", 10, LCD_HEIGHT - 12);
}

int main(void)
{
    // Initialize graphics
    gfx_Begin();
    gfx_SetDrawBuffer();

    // Clear screen with background color
    gfx_FillScreen(COLOR_BACKGROUND);

    // Draw chart components
    draw_chart_frame();
    draw_bars();
    draw_legend();

    // Swap buffer to display
    gfx_SwapDraw();

    // Wait for user to press CLEAR
    while (!(kb_Data[6] & kb_Clear)) {
        kb_Scan();
    }

    // Clean up
    gfx_End();

    return 0;
}
