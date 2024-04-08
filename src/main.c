#include <time.h>
#include <stdlib.h>

#include "raylib_snake_case.h"

#define MAP_WIDTH 25
#define MAP_HEIGHT 25
#define MINE_COUNT 100

#define FIELD_SIZE 25
#define BORDER_SIZE 15

#define WINDOW_WIDTH MAP_WIDTH * FIELD_SIZE + BORDER_SIZE * 2
#define WINDOW_HEIGHT MAP_HEIGHT * FIELD_SIZE + BORDER_SIZE * 2

#define MIN(a, b) ((a < b) ? a : b)
#define MAX(a, b) ((a > b) ? a : b)

struct field_t {
  bool uncovered;
  bool has_flag;
  bool red_bomb;
  int mine_count;
};

struct field_t* fields;
char* numbers;
bool show_bombs = false;
bool first_click = true;
int flags_left = MINE_COUNT;

rl_font_t minesweeper_font;
rl_texture2d_t flag_texture;
rl_texture2d_t bomb_textrue;

rl_vector2_t vec2(float x, float y) {
  return (rl_vector2_t) {
    .x = x,
    .y = y
  };
}

void load_assets(void) {
  rl_image_t flag_image = rl_load_image("assets/flag.png");
  rl_image_t bomb_image = rl_load_image("assets/bomb.png");

  int flag_size = FIELD_SIZE - 3 * 2 - 2;
  int bomb_size = FIELD_SIZE - 4;

  rl_image_resize(&flag_image, flag_size, flag_size);
  rl_image_resize(&bomb_image, bomb_size, bomb_size);

  flag_texture = rl_load_texture_from_image(flag_image);
  bomb_textrue = rl_load_texture_from_image(bomb_image);

  rl_unload_image(flag_image);
  rl_unload_image(bomb_image);

  minesweeper_font = rl_load_font("assets/minesweeper.ttf");
}

void unload_assets(void) {
  rl_unload_texture(flag_texture);
  rl_unload_texture(bomb_textrue);
  rl_unload_font(minesweeper_font);
}

void init_numbers(void) {
  numbers = rl_mem_alloc(2 * 8);

  for (int i = 0; i < 8; i++) {
    numbers[i * 2] = '1' + i;
    numbers[i * 2 + 1] = '\0';
  }
}

rl_color_t get_color_for_count(int count) {
  static rl_color_t colors[] = { BLUE, GREEN, RED, DARKBLUE, MAROON, SKYBLUE, BLACK, GRAY };
  return colors[count - 1];
}

void draw_border(void) {
  // Outer shade
  rl_draw_triangle(vec2(0, 0), vec2(0, WINDOW_HEIGHT), vec2(WINDOW_WIDTH, 0), WHITE);
  rl_draw_triangle(vec2(WINDOW_WIDTH, 0), vec2(0, WINDOW_HEIGHT), vec2(WINDOW_WIDTH, WINDOW_HEIGHT), GRAY);

  // Border
  rl_draw_rectangle(3, 3, WINDOW_WIDTH - 6, WINDOW_HEIGHT - 6, LIGHTGRAY);

  // Inner shade
  rl_draw_triangle(vec2(BORDER_SIZE - 3, BORDER_SIZE - 3), vec2(BORDER_SIZE - 3, WINDOW_HEIGHT - BORDER_SIZE + 3), vec2(WINDOW_WIDTH - BORDER_SIZE + 3, BORDER_SIZE - 3), GRAY);
  rl_draw_triangle(vec2(WINDOW_WIDTH - BORDER_SIZE + 3, BORDER_SIZE - 3), vec2(BORDER_SIZE - 3, WINDOW_HEIGHT - BORDER_SIZE + 3), vec2(WINDOW_WIDTH - BORDER_SIZE + 3, WINDOW_HEIGHT - BORDER_SIZE + 3), WHITE);

  // Background
  rl_draw_rectangle(BORDER_SIZE, BORDER_SIZE, WINDOW_WIDTH - BORDER_SIZE * 2, WINDOW_HEIGHT - BORDER_SIZE * 2, LIGHTGRAY);
}

