#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <graphx.h>
#include <keypadc.h>

// Fast integer sine/cosine lookup table (0-360 degrees)
// Values are scaled by 256 for precision
static const int16_t sin_table[361] = {
    0, 4, 9, 13, 18, 22, 27, 31, 36, 40, 44, 49, 53, 58, 62, 66, 71, 75, 79, 83,
    88, 92, 96, 100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 139, 143, 147, 150, 154, 158, 161,
    165, 168, 171, 175, 178, 181, 184, 187, 190, 193, 196, 199, 202, 204, 207, 210, 212, 215, 217, 219,
    222, 224, 226, 228, 230, 232, 234, 236, 237, 239, 241, 242, 243, 245, 246, 247, 248, 249, 250, 251,
    252, 253, 253, 254, 254, 255, 255, 255, 256, 256, 256, 256, 256, 256, 256, 255, 255, 255, 254, 254,
    253, 253, 252, 251, 250, 249, 248, 247, 246, 245, 243, 242, 241, 239, 237, 236, 234, 232, 230, 228,
    226, 224, 222, 219, 217, 215, 212, 210, 207, 204, 202, 199, 196, 193, 190, 187, 184, 181, 178, 175,
    171, 168, 165, 161, 158, 154, 150, 147, 143, 139, 136, 132, 128, 124, 120, 116, 112, 108, 104, 100,
    96, 92, 88, 83, 79, 75, 71, 66, 62, 58, 53, 49, 44, 40, 36, 31, 27, 22, 18, 13,
    9, 4, 0, -4, -9, -13, -18, -22, -27, -31, -36, -40, -44, -49, -53, -58, -62, -66, -71, -75,
    -79, -83, -88, -92, -96, -100, -104, -108, -112, -116, -120, -124, -128, -132, -136, -139, -143, -147, -150, -154,
    -158, -161, -165, -168, -171, -175, -178, -181, -184, -187, -190, -193, -196, -199, -202, -204, -207, -210, -212, -215,
    -217, -219, -222, -224, -226, -228, -230, -232, -234, -236, -237, -239, -241, -242, -243, -245, -246, -247, -248, -249,
    -250, -251, -252, -253, -253, -254, -254, -255, -255, -255, -256, -256, -256, -256, -256, -256, -256, -255, -255, -255,
    -254, -254, -253, -253, -252, -251, -250, -249, -248, -247, -246, -245, -243, -242, -241, -239, -237, -236, -234, -232,
    -230, -228, -226, -224, -222, -219, -217, -215, -212, -210, -207, -204, -202, -199, -196, -193, -190, -187, -184, -181,
    -178, -175, -171, -168, -165, -161, -158, -154, -150, -147, -143, -139, -136, -132, -128, -124, -120, -116, -112, -108,
    -104, -100, -96, -92, -88, -83, -79, -75, -71, -66, -62, -58, -53, -49, -44, -40, -36, -31, -27, -22,
    0  // 360 degrees = 0 degrees
};

// Fast sine using lookup table
static int16_t fast_sin(int16_t deg) {
    while (deg < 0) deg += 360;
    while (deg >= 360) deg -= 360;
    return sin_table[deg];
}

// Fast cosine using sine lookup
static int16_t fast_cos(int16_t deg) {
    return fast_sin(deg + 90);
}

#define MAX_SLICES 8
#define MIN_SLICES 3

// Chart positioning
#define CHART_CENTER_X 160
#define CHART_CENTER_Y 110
#define CHART_RADIUS 70

// Colors
#define COLOR_BG 255        // White
#define COLOR_TEXT 0        // Black
#define COLOR_LINE 224      // Light gray for connecting lines

// Pastel slice colors (matching bar chart theme)
static const uint8_t slice_colors[] = {
    224,  // Light blue
    227,  // Coral/salmon
    151,  // Mint green
    220,  // Lavender
    228,  // Peach
    148,  // Sky blue
    250,  // Light pink
    200   // Pale yellow
};

// Data
static uint8_t num_slices = 5;
static char labels[MAX_SLICES][8] = {"Sales", "Market", "Dev", "Support", "Admin", "R&D", "HR", "Ops"};
static uint8_t data[MAX_SLICES] = {25, 30, 20, 15, 10, 18, 12, 22};

// Calculate angle endpoints for each slice
typedef struct {
    int16_t start_angle;  // 0-360 degrees
    int16_t end_angle;
    int16_t mid_angle;
    uint8_t percentage;
    int16_t label_x;
    int16_t label_y;
    bool use_external_label;
} SliceInfo;

static SliceInfo slices[MAX_SLICES];

