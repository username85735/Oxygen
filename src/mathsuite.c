/*
 * MAT142 Exam 2 Suite - TI-84 Plus CE
 * Comprehensive statistics and probability calculator
 * Optimized for eZ80 hardware constraints
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <graphx.h>
#include <keypadc.h>

// Hardware constraints
#define MAX_FACTORIAL 170  // Beyond this, float overflow
#define EPSILON 0.000001   // Precision threshold
#define MAX_ITERATIONS 50  // For iterative solvers

// Screen layout
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define MARGIN 5
#define LINE_HEIGHT 10

// Colors
#define COLOR_BG 255
#define COLOR_TEXT 0
#define COLOR_HEADER 224
#define COLOR_INPUT 227
#define COLOR_OUTPUT 151

// ====================
// MATH LIBRARY
// ====================

// Safe factorial with overflow protection
static float safe_factorial(int n) {
    if (n < 0) return 0.0f;
    if (n > MAX_FACTORIAL) return INFINITY;
    if (n == 0 || n == 1) return 1.0f;

    float result = 1.0f;
    for (int i = 2; i <= n; i++) {
        result *= i;
        if (isinf(result)) return INFINITY;
    }
    return result;
}

// Safe permutation: nPr = n! / (n-r)!
static float permutation(int n, int r) {
    if (r > n || r < 0 || n < 0) return 0.0f;
    if (r == 0) return 1.0f;

    // Calculate directly to avoid overflow: n * (n-1) * ... * (n-r+1)
    float result = 1.0f;
    for (int i = 0; i < r; i++) {
        result *= (n - i);
        if (isinf(result)) return INFINITY;
    }
    return result;
}

// Safe combination: nCr = n! / (r! * (n-r)!)
static float combination(int n, int r) {
    if (r > n || r < 0 || n < 0) return 0.0f;
    if (r == 0 || r == n) return 1.0f;

    // Optimize by using smaller r
    if (r > n - r) r = n - r;

    // Calculate directly to avoid overflow
    float result = 1.0f;
    for (int i = 0; i < r; i++) {
        result *= (n - i);
        result /= (i + 1);
        if (isinf(result)) return INFINITY;
    }
    return result;
}

// Normal distribution CDF approximation (Abramowitz & Stegun)
// Returns P(Z <= z) for standard normal distribution
static float normal_cdf(float z) {
    const float a1 =  0.254829592f;
    const float a2 = -0.284496736f;
    const float a3 =  1.421413741f;
    const float a4 = -1.453152027f;
    const float a5 =  1.061405429f;
    const float p  =  0.3275911f;

    // Save the sign of z
    int sign = (z < 0) ? -1 : 1;
    z = fabsf(z) / sqrtf(2.0f);

    // A&S formula 7.1.26
    float t = 1.0f / (1.0f + p * z);
    float y = 1.0f - (((((a5 * t + a4) * t) + a3) * t + a2) * t + a1) * t * expf(-z * z);

    return 0.5f * (1.0f + sign * y);
}

// Inverse normal CDF using binary search
// Returns z such that P(Z <= z) = p
static float inverse_normal_cdf(float p) {
    if (p <= 0.0f) return -INFINITY;
    if (p >= 1.0f) return INFINITY;
    if (fabsf(p - 0.5f) < EPSILON) return 0.0f;

    // Binary search bounds
    float low = -10.0f;
    float high = 10.0f;
    float mid, cdf_mid;

    // Binary search
    for (int i = 0; i < MAX_ITERATIONS; i++) {
        mid = (low + high) / 2.0f;
        cdf_mid = normal_cdf(mid);

        if (fabsf(cdf_mid - p) < EPSILON) {
            return mid;
        }

        if (cdf_mid < p) {
            low = mid;
        } else {
            high = mid;
        }
    }

    return mid;
}

// ====================
// UI STATE
// ====================

typedef enum {
    MENU_MAIN,
    MENU_NORMAL_DIST,
    MENU_PROB_LOGIC,
    MENU_COUNTING
} MenuState;

static MenuState current_menu = MENU_MAIN;
static int selected_option = 0;

// Input buffers
static char input_buffer[32];
static int input_index = 0;
static bool input_active = false;

// Normal distribution state
static float norm_mu = 0.0f;
static float norm_sigma = 1.0f;
static float norm_x = 0.0f;
static float norm_z = 0.0f;
static float norm_area = 0.0f;

// Probability state
static float prob_a = 0.0f;
static float prob_b = 0.0f;
static float prob_ab = 0.0f;

// Counting state
static int count_n = 0;
static int count_r = 0;

// ====================
// UI DRAWING
// ====================

static void draw_header(const char *title) {
    gfx_SetColor(COLOR_HEADER);
    gfx_FillRectangle(0, 0, SCREEN_WIDTH, 20);
    gfx_SetTextFGColor(COLOR_TEXT);
    gfx_SetTextTransparentColor(COLOR_HEADER);
    gfx_PrintStringXY(title, MARGIN, 5);
}

static void draw_main_menu(void) {
    gfx_FillScreen(COLOR_BG);
    draw_header("MAT142 Exam 2 Suite");

    gfx_SetTextFGColor(COLOR_TEXT);
    gfx_SetTextTransparentColor(COLOR_BG);

    const char *options[] = {
        "1. Normal Distribution Solver",
        "2. Probability & Set Logic",
        "3. Counting & Combinatorics",
        "CLEAR: Exit"
    };

    int y = 40;
    for (int i = 0; i < 4; i++) {
        if (i == selected_option) {
            gfx_SetColor(COLOR_INPUT);
            gfx_FillRectangle(MARGIN, y - 2, SCREEN_WIDTH - 2 * MARGIN, LINE_HEIGHT + 2);
        }
        gfx_SetTextTransparentColor((i == selected_option) ? COLOR_INPUT : COLOR_BG);
        gfx_PrintStringXY(options[i], MARGIN + 5, y);
        y += LINE_HEIGHT + 5;
    }
}

static void draw_normal_dist_menu(void) {
    gfx_FillScreen(COLOR_BG);
    draw_header("Normal Distribution");

    gfx_SetTextFGColor(COLOR_TEXT);
    gfx_SetTextTransparentColor(COLOR_BG);

    char buf[64];
    int y = 30;

    // Inputs
    gfx_PrintStringXY("--- Inputs ---", MARGIN, y);
    y += LINE_HEIGHT + 3;

    sprintf(buf, "Mean (mu): %.3f", norm_mu);
    gfx_PrintStringXY(buf, MARGIN, y);
    y += LINE_HEIGHT;

    sprintf(buf, "Std Dev (sigma): %.3f", norm_sigma);
    gfx_PrintStringXY(buf, MARGIN, y);
    y += LINE_HEIGHT;

    sprintf(buf, "Raw Score (X): %.3f", norm_x);
    gfx_PrintStringXY(buf, MARGIN, y);
    y += LINE_HEIGHT + 5;

    // Outputs
    gfx_SetColor(COLOR_OUTPUT);
    gfx_FillRectangle(MARGIN, y, SCREEN_WIDTH - 2 * MARGIN, 60);
    gfx_SetTextTransparentColor(COLOR_OUTPUT);

    y += 5;
    gfx_PrintStringXY("--- Results ---", MARGIN + 5, y);
    y += LINE_HEIGHT + 3;

    // Calculate Z-score
    if (norm_sigma > EPSILON) {
        norm_z = (norm_x - norm_mu) / norm_sigma;
        sprintf(buf, "Z-Score: %.4f", norm_z);
        gfx_PrintStringXY(buf, MARGIN + 5, y);
        y += LINE_HEIGHT;

        // Calculate area/probability
        norm_area = normal_cdf(norm_z);
        sprintf(buf, "P(Z<=z): %.4f (%.2f%%)", norm_area, norm_area * 100.0f);
        gfx_PrintStringXY(buf, MARGIN + 5, y);
        y += LINE_HEIGHT;

        sprintf(buf, "Percentile: %.2f", norm_area * 100.0f);
        gfx_PrintStringXY(buf, MARGIN + 5, y);
        y += LINE_HEIGHT;
    } else {
        gfx_PrintStringXY("Error: sigma must be > 0", MARGIN + 5, y);
    }

    // Empirical Rule
    gfx_SetTextTransparentColor(COLOR_BG);
    y += 10;
    gfx_PrintStringXY("--- Empirical Rule (68-95-99.7) ---", MARGIN, y);
    y += LINE_HEIGHT + 2;

    sprintf(buf, "68%%: [%.2f, %.2f]", norm_mu - norm_sigma, norm_mu + norm_sigma);
    gfx_PrintStringXY(buf, MARGIN, y);
    y += LINE_HEIGHT;

    sprintf(buf, "95%%: [%.2f, %.2f]", norm_mu - 2 * norm_sigma, norm_mu + 2 * norm_sigma);
    gfx_PrintStringXY(buf, MARGIN, y);
    y += LINE_HEIGHT;

    sprintf(buf, "99.7%%: [%.2f, %.2f]", norm_mu - 3 * norm_sigma, norm_mu + 3 * norm_sigma);
    gfx_PrintStringXY(buf, MARGIN, y);
    y += LINE_HEIGHT + 5;

    gfx_PrintStringXY("1-3: Edit | GRAPH: Inv CDF | 2nd: Menu", MARGIN, SCREEN_HEIGHT - 15);
}

static void draw_prob_logic_menu(void) {
    gfx_FillScreen(COLOR_BG);
    draw_header("Probability & Set Logic");

    gfx_SetTextFGColor(COLOR_TEXT);
    gfx_SetTextTransparentColor(COLOR_BG);

    char buf[64];
    int y = 30;

    // Inputs
    gfx_PrintStringXY("--- Inputs ---", MARGIN, y);
    y += LINE_HEIGHT + 3;

    sprintf(buf, "P(A): %.4f", prob_a);
    gfx_PrintStringXY(buf, MARGIN, y);
    y += LINE_HEIGHT;

    sprintf(buf, "P(B): %.4f", prob_b);
    gfx_PrintStringXY(buf, MARGIN, y);
    y += LINE_HEIGHT;

    sprintf(buf, "P(A and B): %.4f", prob_ab);
    gfx_PrintStringXY(buf, MARGIN, y);
    y += LINE_HEIGHT + 5;

    // Outputs
    gfx_SetColor(COLOR_OUTPUT);
    gfx_FillRectangle(MARGIN, y, SCREEN_WIDTH - 2 * MARGIN, 90);
    gfx_SetTextTransparentColor(COLOR_OUTPUT);

    y += 5;
    gfx_PrintStringXY("--- Results ---", MARGIN + 5, y);
    y += LINE_HEIGHT + 3;

    // Calculate union
    float p_union = prob_a + prob_b - prob_ab;
    sprintf(buf, "P(A or B): %.4f", p_union);
    gfx_PrintStringXY(buf, MARGIN + 5, y);
    y += LINE_HEIGHT;

    // Conditional probabilities
    if (prob_b > EPSILON) {
        float p_a_given_b = prob_ab / prob_b;
        sprintf(buf, "P(A|B): %.4f", p_a_given_b);
        gfx_PrintStringXY(buf, MARGIN + 5, y);
    } else {
        gfx_PrintStringXY("P(A|B): undefined", MARGIN + 5, y);
    }
    y += LINE_HEIGHT;

    if (prob_a > EPSILON) {
        float p_b_given_a = prob_ab / prob_a;
        sprintf(buf, "P(B|A): %.4f", p_b_given_a);
        gfx_PrintStringXY(buf, MARGIN + 5, y);
    } else {
        gfx_PrintStringXY("P(B|A): undefined", MARGIN + 5, y);
    }
    y += LINE_HEIGHT + 5;

    // Logic checks
    gfx_PrintStringXY("--- Logic Checks ---", MARGIN + 5, y);
    y += LINE_HEIGHT + 2;

    // Independence: P(A|B) = P(A)
    bool independent = false;
    if (prob_b > EPSILON) {
        float p_a_given_b = prob_ab / prob_b;
        independent = fabsf(p_a_given_b - prob_a) < EPSILON;
    }
    sprintf(buf, "Independent: %s", independent ? "YES" : "NO");
    gfx_PrintStringXY(buf, MARGIN + 5, y);
    y += LINE_HEIGHT;

    // Mutually Exclusive: P(A and B) = 0
    bool mut_exclusive = (prob_ab < EPSILON);
    sprintf(buf, "Mutually Exclusive: %s", mut_exclusive ? "YES" : "NO");
    gfx_PrintStringXY(buf, MARGIN + 5, y);

    gfx_SetTextTransparentColor(COLOR_BG);
    gfx_PrintStringXY("1-3: Edit | 2nd: Menu", MARGIN, SCREEN_HEIGHT - 15);
}

static void draw_counting_menu(void) {
    gfx_FillScreen(COLOR_BG);
    draw_header("Counting & Combinatorics");

    gfx_SetTextFGColor(COLOR_TEXT);
    gfx_SetTextTransparentColor(COLOR_BG);

    char buf[64];
    int y = 30;

    // Inputs
    gfx_PrintStringXY("--- Inputs ---", MARGIN, y);
    y += LINE_HEIGHT + 3;

    sprintf(buf, "n (total items): %d", count_n);
    gfx_PrintStringXY(buf, MARGIN, y);
    y += LINE_HEIGHT;

    sprintf(buf, "r (selected): %d", count_r);
    gfx_PrintStringXY(buf, MARGIN, y);
    y += LINE_HEIGHT + 5;

    // Outputs
    gfx_SetColor(COLOR_OUTPUT);
    gfx_FillRectangle(MARGIN, y, SCREEN_WIDTH - 2 * MARGIN, 60);
    gfx_SetTextTransparentColor(COLOR_OUTPUT);

    y += 5;
    gfx_PrintStringXY("--- Results ---", MARGIN + 5, y);
    y += LINE_HEIGHT + 3;

    // Factorial
    float fact_n = safe_factorial(count_n);
    if (isinf(fact_n)) {
        sprintf(buf, "n!: OVERFLOW (n > %d)", MAX_FACTORIAL);
    } else {
        sprintf(buf, "n!: %.6g", fact_n);
    }
    gfx_PrintStringXY(buf, MARGIN + 5, y);
    y += LINE_HEIGHT;

    // Permutation
    float perm = permutation(count_n, count_r);
    if (isinf(perm)) {
        sprintf(buf, "nPr: OVERFLOW");
    } else {
        sprintf(buf, "nPr: %.6g", perm);
    }
    gfx_PrintStringXY(buf, MARGIN + 5, y);
    y += LINE_HEIGHT;

    // Combination
    float comb = combination(count_n, count_r);
    if (isinf(comb)) {
        sprintf(buf, "nCr: OVERFLOW");
    } else {
        sprintf(buf, "nCr: %.6g", comb);
    }
    gfx_PrintStringXY(buf, MARGIN + 5, y);

    gfx_SetTextTransparentColor(COLOR_BG);
    gfx_PrintStringXY("1-2: Edit | 2nd: Menu", MARGIN, SCREEN_HEIGHT - 15);
}

static void draw_current_menu(void) {
    switch (current_menu) {
        case MENU_MAIN:
            draw_main_menu();
            break;
        case MENU_NORMAL_DIST:
            draw_normal_dist_menu();
            break;
        case MENU_PROB_LOGIC:
            draw_prob_logic_menu();
            break;
        case MENU_COUNTING:
            draw_counting_menu();
            break;
    }
}

// ====================
// INPUT HANDLING
// ====================

static float get_numeric_input(const char *prompt, float default_val) {
    gfx_FillScreen(COLOR_BG);
    draw_header("Enter Value");

    gfx_SetTextFGColor(COLOR_TEXT);
    gfx_SetTextTransparentColor(COLOR_BG);
    gfx_PrintStringXY(prompt, MARGIN, 40);
    gfx_PrintStringXY("Use number keys, (-): negative", MARGIN, 60);
    gfx_PrintStringXY("ENTER: Confirm | DEL: Backspace", MARGIN, 70);

    input_buffer[0] = '\0';
    input_index = 0;

    while (true) {
        kb_Scan();

        if (kb_IsDown(kb_KeyEnter)) {
            if (input_index > 0) {
                return atof(input_buffer);
            }
            return default_val;
        }

        if (kb_IsDown(kb_KeyClear)) {
            return default_val;
        }

        if (kb_IsDown(kb_KeyDel)) {
            if (input_index > 0) {
                input_index--;
                input_buffer[input_index] = '\0';
            }
        }

        // Number keys
        if (kb_Data[kb_group_1] || kb_Data[kb_group_2]) {
            char key = '\0';
            if (kb_IsDown(kb_Key0)) key = '0';
            if (kb_IsDown(kb_Key1)) key = '1';
            if (kb_IsDown(kb_Key2)) key = '2';
            if (kb_IsDown(kb_Key3)) key = '3';
            if (kb_IsDown(kb_Key4)) key = '4';
            if (kb_IsDown(kb_Key5)) key = '5';
            if (kb_IsDown(kb_Key6)) key = '6';
            if (kb_IsDown(kb_Key7)) key = '7';
            if (kb_IsDown(kb_Key8)) key = '8';
            if (kb_IsDown(kb_Key9)) key = '9';
            if (kb_IsDown(kb_KeyDecPnt)) key = '.';
            if (kb_IsDown(kb_KeyChs)) key = '-';  // Negative sign

            if (key != '\0' && input_index < 31) {
                input_buffer[input_index++] = key;
                input_buffer[input_index] = '\0';
            }
            delay(150);
        }

        // Display current input
        gfx_SetColor(COLOR_BG);
        gfx_FillRectangle(MARGIN, 90, SCREEN_WIDTH - 2 * MARGIN, 20);
        gfx_SetTextFGColor(COLOR_TEXT);
        gfx_PrintStringXY(input_buffer, MARGIN, 95);

        delay(50);
    }
}

// ====================
// MAIN PROGRAM
// ====================

int main(void) {
    gfx_Begin();
    gfx_SetDrawBuffer();

    // Initial values
    norm_mu = 50.0f;
    norm_sigma = 10.0f;
    norm_x = 60.0f;
    prob_a = 0.3f;
    prob_b = 0.4f;
    prob_ab = 0.1f;
    count_n = 10;
    count_r = 3;

    draw_current_menu();
    gfx_SwapDraw();

    while (true) {
        kb_Scan();

        if (kb_IsDown(kb_KeyClear)) {
            if (current_menu == MENU_MAIN) {
                break;
            } else {
                current_menu = MENU_MAIN;
                selected_option = 0;
                draw_current_menu();
                gfx_SwapDraw();
                delay(200);
            }
        }

        if (current_menu == MENU_MAIN) {
            // Navigate menu
            if (kb_IsDown(kb_KeyDown)) {
                selected_option = (selected_option + 1) % 3;
                draw_current_menu();
                gfx_SwapDraw();
                delay(150);
            }
            if (kb_IsDown(kb_KeyUp)) {
                selected_option = (selected_option - 1 + 3) % 3;
                draw_current_menu();
                gfx_SwapDraw();
                delay(150);
            }
            if (kb_IsDown(kb_KeyEnter)) {
                current_menu = (MenuState)(selected_option + 1);
                draw_current_menu();
                gfx_SwapDraw();
                delay(200);
            }
        } else if (current_menu == MENU_NORMAL_DIST) {
            // Edit inputs
            if (kb_IsDown(kb_Key1)) {
                norm_mu = get_numeric_input("Enter Mean (mu):", norm_mu);
                draw_current_menu();
                gfx_SwapDraw();
            }
            if (kb_IsDown(kb_Key2)) {
                norm_sigma = get_numeric_input("Enter Std Dev (sigma):", norm_sigma);
                if (norm_sigma <= 0) norm_sigma = 1.0f;
                draw_current_menu();
                gfx_SwapDraw();
            }
            if (kb_IsDown(kb_Key3)) {
                norm_x = get_numeric_input("Enter Raw Score (X):", norm_x);
                draw_current_menu();
                gfx_SwapDraw();
            }
            // Inverse CDF
            if (kb_IsDown(kb_KeyGraph)) {
                float percentile = get_numeric_input("Enter percentile (0-1):", 0.95f);
                if (percentile > 0 && percentile < 1) {
                    float z_inv = inverse_normal_cdf(percentile);
                    norm_x = norm_mu + z_inv * norm_sigma;
                }
                draw_current_menu();
                gfx_SwapDraw();
            }
            if (kb_IsDown(kb_Key2nd)) {
                current_menu = MENU_MAIN;
                draw_current_menu();
                gfx_SwapDraw();
                delay(200);
            }
        } else if (current_menu == MENU_PROB_LOGIC) {
            if (kb_IsDown(kb_Key1)) {
                prob_a = get_numeric_input("Enter P(A):", prob_a);
                draw_current_menu();
                gfx_SwapDraw();
            }
            if (kb_IsDown(kb_Key2)) {
                prob_b = get_numeric_input("Enter P(B):", prob_b);
                draw_current_menu();
                gfx_SwapDraw();
            }
            if (kb_IsDown(kb_Key3)) {
                prob_ab = get_numeric_input("Enter P(A and B):", prob_ab);
                draw_current_menu();
                gfx_SwapDraw();
            }
            if (kb_IsDown(kb_Key2nd)) {
                current_menu = MENU_MAIN;
                draw_current_menu();
                gfx_SwapDraw();
                delay(200);
            }
        } else if (current_menu == MENU_COUNTING) {
            if (kb_IsDown(kb_Key1)) {
                float val = get_numeric_input("Enter n (total):", count_n);
                count_n = (int)val;
                if (count_n < 0) count_n = 0;
                draw_current_menu();
                gfx_SwapDraw();
            }
            if (kb_IsDown(kb_Key2)) {
                float val = get_numeric_input("Enter r (selected):", count_r);
                count_r = (int)val;
                if (count_r < 0) count_r = 0;
                draw_current_menu();
                gfx_SwapDraw();
            }
            if (kb_IsDown(kb_Key2nd)) {
                current_menu = MENU_MAIN;
                draw_current_menu();
                gfx_SwapDraw();
                delay(200);
            }
        }

        delay(50);
    }

    gfx_End();
    return 0;
}