void draw_field_covered(int x, int y) {
  // Shade
  rl_draw_triangle(vec2(x, y), vec2(x, y + FIELD_SIZE), vec2(x + FIELD_SIZE, y), WHITE);
  rl_draw_triangle(vec2(x + FIELD_SIZE, y), vec2(x, y + FIELD_SIZE), vec2(x + FIELD_SIZE, y + FIELD_SIZE), GRAY);

  // Field
  rl_draw_rectangle(x + 3, y + 3, FIELD_SIZE - 6, FIELD_SIZE - 6, LIGHTGRAY);
}

void draw_field_uncovered(int x, int y, rl_color_t color) {
  rl_draw_rectangle(x, y, FIELD_SIZE, FIELD_SIZE, color);
  rl_draw_rectangle_lines(x, y, FIELD_SIZE, FIELD_SIZE, GRAY);
}

void draw_mine_count(int x, int y, int count) {
  char* text = &numbers[2 * (count - 1)];
  rl_vector2_t size = rl_measure_text_ex(minesweeper_font, text, 16, 1);
  int text_x = x + FIELD_SIZE / 2 - size.x / 2 + 1;
  int text_y = y + FIELD_SIZE / 2 - size.y / 2;
  rl_draw_text_ex(minesweeper_font, text, vec2(text_x, text_y), 16, 1, get_color_for_count(count));
}

void draw_field(int x, int y) {
  struct field_t* field = &fields[y * MAP_WIDTH + x];

  int screen_x = x * FIELD_SIZE + BORDER_SIZE;
  int screen_y = y * FIELD_SIZE + BORDER_SIZE;

  if (field->uncovered) {
    draw_field_uncovered(screen_x, screen_y, field->red_bomb ? RED : LIGHTGRAY);

    if (field->mine_count > 0)
      draw_mine_count(screen_x, screen_y, field->mine_count);

    if (show_bombs && field->mine_count == -1)
      rl_draw_texture(bomb_textrue, screen_x + 2, screen_y + 2, WHITE);

    if (show_bombs && field->has_flag) {
      rl_draw_line_ex(vec2(screen_x + 1, screen_y + 1), vec2(screen_x + FIELD_SIZE - 1, screen_y + FIELD_SIZE - 1), 2, RED);
      rl_draw_line_ex(vec2(screen_x + 1, screen_y + FIELD_SIZE - 1), vec2(screen_x + FIELD_SIZE - 1, screen_y + 1), 2, RED);
    }
  }
  else {
    draw_field_covered(screen_x, screen_y);

    if (field->has_flag)
      rl_draw_texture(flag_texture, screen_x + 4, screen_y + 4, WHITE);
  }
}

bool is_adjacent_to(int x1, int y1, int x2, int y2) {
  return abs(x1 - x2) <= 1 && abs(y1 - y2) <= 1;
}

void randomize_mines(int excl_x, int excl_y) {
  rl_set_random_seed(time(NULL));

  int mines[MINE_COUNT] = { 0 };
  int count = 0;

  while (count < MINE_COUNT) {
    int value = rl_get_random_value(0, MAP_WIDTH * MAP_HEIGHT - 1);
    bool repeat = false;

    for (int i = 0; i < MINE_COUNT; i++)
      if (mines[i] == value)
        repeat = true;

    if (repeat || is_adjacent_to(value % MAP_WIDTH, value / MAP_WIDTH, excl_x, excl_y))
      continue;

    mines[count] = value;
    count++;
  }

  for (int i = 0; i < MINE_COUNT; i++)
    fields[mines[i]].mine_count = -1;
}