// Calculate slice information
static void calculate_slices(void) {
    uint16_t total = 0;
    uint8_t i;

    // Calculate total
    for (i = 0; i < num_slices; i++) {
        total += data[i];
    }

    // Calculate angles and percentages
    int16_t current_angle = 0;  // Start at top (0 degrees = 12 o'clock)
    for (i = 0; i < num_slices; i++) {
        slices[i].start_angle = current_angle;
        slices[i].percentage = (data[i] * 100) / total;
        int16_t angle_span = (data[i] * 360) / total;
        slices[i].end_angle = current_angle + angle_span;
        slices[i].mid_angle = current_angle + angle_span / 2;

        // Determine if slice is large enough for internal label (>12%)
        slices[i].use_external_label = (slices[i].percentage < 12);

        // Calculate label position using integer trig
        int16_t angle = slices[i].mid_angle - 90;  // -90 to rotate to 12 o'clock start

        if (slices[i].use_external_label) {
            // Position label outside the pie - further out for readability
            int16_t outer_dist = CHART_RADIUS + 30;
            slices[i].label_x = CHART_CENTER_X + ((fast_cos(angle) * outer_dist) / 256);
            slices[i].label_y = CHART_CENTER_Y + ((fast_sin(angle) * outer_dist) / 256);
        } else {
            // Position label inside the pie (55% of radius for better centering)
            int16_t inner_radius = (CHART_RADIUS * 55) / 100;
            slices[i].label_x = CHART_CENTER_X + ((fast_cos(angle) * inner_radius) / 256);
            slices[i].label_y = CHART_CENTER_Y + ((fast_sin(angle) * inner_radius) / 256);
        }

        current_angle = slices[i].end_angle;
    }
}

// Draw a filled pie slice
static void draw_slice(uint8_t index) {
    SliceInfo *slice = &slices[index];
    uint8_t color = slice_colors[index % 8];

    gfx_SetColor(color);

    // Draw filled wedge using triangles with 1-degree increments for smoothness
    int16_t start = slice->start_angle - 90;  // Rotate to 12 o'clock start
    int16_t end = slice->end_angle - 90;

    // Draw filled arc segments
    for (int16_t angle = start; angle < end; angle++) {
        int16_t x1 = CHART_CENTER_X + ((fast_cos(angle) * CHART_RADIUS) / 256);
        int16_t y1 = CHART_CENTER_Y + ((fast_sin(angle) * CHART_RADIUS) / 256);
        int16_t x2 = CHART_CENTER_X + ((fast_cos(angle + 1) * CHART_RADIUS) / 256);
        int16_t y2 = CHART_CENTER_Y + ((fast_sin(angle + 1) * CHART_RADIUS) / 256);

        gfx_FillTriangle(CHART_CENTER_X, CHART_CENTER_Y, x1, y1, x2, y2);
    }

    // Draw outline - black radial lines at slice boundaries only
    gfx_SetColor(COLOR_TEXT);
    int16_t x_start = CHART_CENTER_X + ((fast_cos(start) * CHART_RADIUS) / 256);
    int16_t y_start = CHART_CENTER_Y + ((fast_sin(start) * CHART_RADIUS) / 256);
    int16_t x_end = CHART_CENTER_X + ((fast_cos(end) * CHART_RADIUS) / 256);
    int16_t y_end = CHART_CENTER_Y + ((fast_sin(end) * CHART_RADIUS) / 256);

    // Draw radial lines from center to edge
    gfx_Line(CHART_CENTER_X, CHART_CENTER_Y, x_start, y_start);
    gfx_Line(CHART_CENTER_X, CHART_CENTER_Y, x_end, y_end);

    // Draw the arc perimeter for this slice
    int16_t prev_x = x_start;
    int16_t prev_y = y_start;
    for (int16_t angle = start + 1; angle <= end; angle++) {
        int16_t x = CHART_CENTER_X + ((fast_cos(angle) * CHART_RADIUS) / 256);
        int16_t y = CHART_CENTER_Y + ((fast_sin(angle) * CHART_RADIUS) / 256);
        gfx_Line(prev_x, prev_y, x, y);
        prev_x = x;
        prev_y = y;
    }
}

// Draw connecting line for external labels
static void draw_label_line(uint8_t index) {
    SliceInfo *slice = &slices[index];

    if (!slice->use_external_label) return;

    // Draw line from edge of pie to label
    int16_t angle = slice->mid_angle - 90;
    int16_t edge_x = CHART_CENTER_X + ((fast_cos(angle) * CHART_RADIUS) / 256);
    int16_t edge_y = CHART_CENTER_Y + ((fast_sin(angle) * CHART_RADIUS) / 256);

    gfx_SetColor(COLOR_LINE);
    gfx_Line(edge_x, edge_y, slice->label_x, slice->label_y);
}