void compute_mine_counts(void) {
  for (int x = 0; x < MAP_WIDTH; x++) {
    for (int y = 0; y < MAP_HEIGHT; y++) {
      if (fields[y * MAP_WIDTH + x].mine_count == -1)
        continue;

      for (int dx = MAX(x - 1, 0); dx <= MIN(x + 1, MAP_WIDTH - 1); dx++) {
        for (int dy = MAX(y - 1, 0); dy <= MIN(y + 1, MAP_HEIGHT - 1); dy++) {
          if (x == dx && y == dy)
            continue;

          if (fields[dy * MAP_WIDTH + dx].mine_count == -1)
            fields[y * MAP_WIDTH + x].mine_count++;
        }
      }
    }
  }
}

void uncover_adjacent_fields(int x, int y) {
  for (int dx = MAX(x - 1, 0); dx <= MIN(x + 1, MAP_WIDTH - 1); dx++) {
    for (int dy = MAX(y - 1, 0); dy <= MIN(y + 1, MAP_HEIGHT - 1); dy++) {
      if (x == dx && y == dy)
        continue;

      struct field_t* field = &fields[dy * MAP_WIDTH + dx];

      if (field->mine_count == -1 || field->uncovered)
        continue;

      field->uncovered = true;

      if (field->mine_count == 0)
        uncover_adjacent_fields(dx, dy);
    }
  }
}

void handle_click(int x, int y) {
  struct field_t* field = &fields[y * MAP_WIDTH + x];

  if (first_click) {
    randomize_mines(x, y);
    compute_mine_counts();
    uncover_adjacent_fields(x, y);
    field->uncovered = true;
    first_click = false;
  }

  if (field->uncovered)
    return;

  if (field->mine_count == -1) {
    show_bombs = true;
    field->red_bomb = true;

    for (int dx = 0; dx < MAP_WIDTH; dx++) {
      for (int dy = 0; dy < MAP_HEIGHT; dy++) {
        struct field_t* curr = &fields[dy * MAP_WIDTH + dx]; 

        if (curr->mine_count == -1 && !curr->has_flag)
          curr->uncovered = true;

        if (curr->has_flag && curr->mine_count > -1) {
          curr->mine_count = -1;
          curr->uncovered = true;
        }
      }
    }

    return;
  }

  field->uncovered = true;

  if (field->mine_count == 0)
    uncover_adjacent_fields(x, y);
}

void handle_input(void) {
  if (show_bombs)
    return;

  rl_vector2_t pos = vec2(rl_get_mouse_x() - BORDER_SIZE, rl_get_mouse_y() - BORDER_SIZE);

  if (pos.x < 0 || pos.y < 0 || pos.x > MAP_WIDTH * FIELD_SIZE || pos.y > MAP_HEIGHT * FIELD_SIZE)
    return;

  int x = pos.x / FIELD_SIZE;
  int y = pos.y / FIELD_SIZE;

  struct field_t* field = &fields[y * MAP_WIDTH + x];

  if (rl_is_mouse_button_pressed(MOUSE_BUTTON_LEFT))
    if (!field->has_flag)
      handle_click(x, y);

  if (rl_is_mouse_button_pressed(MOUSE_BUTTON_RIGHT) && !field->uncovered) {
    if (!field->has_flag && flags_left > 0) {
      field->has_flag = true;
      flags_left--;
    }
    else if (field->has_flag) {
      field->has_flag = false;
      flags_left++;
    }
  }
}

int main(void) {
  rl_init_window(WINDOW_WIDTH, WINDOW_HEIGHT, "Minesuwueeper");
  rl_set_target_fps(30);

  load_assets();
  init_numbers();

  fields = rl_mem_alloc(MAP_WIDTH * MAP_HEIGHT * sizeof(struct field_t));

  while (!rl_window_should_close()) {
    rl_begin_drawing();
    rl_clear_background(WHITE);

    draw_border();

    for (int x = 0; x < MAP_WIDTH; x++)
      for (int y = 0; y < MAP_HEIGHT; y++)
        draw_field(x, y);

    handle_input();

    rl_end_drawing();
  }

  rl_mem_free(fields);
  unload_assets();
  rl_close_window();

  return 0;
}