// Draw slice label
static void draw_label(uint8_t index) {
    SliceInfo *slice = &slices[index];
    char line1[16];
    char line2[16];
    bool two_lines = false;

    gfx_SetTextFGColor(COLOR_TEXT);
    gfx_SetTextTransparentColor(COLOR_BG);

    if (slice->use_external_label) {
        // External: show full label on one line
        sprintf(line1, "%s %d%%", labels[index], slice->percentage);

        uint8_t text_width = gfx_GetStringWidth(line1);
        int16_t text_x = slice->label_x - text_width / 2;
        int16_t text_y = slice->label_y - 4;

        // Bounds checking
        if (text_x < 0) text_x = 2;
        if (text_x + text_width > 320) text_x = 320 - text_width - 2;
        if (text_y < 30) text_y = 30;
        if (text_y > 230) text_y = 230;

        gfx_PrintStringXY(line1, text_x, text_y);
    } else {
        // Internal: show label and percentage
        if (slice->percentage >= 15) {
            // Two lines for larger slices
            sprintf(line1, "%s", labels[index]);
            sprintf(line2, "%d%%", slice->percentage);
            two_lines = true;
        } else {
            // Just percentage for smaller slices
            sprintf(line1, "%d%%", slice->percentage);
        }

        if (two_lines) {
            // Draw first line
            uint8_t width1 = gfx_GetStringWidth(line1);
            int16_t x1 = slice->label_x - width1 / 2;
            int16_t y1 = slice->label_y - 8;
            gfx_PrintStringXY(line1, x1, y1);

            // Draw second line
            uint8_t width2 = gfx_GetStringWidth(line2);
            int16_t x2 = slice->label_x - width2 / 2;
            int16_t y2 = slice->label_y + 1;
            gfx_PrintStringXY(line2, x2, y2);
        } else {
            // Single line
            uint8_t text_width = gfx_GetStringWidth(line1);
            int16_t text_x = slice->label_x - text_width / 2;
            int16_t text_y = slice->label_y - 4;
            gfx_PrintStringXY(line1, text_x, text_y);
        }
    }
}

// Draw the complete pie chart
static void draw_pie_chart(void) {
    uint8_t i;

    // Clear screen
    gfx_FillScreen(COLOR_BG);

    // Calculate slice positions
    calculate_slices();

    // Draw title
    gfx_SetTextFGColor(COLOR_TEXT);
    gfx_PrintStringXY("Pie Chart Demo", 10, 10);
    gfx_PrintStringXY("ENTER:Random +/-:Slices", 10, 20);

    // Draw all slices
    for (i = 0; i < num_slices; i++) {
        draw_slice(i);
    }

    // Draw outer circle outline
    gfx_SetColor(COLOR_TEXT);
    gfx_Circle(CHART_CENTER_X, CHART_CENTER_Y, CHART_RADIUS);

    // Draw connecting lines for external labels
    for (i = 0; i < num_slices; i++) {
        draw_label_line(i);
    }

    // Draw all labels
    for (i = 0; i < num_slices; i++) {
        draw_label(i);
    }
}

// Randomize data values
static void randomize_data(void) {
    uint8_t i;
    for (i = 0; i < num_slices; i++) {
        data[i] = (rand() % 40) + 10;  // Random value between 10 and 49
    }
}

// Add a slice
static void add_slice(void) {
    if (num_slices < MAX_SLICES) {
        num_slices++;
    }
}

// Remove a slice
static void remove_slice(void) {
    if (num_slices > MIN_SLICES) {
        num_slices--;
    }
}

// Main function
int main(void) {
    // Initialize graphics
    gfx_Begin();
    gfx_SetDrawBuffer();

    // Seed random number generator
    srand(rtc_Time());

    // Initial draw
    draw_pie_chart();
    gfx_SwapDraw();

    // Main loop
    while (true) {
        kb_Scan();

        // Exit on CLEAR
        if (kb_IsDown(kb_KeyClear)) {
            break;
        }

        // Randomize on ENTER
        if (kb_IsDown(kb_KeyEnter)) {
            randomize_data();
            draw_pie_chart();
            gfx_SwapDraw();
            delay(200);  // Debounce
        }

        // Add slice on +
        if (kb_IsDown(kb_KeyAdd)) {
            add_slice();
            draw_pie_chart();
            gfx_SwapDraw();
            delay(200);  // Debounce
        }

        // Remove slice on -
        if (kb_IsDown(kb_KeyChs)) {
            remove_slice();
            draw_pie_chart();
            gfx_SwapDraw();
            delay(200);  // Debounce
        }

        delay(50);
    }

    // Cleanup
    gfx_End();

    return 0;
}
